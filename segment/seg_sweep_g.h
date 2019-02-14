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
typedef pair<point_type, point_type> seg_type;
struct query_type {
	int x, y1, y2;
};

template<typename T>
void shuffle(vector<T> v) {
    srand(unsigned(time(NULL)));
	int n = v.size();
    for (size_t i = 0; i < n; ++i) {
        size_t j = rand() % (n-i);
        swap(v[i], v[i+j]);
    }
}

inline data_type x1(seg_type& s) {
	return s.first.first;
}

inline data_type y1(seg_type& s) {
	return s.first.second;
}

inline data_type x2(seg_type& s) {
	return s.second.first;
}

inline data_type y2(seg_type& s) {
	return s.second.second;
}

double value_at(seg_type& s, int x) {
	if (x2(s) == x1(s)) return double(y1(s));
	double r1 = (double)((x-x1(s))+0.0)/(x2(s)-x1(s)+0.0)*(y1(s)+0.0);
	double r2 = ((x2(s)-x)+0.0)/(x2(s)-x1(s)+0.0)*(y2(s)+0.0);
	return r1+r2;
}

bool in_seg_x_range(seg_type& s, int p) {
	if ((x1(s) <= p) && (p <= x2(s))) return true;
	return false;
}

bool cross(seg_type& s, query_type q) {
	double cp;
	if (in_seg_x_range(s, q.x)) cp = value_at(s, q.x); else return false;
	if (q.y1 <= cp && cp <= q.y2) return true; else return false;
}

struct SegmentQuery {

	struct inner_entry {
		using key_t = seg_type; 
		static bool comp(key_t a, key_t b) { 
			if (x1(a)<=x1(b)) return (value_at(a, x1(b)) < y1(b));
			return (y1(a) < value_at(b, x1(a)));
		}
	};
	using seg_set = pam_set<inner_entry>;

	using e_type = pair<int, seg_type>;
	
  e_type* ev;
  seg_set* xt;
  size_t n;
  size_t n2;

  SegmentQuery(vector<seg_type>& segs) {
    n = segs.size();
    n2 = 2*n;
		
    reserve_tm.start();
    seg_set::reserve(63*n);
    reserve_tm.stop();

    total_tm.start();
	
    ev = pbbs::new_array<e_type>(2*n);

    parallel_for (0, n, [&] (size_t i) {
	ev[2*i].first = x1(segs[i]);
	ev[2*i].second = segs[i];
	ev[2*i+1].first = x2(segs[i]);
	ev[2*i+1].second = segs[i];
      });
		
    auto less = [] (e_type a, e_type b) {
      return a.first<b.first;};
		
   // sort_tm.start();
    pbbs::sample_sort(ev, 2*n, less);
   // sort_tm.stop();

    using partial_t = pair<seg_set, seg_set>;

    auto insert = [&] (seg_set m, e_type p) {
      if (p.first == x1(p.second))
	return seg_set::insert(m, p.second);
      else return seg_set::remove(m, p.second);
    };

    auto build = [&] (e_type* s, e_type* e) {
      size_t n = e - s;
      vector<seg_type> add;
      vector<seg_type> remove;
      for (int i = 0; i < n; i++) {
		e_type p = s[i];
		if (p.first == x1(p.second)) add.push_back(p.second);
		else remove.push_back(p.second);
	  }
	  return make_pair(seg_set(add.data(), add.data() + add.size()),
						seg_set(remove.data(), remove.data() + remove.size()));
    };

    auto fold = [&] (seg_set m1, partial_t m2) {
      return seg_set::map_difference(seg_set::map_union(m1, std::move(m2.first)),
				     std::move(m2.second));
    };
    
    xt = sweep<partial_t>(ev, n2, seg_set(), insert, build, fold, num_blocks);

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
	
  sequence<seg_type> query_points(const query_type& q) {
    seg_set sm = xt[get_index(q)];
	point_type lp = make_pair(q.x, q.y1);
	point_type rp = make_pair(q.x, q.y2);
	seg_type qy1 = make_pair(lp, lp);
	seg_type qy2 = make_pair(rp, rp);    
	size_t m = sm.rank(qy2) - sm.rank(qy1);
    sequence<seg_type> out(m);
    seg_set::keys(seg_set::range(sm, qy1, qy2), out.as_array());
    return out;
  }
	
  int query_sum(const query_type& q) {
    seg_set sm = xt[get_index(q)];
	point_type lp = make_pair(q.x, q.y1);
	point_type rp = make_pair(q.x, q.y2);
	seg_type qy1 = make_pair(lp, lp);
	seg_type qy2 = make_pair(rp, rp);    
	size_t m = sm.rank(qy2) - sm.rank(qy1);
	return m;
  }

  void print_allocation_stats() {
    cout << "allocation stats:" ;
    seg_set::GC::print_stats();
  }
  
  static void finish() {
    seg_set::finish();
  }


};
