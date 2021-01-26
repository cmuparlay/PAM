#include <algorithm>
#include <iostream>
#include <cstring>
#include <pam/pam.h>
#include <pam/get_time.h>
#include "../common/sweep.h"

using namespace std;

timer run_tm, reserve_tm, sort_tm, deconst_tm;

struct RangeQuery {

  using entry_t = pair<data_type, weight>;
  
  struct map_t {
    using key_t = data_type;
    using val_t = weight;
    static bool comp(key_t a, key_t b) { return a < b;}
    using aug_t = weight;
    static aug_t get_empty() {return 0;}
    static aug_t from_entry(key_t k, val_t v) {return v;}
    static aug_t combine(aug_t a, aug_t b) {return a + b;}
  };

  using c_map = aug_map<map_t>;
  parlay::sequence<c_map> ts;
  parlay::sequence<data_type> xs;
  size_t n;

  RangeQuery(parlay::sequence<point_type> &points) {
    n = points.size();
    point_type* A = points.data();

    reserve_tm.start();
    c_map::reserve(36 * n);
    reserve_tm.stop();
    
    auto less = [] (point_type a, point_type b) {return a.x < b.x;};

    run_tm.start();
    parlay::sort_inplace(points,less);
    
    xs = parlay::sequence<data_type>::uninitialized(n);
    auto ys = parlay::sequence<entry_t>::uninitialized(n);
    parlay::parallel_for (0, n, [&] (size_t i) {
      xs[i] = A[i].x;
      ys[i] = entry_t(A[i].y, A[i].w);
      });

    auto plus = [] (weight a, weight b) {return a + b;};
    
    auto insert = [&] (c_map m, entry_t a) {
      return c_map::insert(m, a, plus);
    };

    auto build = [&] (entry_t* s, entry_t* e) {
      return c_map(s,e,plus);
    };

    auto fold = [&] (c_map m1, c_map m2) {
      return c_map::map_union(m1, std::move(m2), plus);};

    ts = sweep<c_map>(ys, c_map(), insert, build, fold);

    run_tm.stop();
  }

  int get_index(const data_type q) {
    int l = 0, r = n;
    int mid = (l+r)/2;
    if (xs[0]>q) return -1;
    while (l<r-1) {
      if (xs[mid] == q) break;
      if (xs[mid] < q) l = mid;
      else r = mid;
      mid = (l+r)/2;
    }
    return mid;
  }

  weight query(data_type x1, data_type y1, data_type x2, data_type y2) {
    int l = get_index(x1);
    int r = get_index(x2);
    weight left = (l<0) ? 0.0 : ts[l].aug_range(y1,y2);
    weight right = (r<0) ? 0.0 : ts[r].aug_range(y1,y2);
    return right-left;
  }

  static void print_allocation_stats() {
    cout << "allocation stats:" ;  c_map::GC::print_stats();
  }

  static void finish() {
    c_map::GC::finish();
  }
};
