#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <parallel/algorithm>
#include <pam/pam.h>
#include <pam/get_time.h>
#include <parlay/random.h>
#include <parlay/primitives.h>

using namespace std;

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
  static void reserve(size_t n) {amap::reserve(2*n); }

  segment_map_1d(amap m) : m(m) {}
  segment_map_1d() {}

  segment_map_1d(seg_type s) { 
    endPoint P[2];
    P[0] = make_pair(s.first,true);
    P[1] = make_pair(s.second,false);
    m = amap(P,P+2);
  }
  segment_map_1d(parlay::sequence<seg_type> &A) {
    size_t n = A.size();
    auto P = parlay::sequence<endPoint>::uninitialized(2*n);
    parlay::parallel_for (0, n, [&] (size_t i) {
      P[2*i] = make_pair(A[i].first,true);
      P[2*i+1] = make_pair(A[i].second,false);
    });
    m = amap(P);
  }

  int count(coord_t p) {return m.aug_left(p);}

  void insert(seg_type i) {
    m.insert(make_pair(i.first,true));
    m.insert(make_pair(i.second,false));}

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

  struct map_2_t {
    using key_t = coord_t;
    using val_t = seg_type;
    using aug_t = segment_map_1d;
    static bool comp(key_t a, key_t b) { return a < b;}
    static aug_t get_empty() {return segment_map_1d();}
    static aug_t from_entry(key_t k, val_t v) {
      return segment_map_1d(v);}
    static aug_t combine(aug_t a, aug_t b) {return a.seg_union(b);}
  };

  using amap = aug_map<map_2_t>;
  //using anode = amap::node_type;
  amap m;
  static void reserve(size_t n) {
    amap::reserve(n); 
    //cout << (pbbs::log2_up(n)-2) << endl;
    segment_map_1d::reserve((parlay::log2_up(n)-2)*n);
    //segment_map_1d::reserve(*n);
  }
  
  segment_map_2d(parlay::sequence<seg_type_2d> &A) {
    m = amap(A);
  }

  static void finish() {
    amap::finish();
    segment_map_1d::finish();
  }

  struct sum_t {
    int x;
    int r;
    sum_t(int x) : x(x), r(0) {}
    void add_entry(seg_type_2d s) {
      if (s.second.first <= x && x <= s.second.second) x += 1;
    }
    void add_aug_val(segment_map_1d a) { r += a.count(x);}
  };

  int count(query_type s) {
    sum_t qc(s.x);
    m.range_sum(s.y1, s.y2, qc);
    return qc.r;
  }
  

};

void print_stats() {
  cout << "outer map: ";  segment_map_2d::amap::GC::print_stats();
  cout << "Inner map: ";  segment_map_1d::amap::GC::print_stats();
}
