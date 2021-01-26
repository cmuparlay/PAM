#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <parallel/algorithm>
#include <pam/pam.h>
#include <pam/get_time.h>
#include <parlay/random.h>
#include <parlay/primitives.h>

#include "../common/sweep.h"

using namespace std;

timer reserve_tm, build_tm, query_tm, total_tm;
size_t num_blocks = 0;

struct segment_map_1d {
  using endPoint = pair<coord_t, bool>;

  struct map_t {
    using key_t = coord_t;
    using val_t = bool;
    static bool comp(key_t a, key_t b) { return a < b;}
    using aug_t = int;
    static aug_t get_empty() {return 0;}
    static aug_t from_entry(coord_t k, bool v) {return v ? 1 : -1;}
    static aug_t combine(aug_t a, aug_t b) {return a + b;}
  };

  using amap = aug_map<map_t>;
  amap m;
  static void reserve(size_t n) {
    cout << n << endl;
    amap::reserve(60*n); }

  segment_map_1d(amap m) : m(m) {}
  segment_map_1d() {m=amap();}
  segment_map_1d(seg_type s) { 
    endPoint P[2];
    P[0] = make_pair(s.first,true);
    P[1] = make_pair(s.second,false);
    m = amap(P,P+2);
  }
  segment_map_1d(parlay::sequence<seg_type> &A) {
    size_t n = A.size();
    auto P = parlay::sequence<endPoint>::uninitialized(2*n);
    for (size_t i =0; i < n ;i++) {
      P[2*i] = make_pair(A[i].first,true);
      P[2*i+1] = make_pair(A[i].second,false);
    }
    m = amap(P);
  }

  int count(coord_t p) {return m.aug_left(p);}

  segment_map_1d insert(seg_type i) {
    amap m2 = m;
    m2.insert(make_pair(i.first,true));
    m2.insert(make_pair(i.second,false));
    return segment_map_1d(m2);
  }

  segment_map_1d seg_union(segment_map_1d b) {
    return segment_map_1d(amap::map_union(m,b.m));}

  static void finish() {amap::finish();}
};

template<typename T>
void output_content(T t) {
  vector<pair<coord_t,bool>> out;
  t.content(std::back_inserter(out));
  for (int i = 0; i < out.size(); i++) cout << out[i].first << " ";
}

struct segment_map_2d {
	
  using e_type = seg_type_2d;

  parlay::sequence<seg_type_2d> ev;
  parlay::sequence<segment_map_1d> xt;
  size_t n;

  segment_map_2d(parlay::sequence<seg_type_2d> &A) { 
    n = A.size();
    ev = A;
	
    reserve_tm.start();
    segment_map_1d::reserve(n);
    reserve_tm.stop();

    total_tm.start();

    auto less = [] (e_type a, e_type b) {
      return a.first<b.first;};
		
    // sort_tm.start();
    parlay::sort(ev, less);
    // sort_tm.stop();
	
    auto insert = [&] (segment_map_1d m, e_type p) {
		return m.insert(p.second);
    };

    auto build = [&] (e_type* s, e_type* e) {
      size_t nx = e - s;
      //auto x = parlay::sequence<segment_1d>::uninitialized(nx);
      auto x = parlay::tabulate(nx, [&] (size_t i) -> seg_type {
    	  return s[i].second;});
      return segment_map_1d(x);
    };

    auto fold = [&] (segment_map_1d m1, segment_map_1d m2) {
      return m1.seg_union(m2);
    };
    
    xt = sweep<segment_map_1d>(ev, segment_map_1d(), insert, build, fold, num_blocks);
	
    cout << "built" << endl;
    total_tm.stop();
  }

  static void finish() {
    segment_map_1d::finish();
  }
  
  size_t get_index(const coord_t x) {
    size_t l = 0, r = n;
    size_t mid = (l+r)/2;
    while (l<r-1) {
      if (ev[mid].first == x) break;
      if (ev[mid].first < x) l = mid;
      else r = mid;
      mid = (l+r)/2;
    }
    return mid;
  }

  int count(query_type s) {
    segment_map_1d ml = xt[get_index(s.y1)];
    segment_map_1d mr = xt[get_index(s.y2)];
    int xl = ml.m.aug_left(s.x);
    int xr = mr.m.aug_left(s.x);
    return xr-xl;
  }

};

void print_stats() {
  //cout << "outer map: ";  segment_map_2d::amap::GC::print_stats();
  cout << "Inner map: ";  segment_map_1d::amap::GC::print_stats();
}
