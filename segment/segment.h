#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <parallel/algorithm>
#include <pam.h>
#include "pbbslib/get_time.h"
#include "pbbslib/random_shuffle.h"
using namespace std;

timer init_tm, build_tm, total_tm, aug_tm, sort_tm, reserve_tm, outmost_tm, globle_tm, g_tm, extra_tm;

typedef pair<int, int> seg_type;
typedef pair<int, seg_type> seg_type_2d;
struct query_type {
	int x, y1, y2;
};

int infty = 1000000000;

template<typename T>
void shuffle(vector<T> v) {
    srand(unsigned(time(NULL)));
	int n = v.size();
    for (size_t i = 0; i < n; ++i) {
        size_t j = rand() % (n-i);
        swap(v[i], v[i+j]);
    }
}

template<typename T>
void output_content(T t) {
	vector<seg_type_2d> out;
	t.content(std::back_inserter(out));
	for (int i = 0; i < out.size(); i++) cout << "(" << out[i].first << ", " << out[i].second.first << ", " << out[i].second.second << ")";
}

template<typename c_type>
struct SegmentQuery {
	using x_type = c_type;
	using y_type = c_type;
	struct inner_entry {
		using key_t = seg_type_2d; 
		//using val_t = seg_type;
		static bool comp(key_t a, key_t b) { return a.first < b.first;}
	};
	using seg_set = pam_set<inner_entry>;
	using set_node = typename seg_set::node;
	using set_ops = typename seg_set::Tree;
	//using value_sets = int;
	struct aug_sets {
		seg_set first, second, lmr, rml;
		aug_sets(seg_set _first, seg_set _second) : first(_first), second(_second) {}
		aug_sets(seg_set _first, seg_set _second, seg_set _lmr, seg_set _rml) : first(_first), second(_second), lmr(_lmr), rml(_rml){}
		aug_sets() {}
	};
	
	struct outer_entry {
		using key_t = seg_type;
		static bool comp(key_t a, key_t b) { return a < b;}
		using val_t = c_type;
		using aug_t = aug_sets;
		static inline aug_t from_entry(seg_type k, c_type v) { 
			if (k.first>k.second) 
				return aug_sets(seg_set(make_pair(v, make_pair(k.second, k.first))), seg_set()); 
			else 
				return aug_sets(seg_set(), seg_set(make_pair(v, k)));
		}
		static inline aug_t combine(aug_t a, aug_t b) { 
			seg_set rml = seg_set::map_difference(b.first, a.second);
			seg_set l = seg_set::map_union(a.first, rml);

			seg_set lmr = seg_set::map_difference(a.second, b.first);
			seg_set r = seg_set::map_union(b.second, lmr);
			
			//seg_set mid = set_intersection(a.second, b.first);
			
			return aug_sets(l, r, lmr, rml);
		}
		static aug_t get_empty() { return aug_sets(seg_set(), seg_set());}
	};
	using seg_tree = aug_map<outer_entry>;
	using seg_node = typename seg_tree::node;
	using seg_entry = typename seg_tree::E;
	using seg_ops = typename seg_tree::Tree;
//	typedef tree_operations<set_node> tree_ops;

	typename seg_set::E
	get_entry(set_node* r) {return r->entry;}
	
	typename seg_tree::E
	get_entry(seg_node* r) {return r->entry.first;}
	
	typename seg_set::K
	get_key(set_node* r) {return r->entry;}
	
	typename seg_tree::K
	get_key(seg_node* r) {return r->entry.first.first;}
	
	typename seg_tree::V
	get_val(seg_node* r) {return r->entry.first.second;}

	
    SegmentQuery(vector<seg_type_2d>& segs) {
		construct(segs);
    }

    static void finish() {
      seg_tree::finish();
      seg_set::finish();
    }
	
    void construct(vector<seg_type_2d>& segs) {
        const size_t n = segs.size();
		reserve_tm.start();
		seg_tree::reserve(3*n);
		seg_set::reserve(50*n);
		reserve_tm.stop();
		seg_entry *end_points = new seg_entry[2*n];
		total_tm.start();
		
		parallel_for (0, n, [&] (size_t i) {
		    seg_type cur_seg = segs[i].second;
		    end_points[2*i] = make_pair(segs[i].second, segs[i].first);
		    end_points[2*i+1] = make_pair(make_pair(cur_seg.second, cur_seg.first), segs[i].first);
		  });
	
        main_tree = seg_tree(end_points, end_points + (2*n));

		delete[] end_points;
		total_tm.stop();
    }
	
