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
typedef pair<point_type, point_type> rec_type;
struct query_type {
	int x, y;
};

inline int x1(rec_type& s) {return s.first.first;}
inline int y1(rec_type& s) {return s.first.second;}
inline int x2(rec_type& s) {return s.second.first;}
inline int y2(rec_type& s) {return s.second.second;}

void print_out(rec_type e) {
	cout << "[" << x1(e) << ", " << y1(e) << "; " << x2(e) << ", " << y2(e) << "]";
}

template<typename T>
void output_content(T t) {
	using ent = typename T::E;
	pbbs::sequence<ent> out(t.size());
	cout << t.size() << ": ";
	T::entries(t, out.begin());
	for (int i = 0; i < out.size(); i++) {
		print_out(out[i]);
		cout << " ";
	}
	cout << endl;
}


inline bool inside(data_type a, data_type b, data_type q) {
	return ((a<=q) && (q<=b));
}

inline bool in_rec(rec_type r, query_type p) {
	return ((inside(x1(r), x2(r), p.x)) && (inside(y1(r), y2(r), p.y)));
}


struct RectangleQuery {
	struct inner_entry {
		using key_t = rec_type; 
		static bool comp(key_t a, key_t b) { return y1(a)<y1(b);}
		using aug_t = data_type;
		static aug_t from_entry(key_t k) { 
			return y2(k); }
		static aug_t combine(aug_t a, aug_t b) {
			return (a > b) ? a : b;}
		static aug_t get_empty() { return 0;}
	};
	using interval_tree = aug_set<inner_entry>;
	using set_node = typename interval_tree::node;

	struct aug_sets {
		interval_tree first, second, lmr, rml;
		aug_sets(interval_tree _first, interval_tree _second) : first(_first), second(_second) {}
		aug_sets(interval_tree _first, interval_tree _second, interval_tree _lmr, interval_tree _rml) : first(_first), second(_second), lmr(_lmr), rml(_rml){}
		aug_sets() {}
	};
	
	struct outer_entry {
		using key_t = data_type;
		static bool comp(key_t a, key_t b) { return a < b;}
		using val_t = rec_type;
		using aug_t = aug_sets;
		static inline aug_t from_entry(key_t k, val_t v) { 
			if (k == x2(v)) 
				return aug_sets(interval_tree(v), interval_tree()); 
			else 
				return aug_sets(interval_tree(), interval_tree(v));
		}
		static inline aug_t combine(aug_t a, aug_t b) { 
			//cout << "---" << endl;
			//output_content(a.first); output_content(a.second);
			//output_content(b.first); output_content(b.second);
			interval_tree rml = interval_tree::map_difference(b.first, a.second);
			interval_tree l = interval_tree::map_union(a.first, rml);
			//output_content(rml); output_content(l);

			interval_tree lmr = interval_tree::map_difference(a.second, b.first);
			interval_tree r = interval_tree::map_union(b.second, lmr);
			//output_content(lmr); output_content(r);
			
			return aug_sets(l, r, lmr, rml);
		}
		static aug_t get_empty() { return aug_sets(interval_tree(), interval_tree());}
	};
	using rec_tree = aug_map<outer_entry>;
	
	using rec_node = typename rec_tree::node;
	using interval_node = typename interval_tree::node;
	
	using rec_entry = typename rec_tree::E;

	typename rec_tree::K
	get_key(rec_node* r) {return r->entry.first.first;}
	
	typename interval_tree::K
	get_key(interval_node* r) {return r->entry.first;}
	
	typename rec_tree::V
	get_val(rec_node* r) {return r->entry.first.second;}

    RectangleQuery(vector<rec_type>& recs) {
		construct(recs);
    }

    static void finish() {
      rec_tree::finish();
      interval_tree::finish();
    }
	
    void construct(vector<rec_type>& recs) {
        const size_t n = recs.size();
		reserve_tm.start();
		rec_tree::reserve(3*n);
		interval_tree::reserve(50*n);
		reserve_tm.stop();
		rec_entry *end_points = new rec_entry[2*n];
		total_tm.start();

		parallel_for (0, n, [&] (size_t i) {
			end_points[2*i] = make_pair(x1(recs[i]), recs[i]);
			end_points[2*i+1] = make_pair(x2(recs[i]), recs[i]);
		  });
	
        main_tree = rec_tree(end_points, end_points + (2*n));

		delete[] end_points;
		total_tm.stop();
    }

	template<typename OutIter>
	int query_points(const query_type& q, OutIter* out) {
		rec_node* r = main_tree.root;
		if (!r) return 0;
		auto f = [&] (interval_tree::A a) {return a>q.y;};
		rec_type yrec = make_pair(make_pair(0, q.y), make_pair(0, q.y));
		int total = 0;
		while (r) {
			//cout << get_key(r) << endl;
			if (q.x > get_key(r)) {
				interval_tree t = r->entry.second.lmr; 
				interval_tree tt = interval_tree::upTo(t, yrec);
				tt = interval_tree::aug_filter(tt, f);
				size_t s = tt.size();
				//cout << "t = " << t.size() << ", tt = " << s << endl;
				total+=s;
				//interval_tree::keys(tt,out);
				//out+=s;
				r = r->rc;
			}  else {
				if (q.x < get_key(r)) {
					interval_tree t = r->entry.second.rml;
					interval_tree tt = interval_tree::upTo(t, yrec);
					tt = interval_tree::aug_filter(tt, f);
					size_t s = tt.size();
					//cout << "t = " << t.size() << ", tt = " << s << endl;
					total+=s;
					r = r->lc;
				} else break;
			}
		}
		//cout << "total = " << total << endl;
		return total;
	}
	
	void print_stats() {
		cout << "outer map: ";  rec_tree::GC::print_stats();
		cout << "Inner map: ";  interval_tree::GC::print_stats();
	}
	
	void print_node(rec_node* r, string name) {
		cout << name << ": " << r->entry.first.first << endl;
		output_content(r->entry.second.first);
		output_content(r->entry.second.second);
		output_content(r->entry.second.lmr);
		output_content(r->entry.second.rml);
		cout << endl;
	}

	
	void print_root() {
		rec_node* r = main_tree.root;
		print_node(r, "root");
		//print_node(r->lc, "left");
		//print_node(r->rc, "right");
	}

	rec_tree main_tree;
};
