#include <algorithm>
#include <iostream>
#include <cstring>
#include <pam/pam.h>
#include <parlay/random.h>
#include <parlay/primitives.h>
using namespace std;

timer init_tm, build_tm, total_tm, aug_tm, sort_tm, reserve_tm, outmost_tm, globle_tm, g_tm, extra_tm;

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
			  return aug_sets(interval_tree({v}), interval_tree()); 
			else 
			  return aug_sets(interval_tree(), interval_tree({v}));
		}
		static inline aug_t combine(aug_t a, aug_t b) { 
			interval_tree rml = interval_tree::map_difference(b.first, a.second);
			interval_tree l = interval_tree::map_union(a.first, rml);

			interval_tree lmr = interval_tree::map_difference(a.second, b.first);
			interval_tree r = interval_tree::map_union(b.second, lmr);
			
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

    RectangleQuery(parlay::sequence<rec_type>& recs) {
		construct(recs);
    }

    static void finish() {
      rec_tree::finish();
      interval_tree::finish();
    }
	
    void construct(parlay::sequence<rec_type>& recs) {
        const size_t n = recs.size();
	reserve_tm.start();
	rec_tree::reserve(3*n);
	interval_tree::reserve(50*n);
	reserve_tm.stop();
	auto end_points = parlay::sequence<rec_entry>::uninitialized(2*n);
	total_tm.start();

	parlay::parallel_for (0, n, [&] (size_t i) {
          end_points[2*i] = make_pair(x1(recs[i]), recs[i]);
	  end_points[2*i+1] = make_pair(x2(recs[i]), recs[i]);
	});
	
	main_tree = rec_tree(end_points);

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
			if (q.x > get_key(r)) {
				interval_tree t = r->entry.second.lmr; 
				interval_tree tt = interval_tree::upTo(t, yrec);
				tt = interval_tree::aug_filter(tt, f);
				size_t s = tt.size();
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
					total+=s;
					r = r->lc;
				} else break;
			}
		}
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
