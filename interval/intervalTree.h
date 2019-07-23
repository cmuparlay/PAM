#include "pam.h"
#include <stdio.h>
#include <vector>
#include <cstdio>
#include <iostream>
#include <algorithm>

using namespace std;

using point = int;
using par = pair<point,point>;

struct interval_map {
  
  using interval = pair<point, point>;

  struct entry {
    using key_t = point;
    using val_t = point;
    using aug_t = point;
    static inline bool comp(key_t a, key_t b) { return a < b;}
    static aug_t get_empty() { return 0;}
    static aug_t from_entry(key_t k, val_t v) { return v;}
    static aug_t combine(aug_t a, aug_t b) {
      return (a>b) ? a : b;}
  };

  using amap = aug_map<entry>;
  amap m;

  interval_map(size_t n) {
    amap::reserve(n); 
  }

  static void finish() {
    amap::finish();
  }

  interval_map(interval* A, size_t n) {
    m = amap(A,A+n); }

  bool stab(point p) {
    return (m.aug_left(p) > p);}

  void insert(interval i) {m.insert(i);}

  interval* report_all(point p) {
    amap a = amap::upTo(m, p);
    amap b = amap::aug_filter(a, [&](point a) {return a >= p;});
	interval* out = new interval[b.size()];
	amap::entries(b, out);
    return out; 
  }

};