#include <iostream>
#include <vector>
#include <pam/pam.h>
#include <pam/get_time.h>
#include <parlay/primitives.h>
#include "../common/sweep.h"
using namespace std;

timer init_tm, total_tm, sort_tm, reserve_tm;
size_t num_blocks = 0;

void reset_timers() {
  total_tm.reset();
  reserve_tm.reset();
}

#define x1 second.first
#define x2 second.second
#define y first

struct SegmentQuery {

  using entry_t = seg_type_2d;

  struct map_t {
    using key_t = seg_type_2d;
    static bool comp(key_t a, key_t b) { return a < b;}
  };

  using seg_map = pam_set<map_t>;

  parlay::sequence<seg_type_2d> end_points;
  parlay::sequence<seg_map> xt;
  size_t n;
  size_t n2;
  coord_t c_min, c_max;
	
  void set_min_max(coord_t _c_min, coord_t _c_max) {
    c_min = _c_min;
    c_max = _c_max;
  }

  SegmentQuery(parlay::sequence<seg_type_2d>& segs) {
    n = segs.size();
    n2 = 2*n;
		
    reserve_tm.start();
    seg_map::reserve(63*n);
    reserve_tm.stop();

    total_tm.start();
    end_points = parlay::sequence<seg_type_2d>::uninitialized(2*n);

    parlay::parallel_for (0, n, [&] (size_t i) {
      seg_type_2d s = segs[i];
      end_points[2*i] = s;
      std::swap(s.second.first, s.second.second);
      end_points[2*i+1] = s;
      });
		
    auto less = [] (seg_type_2d a, seg_type_2d b) {
      return (a.x1 < b.x1) || (a.x1 == b.x1 && a.y < b.y);};
		
    parlay::sort_inplace(end_points, less);

    using partial_t = pair<seg_map, seg_map>;

    auto insert = [&] (seg_map m, seg_type_2d p) {
      if (p.x1 < p.x2)
	return seg_map::insert(m, p);
      else return seg_map::remove(m, p);
    };

    auto build = [&] (seg_type_2d* s, seg_type_2d* e) {
      size_t n = e - s;
      vector<entry_t> add;
      vector<entry_t> remove;
      for (size_t i = 0; i < n; i++) {
	seg_type_2d p = s[i];
	if (p.x1 < p.x2) add.push_back(p);
	else remove.push_back(p);
      }
      return make_pair(seg_map(add.data(), add.data() + add.size()),
		       seg_map(remove.data(), remove.data() + remove.size()));
    };

    auto fold = [&] (seg_map m1, partial_t m2) {
      return seg_map::map_difference(seg_map::map_union(m1, std::move(m2.first)),
				     std::move(m2.second));
    };
    
    xt = sweep<partial_t>(end_points, seg_map(), insert, build, fold, num_blocks);

    total_tm.stop();
  }
	
  size_t get_index(const query_type q) {
    size_t l = 0, r = n2;
    size_t mid = (l+r)/2;
    while (l<r-1) {
      if (end_points[mid].x1 == q.x) break;
      if (end_points[mid].x1 < q.x) l = mid;
      else r = mid;
      mid = (l+r)/2;
    }
    return mid;
  }
	
  parlay::sequence<seg_type_2d> query_points(query_type q) {
    seg_map sm = xt[get_index(q)];
	if (q.y1 > q.y2) {
		coord_t t = q.y1; q.y1 = q.y2; q.y2 = t;
	}
    seg_type_2d left(q.y1, make_pair(0,0));
    seg_type_2d right(q.y2, make_pair(0,0));
	
    size_t m = sm.rank(right) - sm.rank(left);
    auto out = parlay::sequence<seg_type_2d>::uninitialized(m);
    seg_map::keys(seg_map::range(sm, left, right), out.begin());
    return out;
  }
	
  int query_sum(const query_type q) {
    int ind = get_index(q);
    seg_type_2d left(q.y1, make_pair(0,0));
    seg_type_2d right(q.y2, make_pair(0,0));
    //return xt[ind].rank(left, right);
	return (xt[ind].rank(right) - xt[ind].rank(left));
  }

  void print_allocation_stats() {
    cout << "allocation stats: " ;
    seg_map::GC::print_stats();
  }
  
  static void finish() {
    seg_map::finish();
  }


};
