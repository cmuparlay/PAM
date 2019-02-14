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

using data_type = int;

typedef pair<data_type, data_type> point_type;
typedef pair<point_type, point_type> seg_type;
struct query_type {
	int x, y1, y2;
};

template<typename T>
void shuffle(vector<T>& v) {
    srand(unsigned(time(NULL)));
	int n = v.size();
    for (size_t i = 0; i < n; ++i) {
        size_t j = rand() % (n-i);
        swap(v[i], v[i+j]);
    }
}

inline data_type x1(seg_type& s) { return s.first.first;}
inline data_type y1(seg_type& s) { return s.first.second;}
inline data_type x2(seg_type& s) { return s.second.first;}
inline data_type y2(seg_type& s) { return s.second.second;}

double value_at(seg_type& s, int x) {
	if (x2(s) == x1(s)) return double(y1(s));
	double r1 = (double)((x-x1(s))+0.0)/(x2(s)-x1(s)+0.0)*(y1(s)+0.0);
	double r2 = ((x2(s)-x)+0.0)/(x2(s)-x1(s)+0.0)*(y2(s)+0.0);
	return r1+r2;
}

bool in_seg_x_range(seg_type s, int p) {
	if ((x1(s) < p) && (p < x2(s))) return true;
	return false;
}

bool cross(seg_type s, query_type q) {
	double cp;
	if (in_seg_x_range(s, q.x)) cp = value_at(s, q.x); else return false;
	if (q.y1 < cp && cp < q.y2) return true; else return false;
}

template<typename c_type>
struct SegmentQuery {
	using x_type = c_type;
	using y_type = c_type;
	struct inner_entry {
		using key_t = seg_type; 
		static bool comp(key_t a, key_t b) { 
			if (x1(a)<=x1(b)) return (value_at(a, x1(b)) < y1(b));
			return (y1(a) < value_at(b, x1(a)));
		}
	};
	using seg_set = pam_set<inner_entry>;
	using set_node = typename seg_set::node;
	using set_ops = typename seg_set::Tree;
	
	struct aug_sets {
		seg_set first, second, lmr, rml;
		aug_sets(seg_set _first, seg_set _second) : first(_first), second(_second) {}
		aug_sets(seg_set _first, seg_set _second, seg_set _lmr, seg_set _rml) : first(_first), second(_second), lmr(_lmr), rml(_rml){}
		aug_sets() {}
	};
	
	struct outer_entry {
		using key_t = pair<c_type, seg_type>;
		static bool comp(key_t a, key_t b) { return a < b;}
		using val_t = bool; //
		using aug_t = aug_sets;
		static inline aug_t from_entry(key_t k, val_t v) { 
			if (k.first == x2(k.second)) 
				return aug_sets(seg_set(k.second), seg_set()); 
			else 
				return aug_sets(seg_set(), seg_set(k.second));
		}
		static inline aug_t combine(aug_t a, aug_t b) { 
			seg_set rml = seg_set::map_difference(b.first, a.second);
			seg_set l = seg_set::map_union(a.first, rml);
			seg_set lmr = seg_set::map_difference(a.second, b.first);
			seg_set r = seg_set::map_union(b.second, lmr);
			
			return aug_sets(l, r, lmr, rml);
		}
		static aug_t get_empty() { return aug_sets(seg_set(), seg_set());}
	};
	using seg_tree = aug_map<outer_entry>;
	using seg_node = typename seg_tree::node;
	using seg_entry = typename seg_tree::E;

	typename seg_set::E get_entry(set_node* r) {return r->entry;}
	typename seg_tree::E get_entry(seg_node* r) {return r->entry.first;}
	typename seg_set::K get_key(set_node* r) {return r->entry;}	
	typename seg_tree::K get_key(seg_node* r) {return r->entry.first.first;}
		
    SegmentQuery(vector<seg_type>& segs) {
		construct(segs);
    }

