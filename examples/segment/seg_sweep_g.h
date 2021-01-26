#include <iostream>
#include <vector>
#include <pam/pam.h>
#include <pam/get_time.h>
#include "../common/sweep.h"
using namespace std;

timer init_tm, total_tm, sort_tm, reserve_tm;
size_t num_blocks = 0;

void reset_timers() {
  total_tm.reset();
  reserve_tm.reset();
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
	
  parlay::sequence<e_type> ev;
  parlay::sequence<seg_set> xt;
  size_t n;
  size_t n2;

  SegmentQuery(parlay::sequence<seg_type>& segs) {
    n = segs.size();
    n2 = 2*n;
		
    reserve_tm.start();
    seg_set::reserve(63*n);
    reserve_tm.stop();

    total_tm.start();
	
    ev = parlay::sequence<e_type>::uninitialized(2*n);

    parallel_for (0, n, [&] (size_t i) {
	ev[2*i].first = x1(segs[i]);
	ev[2*i].second = segs[i];
	ev[2*i+1].first = x2(segs[i]);
	ev[2*i+1].second = segs[i];
      });
		
    auto less = [] (e_type a, e_type b) {
      return a.first<b.first;};
			
    parlay::sort_inplace(ev, less);	

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
    
    xt = sweep<partial_t>(ev, seg_set(), insert, build, fold, num_blocks);

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
	
  parlay::sequence<seg_type> query_points(const query_type& q) {
    seg_set sm = xt[get_index(q)];
	point_type lp = make_pair(q.x, q.y1);
	point_type rp = make_pair(q.x, q.y2);
	seg_type qy1 = make_pair(lp, lp);
	seg_type qy2 = make_pair(rp, rp);    
	size_t m = sm.rank(qy2) - sm.rank(qy1);
    parlay::sequence<seg_type> out(m);
    seg_set::keys(seg_set::range(sm, qy1, qy2), out.begin());
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
