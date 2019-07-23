#include <iostream>
#include <vector>
#include "pam.h"
#include "pbbslib/get_time.h"
#include "pbbslib/sample_sort.h"
#include "../common/sweep.h"
using namespace std;

timer init_tm, total_tm, sort_tm, reserve_tm;
size_t num_blocks = 0;

void reset_timers() {
  total_tm.reset();
  reserve_tm.reset();
}

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
	using e_type = pair<int, rec_type>;
	
  e_type* ev;
  interval_tree* xt;
  size_t n;
  size_t n2;

  RectangleQuery(vector<rec_type>& recs) {
    n = recs.size();
    n2 = 2*n;
		
    reserve_tm.start();
    interval_tree::reserve(63*n);
    reserve_tm.stop();

    total_tm.start();
	
    ev = pbbs::new_array<e_type>(2*n);

    parallel_for (0, n, [&] (size_t i) {
      ev[2*i].first = x1(recs[i]);
      ev[2*i].second = recs[i];
	  ev[2*i+1].first = x2(recs[i]);
      ev[2*i+1].second = recs[i];
      });
		
    auto less = [] (e_type a, e_type b) {
      return a.first<b.first;};
		
   // sort_tm.start();
    pbbs::sample_sort(ev, 2*n, less);
   // sort_tm.stop();

    using partial_t = pair<interval_tree, interval_tree>;

    auto insert = [&] (interval_tree m, e_type p) {
      if (p.first == x1(p.second))
	return interval_tree::insert(m, p.second);
      else return interval_tree::remove(m, p.second);
    };

    auto build = [&] (e_type* s, e_type* e) {
      size_t n = e - s;
      vector<rec_type> add;
      vector<rec_type> remove;
      for (int i = 0; i < n; i++) {
	e_type p = s[i];
	if (p.first == x1(p.second)) add.push_back(p.second);
	else remove.push_back(p.second);
      }
      return make_pair(interval_tree(add.data(), add.data() + add.size()),
		       interval_tree(remove.data(), remove.data() + remove.size()));
    };

    auto fold = [&] (interval_tree m1, partial_t m2) {
      return interval_tree::map_difference(interval_tree::map_union(m1, std::move(m2.first)),
				     std::move(m2.second));
    };
    
    xt = sweep<partial_t>(ev, n2, interval_tree(), insert, build, fold, num_blocks);

    total_tm.stop();
  }
	
  size_t get_index(const query_type& q) {
    size_t l = 0, r = n2;
    size_t mid = (l+r)/2;
    while (l<r-1) {
      if (ev[mid].first == q.x) break;
      if (ev[mid].first < q.x) l = mid;
      else r = mid;
      mid = (l+r)/2;
    }
    return mid;
  }
	
  pbbs::sequence<rec_type> query_points(const query_type& q) {
	//cout << "query: " << q.x << " " << q.y << endl;
    interval_tree it = xt[get_index(q)];
	//cout << "it: "; output_content(it);
	rec_type yrec = make_pair(make_pair(0, q.y), make_pair(0, q.y));
	interval_tree res = interval_tree::upTo(it, yrec);
	//cout << "res1: "; output_content(res);
	auto f = [&] (interval_tree::A a) {return a>q.y;};
	res = interval_tree::aug_filter(res, f);
	//cout << "res: "; output_content(res);
	size_t m = res.size();
	pbbs::sequence<rec_type> out(m);
	interval_tree::keys(res, out.begin());
    return out;
  }
  

  void print_allocation_stats() {
    cout << "allocation stats:" ;
    interval_tree::GC::print_stats();
  }
  
  static void finish() {
    interval_tree::finish();
  }


};
