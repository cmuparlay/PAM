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

using coord_t = int;
using weight_t = int;

struct segment_t {coord_t x1, x2, y; weight_t w;};
struct query_t { coord_t x, y1, y2;};

struct mkey_t {
  coord_t y, x1, x2;
  mkey_t() {}
  mkey_t(coord_t y, coord_t x1, coord_t x2) : y(y), x1(x1), x2(x2) {}
  const bool operator < (const mkey_t &b) const {
    return y < b.y || (y == b.y && x1 < b.x1);}
};

struct SegmentQuery {

  using entry_t = pair<mkey_t, weight_t>;

  struct map_t {
    using key_t = mkey_t;
    using val_t = weight_t;
    static bool comp(key_t a, key_t b) { return a < b;}
    using aug_t = weight_t;
    static aug_t get_empty() {return 0;}
    static aug_t from_entry(key_t k, val_t v) {return v;}
    static aug_t combine(aug_t a, aug_t b) {return a + b;}
  };

  using seg_map = aug_map<map_t>;

  segment_t* end_points;
  seg_map* xt;
  size_t n;
  size_t n2;
  coord_t c_min, c_max;
	
  void set_min_max(coord_t _c_min, coord_t _c_max) {
    c_min = _c_min;
    c_max = _c_max;
  }

  SegmentQuery(vector<segment_t>& segs) {
    n = segs.size();
    n2 = 2*n;
		
    reserve_tm.start();
    seg_map::reserve(63*n);
    reserve_tm.stop();

    total_tm.start();
   // init_tm.start();
    end_points = pbbs::new_array<segment_t>(2*n);

    parallel_for (0, n, [&] (size_t i) {
      segment_t s = segs[i];
      end_points[2*i] = s;
      std::swap(s.x1, s.x2);
      end_points[2*i+1] = s;
      });
 //   init_tm.stop();
		
    auto less = [] (segment_t a, segment_t b) {
      return (a.x1 < b.x1) || (a.x1 == b.x1 && a.y < b.y);};
		
   // sort_tm.start();
    pbbs::sample_sort(end_points, 2*n, less);
   // sort_tm.stop();

    using partial_t = pair<seg_map, seg_map>;

    auto insert = [&] (seg_map m, segment_t p) {
      if (p.x1 < p.x2)
	return seg_map::insert(m, make_pair(mkey_t(p.y, p.x1, p.x2), p.w));
      else return seg_map::remove(m, mkey_t(p.y, p.x2, p.x1));
    };

    auto build = [&] (segment_t* s, segment_t* e) {
      size_t n = e - s;
      vector<entry_t> add;
      vector<entry_t> remove;
      for (int i = 0; i < n; i++) {
	segment_t p = s[i];
	if (p.x1 < p.x2) add.push_back(make_pair(mkey_t(p.y, p.x1, p.x2), p.w));
	else remove.push_back(make_pair(mkey_t(p.y, p.x2, p.x1), p.w));
      }
      return make_pair(seg_map(add.data(), add.data() + add.size()),
		       seg_map(remove.data(), remove.data() + remove.size()));
    };

    auto fold = [&] (seg_map m1, partial_t m2) {
      return seg_map::map_difference(seg_map::map_union(m1, std::move(m2.first)),
				     std::move(m2.second));
    };
    
    xt = sweep<partial_t>(end_points, n2, seg_map(), insert, build, fold, num_blocks);

    total_tm.stop();
  }
	
  size_t get_index(const query_t& q) {
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
	
  pbbs::sequence<mkey_t> query_points(const query_t& q) {
    seg_map sm = xt[get_index(q)];
    mkey_t left(q.y1,0,0);
    mkey_t right(q.y2,0,0);
    if (q.y1 > q.y2) std::swap(left,right);
    size_t m = sm.rank(right) - sm.rank(left);
    pbbs::sequence<mkey_t> out(m);
    seg_map::keys(seg_map::range(sm, left, right), out.begin());
    return out;
  }
	
  weight_t query_sum(const query_t& q) {
    int ind = get_index(q);
    mkey_t left(q.y1,0,0);
    mkey_t right(q.y2,0,0);
    return xt[ind].aug_range(left, right);
  }

  void print_allocation_stats() {
    cout << "allocation stats:" ;
    seg_map::GC::print_stats();
  }
  
  static void finish() {
    seg_map::finish();
  }


};