	template<typename OutIter>
	void get_y_range(set_node* r, const query_type& q, OutIter out) {
	  using set_entry = typename seg_set::E;
	  auto f = [&] (set_entry e) {*out = e; ++out;};
	    while (true) {
			if (!r) break;
			if (get_key(r).first<q.y1) {
				r = r->rc; continue; }
			if (get_key(r).first>q.y2) {
				r = r->lc; continue; }
			break;
		}
		if (!r) return;
		*out = get_key(r); ++out;
		set_node* r1 = r->lc;
		while (r1) {
			if (get_key(r1).first<q.y1) {
				r1 = r1->rc; continue;
			}
			if (r1->rc) set_ops::foreach_seq(r1->rc, f);
			f(get_entry(r1));
			r1 = r1->lc; continue;
		}
		set_node* r2 = r->rc;
		while (r2) {
		        if (get_key(r2).first>q.y2) {
				r2 = r2->lc; continue;
			}
			if (r2->lc) set_ops::foreach_seq(r2->lc, f);
			f(get_entry(r2));
			r2 = r2->rc; continue;
		}		
	}
	
	
	template<typename OutIter>
	void query_points(const query_type& q, OutIter* out) {
		seg_node* r = main_tree.root;
		if (!r) return;
		seg_type_2d y1 = make_pair(q.y1, make_pair(infty,infty));
		seg_type_2d y2 = make_pair(q.y2, make_pair(0,0));
		while (r) {
			if (q.x > get_key(r).first) {
				seg_set t = r->entry.second.lmr; 
				//get_y_range(t.root, q, out);
				seg_set tt = seg_set::range(t,y1,y2);
				size_t s = tt.size();
				seg_set::keys(std::move(tt),out);
				out+=s;
				r = r->rc;
			}  else {
				if (q.x < get_key(r).first) {
					seg_set t = r->entry.second.rml;
					//get_y_range(t.root, q, out);
					seg_set tt = seg_set::range(t,y1,y2);
					size_t s = tt.size();
					seg_set::keys(std::move(tt),out);
					out+=s;
					int cur_x1 = get_key(r).first, cur_x2 = get_key(r).second;
					int cur_y = get_val(r);
					if ((cur_x1 >= q.x) && (q.x >= cur_x2) && (q.y1 <= cur_y) && (cur_y <= q.y2)) {
						*out = make_pair(cur_y,get_key(r)); out++;
					}
					r = r->lc;
				} else {
					if (q.x == get_key(r).first) {
						seg_set t1 = r->entry.second.first;
						//get_y_range(t1.root, q, out);
						seg_set tt1 = seg_set::range(t1,y1,y2);
						size_t s1 = tt1.size();
						seg_set::keys(std::move(tt1),out);
						out+=s1;
						seg_set t2 = r->entry.second.rml;
						//get_y_range(t2.root, q, out);
						seg_set tt2 = seg_set::range(t2,y1,y2);
						size_t s2 = tt2.size();
						seg_set::keys(std::move(tt2),out);
						out+=s2;
						break;
					}
				}
			}
		}
	}
	
	int count_y_range(set_node* r, const query_type& q, int cur_count) {
		while (true) {
			if (!r) break;
			if (r->entry.first<q.y1) {
				r = r->rc; continue; }
			if (r->entry.first>q.y2) {
				r = r->lc; continue; }
			break;
		}
		if (!r) return cur_count;
		cur_count++; //  adding r
		set_node* r1 = r->lc;
		while (r1) {
			if (r1->entry.first<q.y1) {
				r1 = r1->rc; continue;
			}
			if (r1->rc) {
				cur_count += r1->rc->s;
			}
			cur_count++;
			r1 = r1->lc; continue;
		}
		set_node* r2 = r->rc;
		while (r2) {
			if (r2->entry.first>q.y2) {
				r2 = r2->lc; continue;
			}
			if (r2->lc) {
				cur_count += r2->lc->s;
			}
			cur_count++;
			r2 = r2->rc; continue;
		}
		return cur_count;
	}	
	
	int query_count(const query_type& q, int cur_count) {
		seg_node* r = main_tree.root;
		if (!r) return cur_count;
		while (r) {
			if (q.x > seg_ops::get_key(r).first) {
				//seg_set t = seg_ops::aug_val(r).lmr; 
				seg_set t = r->entry.second.lmr; 
				cur_count = count_y_range(t.root, q, cur_count);
				r = r->rc;
				continue;
			} 
			if (q.x < seg_ops::get_key(r).first) {
				seg_set t = r->entry.second.rml;
				cur_count = count_y_range(t.root, q, cur_count);
				r = r->lc;
				continue;
			}
			if (q.x == seg_ops::get_key(r).first) {
				seg_set t1 = r->entry.second.first;
				cur_count = count_y_range(t1.root, q, cur_count);
				seg_set t2 = r->entry.second.rml;
				cur_count = count_y_range(t2.root, q, cur_count);
				//cur_count = count_end_at(r->lc, q, cur_count);
				//cur_count = count_start_at(r->rc, q, cur_count);
				break;
			}
		}
		return cur_count;
	}
	
	void print_stats() {
		cout << "outer map: ";  seg_tree::GC::print_stats();
		cout << "Inner map: ";  seg_set::GC::print_stats();
	}


	seg_tree main_tree;
};