    static void finish() {
      seg_tree::finish();
      seg_set::finish();
    }
	
    void construct(vector<seg_type>& segs) {
        const size_t n = segs.size();
	reserve_tm.start();
	seg_tree::reserve(3*n);
	seg_set::reserve(50*n);
	reserve_tm.stop();
	seg_entry *end_points = new seg_entry[2*n];
	total_tm.start();
		
	parallel_for (0, n, [&] (size_t i) {
	    end_points[2*i] = make_pair(make_pair(x1(segs[i]), segs[i]), true);
	    end_points[2*i+1] = make_pair(make_pair(x2(segs[i]), segs[i]), false);
	  });
	
	main_tree = seg_tree(end_points, end_points + (2*n));

	delete[] end_points;
	total_tm.stop();
    }
		
  template<typename OutIter>
	int query_points(const query_type& q, OutIter* out) {
		seg_node* r = main_tree.root;
		if (!r) return 0;
		point_type lp = make_pair(q.x, q.y1);
		point_type rp = make_pair(q.x, q.y2);
		seg_type qy1 = make_pair(lp, lp);
		seg_type qy2 = make_pair(rp, rp);
		int tot = 0;
		while (r) {
			if (q.x > get_key(r).first) {
				seg_set t = r->entry.second.lmr; 
				seg_set tt = seg_set::range(t,qy1,qy2);
				size_t s = tt.size();
				seg_set::keys(std::move(tt),out);
				out+=s; tot+=s;
				r = r->rc;
			}  else {
				if (q.x < get_key(r).first) {
					seg_set t = r->entry.second.rml;
					seg_set tt = seg_set::range(t,qy1,qy2);
					size_t s = tt.size();
					seg_set::keys(std::move(tt),out);
					out+=s; tot+=s;
					seg_type cur = get_key(r).second;
					if (cross(cur, q)) {
						*out = cur; out++; tot++;
					}
					r = r->lc;
				} else break;
			}
		}
		return tot;
	}
	
	
	int count_y_range(set_node* r, const query_type& q, int cur_count) {
		while (true) {
			if (!r) break;
			seg_type s = r->entry;
			double y = value_at(s, q.x);
			if (y < q.y1) { r = r->rc; continue; }
			if (y > q.y2) {	r = r->lc; continue; }
			break;
		}
		if (!r) return cur_count;
		cur_count++; //  adding r
		set_node* r1 = r->lc;
		while (r1) {
			if (value_at(r1->entry, q.x)<q.y1) {
				r1 = r1->rc; continue;
			}
			if (r1->rc) cur_count += r1->rc->s;
			cur_count++;
			r1 = r1->lc; continue;
		}
		set_node* r2 = r->rc;
		while (r2) {
			if (value_at(r2->entry, q.x)>q.y2) {
				r2 = r2->lc; continue;
			}
			if (r2->lc) cur_count += r2->lc->s;
			cur_count++;
			r2 = r2->rc; continue;
		}
		return cur_count;
	}	
	
	int query_count(const query_type& q, int cur_count) {
		seg_node* r = main_tree.root;
		if (!r) return cur_count;
		while (r) {
			if (q.x > get_key(r).first) { 
				seg_set t = r->entry.second.lmr; 
				cur_count = count_y_range(t.root, q, cur_count);
				r = r->rc;
				continue;
			} 
			if (q.x < get_key(r).first) {
				seg_set t = r->entry.second.rml;
				cur_count = count_y_range(t.root, q, cur_count);
				seg_type cur = get_key(r).second;
				if (cross(cur, q)) cur_count++;
				r = r->lc;
				continue;
			}
			break;
		}
		return cur_count;
	}
	
	void print_stats() {
		cout << "outer map: ";  seg_tree::GC::print_stats();
		cout << "Inner map: ";  seg_set::GC::print_stats();
	}


	seg_tree main_tree;
};
