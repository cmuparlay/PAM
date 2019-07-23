#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include "pam.h"
#include "pbbslib/get_time.h"
#include "../common/sweep.h"

using namespace std;

timer run_tm, reserve_tm, sort_tm, deconst_tm;

using coord = int;
using weight = float;

struct Point {
  coord x, y;
  weight w;
  Point() {}
  Point(coord x, coord y, weight w) : x(x), y(y), w(w) {}
};

struct RangeQuery {

  using entry_t = pair<coord, weight>;
  
  struct map_t {
    using key_t = coord;
    using val_t = weight;
    static bool comp(key_t a, key_t b) { return a < b;}
    using aug_t = weight;
    static aug_t get_empty() {return 0;}
    static aug_t from_entry(key_t k, val_t v) {return v;}
    static aug_t combine(aug_t a, aug_t b) {return a + b;}
  };

  using c_map = aug_map<map_t>;
  c_map* ts;
  coord* xs;
  size_t n;

  RangeQuery(vector<Point>& points) {
    n = points.size();
    Point* A = points.data();

    reserve_tm.start();
    c_map::reserve(36 * n);
    reserve_tm.stop();
    
    auto less = [] (Point a, Point b) {return a.x < b.x;};

    run_tm.start();
    pbbs::sample_sort(A,n,less);
    
    xs = pbbs::new_array<coord>(n);
    entry_t *ys = pbbs::new_array<entry_t>(n);
    parallel_for (0, n, [&] (size_t i) {
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

    ts = sweep<c_map>(ys, n, c_map(), insert, build, fold);

    pbbs::delete_array(ys, n);

    run_tm.stop();
  }

  int get_index(const coord q) {
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

  weight query(coord x1, coord y1, coord x2, coord y2) {
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
