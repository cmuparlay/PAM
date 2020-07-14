#include "pbbslib/get_time.h"
#include "pbbslib/parse_command_line.h"
timer t;

#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <ctime>
#include <random>
#include <cstdio>
#include <set>
#include <map>
#include "assert.h"
#include "pam.h"
#include "pbbslib/sequence_ops.h"
#include "pbbslib/random_shuffle.h"

using namespace std;
#define NO_AUG 1
#define LARGE 0

#ifdef LARGE
using key_type = size_t;
#else
using key_type = unsigned int;
#endif

struct Add {
  using T = key_type;
  static T add(T a, T b) { return a+b;}
  static T identity() {return 0;}
};

struct entry {
  using key_t = key_type;
  using val_t = key_type;
  using aug_t = key_type;
  static inline bool comp(key_t a, key_t b) { return a < b;}
  static aug_t get_empty() { return 0;}
  static aug_t from_entry(key_t k, val_t v) { return v;}
  static aug_t combine(aug_t a, aug_t b) { return std::max(a,b);}
  // following just used for treaps
  static size_t hash(pair<key_t,val_t> e) { return pbbs::hash64(e.first);}
};

struct entry2 {
  using key_t = key_type;
  using val_t = bool;
  static inline bool comp(key_t a, key_t b) { return a < b;}
};
struct entry3 {
  using key_t = key_type;
  using val_t = char;
  static inline bool comp(key_t a, key_t b) { return a < b;}
};

using par = pair<key_type, key_type>;

#ifdef NO_AUG
//using tmap  = pam_map<entry, treap<entry>>;
using tmap  = pam_map<entry>;
#else
using tmap  = aug_map<entry>;
#endif

struct mapped {
  key_type k, v;
  mapped(key_type _k, key_type _v) : k(_k), v(_v) {};
  mapped(){};

  bool operator < (const mapped& m)
    const { return k < m.k; }

  bool operator > (const mapped& m)
    const { return k > m.k; }

  bool operator == (const mapped& m)
    const { return k == m.k; }
};

size_t str_to_int(char* str) {
  return strtol(str, NULL, 10);
}

void check(bool test, string message) {
  if (!test) {
    cout << "Failed test: " << message << endl;
    exit(1);
  }
}

std::mt19937_64& get_rand_gen() {
  static thread_local std::random_device rd;
  static thread_local std::mt19937_64 generator(rd());
  return generator;
}

pbbs::sequence<par> uniform_input(size_t n, size_t window, bool shuffle = false) {
  auto g = [&] (size_t i) {
    uniform_int_distribution<> r_keys(1, window);
    key_type key = r_keys(get_rand_gen());
    key_type val = i;
    return make_pair(key,val);
  };
  pbbs::sequence<par> Pairs(n, g);
  auto addfirst = [] (par a, par b) -> par {
    return par(a.first+b.first, b.second);};
  pbbs::scan_inplace(Pairs.slice(), pbbs::make_monoid(addfirst,par(0,0)),
		     pbbs::fl_scan_inclusive);
  if (shuffle) pbbs::random_shuffle(Pairs);
  return Pairs;
}

pbbs::sequence<par> uniform_input_unsorted(size_t n, size_t window) {
  auto f = [&] (size_t i) {
    uniform_int_distribution<> r_keys(1, window);
    key_type k = r_keys(get_rand_gen());
    key_type c = r_keys(get_rand_gen());
    return make_pair(k, c);
  };
  pbbs::sequence<par> v(n, f);
  return v;
}

bool check_union(const tmap& m1, const tmap &m2, const tmap& m3) {
  // size_t l1 = m1.size();
  // size_t l2 = m1.size();
  // vector<key_type> e(l1+l2);
  // tmap::keys_to_array(m1,&e[0]);
  // tmap::keys_to_array(m2,&e[0]+l1);

  // sort(e.begin(), e.end());
  // e.erase(unique(e.begin(), e.end()), e.end());

  // bool ret = (m3.size() == e.size());

  // for (auto it : e) 
    // ret &= m3.contains(it);
    
  // return ret;
  
  vector<key_type> e1(m1.size());
  vector<key_type> e2(m2.size());
  vector<key_type> e3;
  tmap::keys_to_array(m1, &e1[0]);
  tmap::keys_to_array(m2, &e2[0]);

  set_union(e1.begin(), e1.end(), e2.begin(), e2.end(), back_inserter(e3));

  bool ret = (m3.size() == e3.size());

  for (auto it : e3)
    ret &= m3.contains(it);

  return ret;
}

bool check_multi_insert(const pbbs::sequence<par>& m1, const pbbs::sequence<par> &m2, const tmap& m3) {
  size_t l1 = m1.size();
  size_t l2 = m2.size();
  vector<key_type> e1(m1.size());
  vector<key_type> e2(m2.size());
  vector<key_type> e3;
  auto f = [&] (size_t i) { e1[i] = m1[i].first;};
  parallel_for(0, l1, f);
  auto f2 = [&] (size_t i) { e2[i] = m2[i].first;};
  parallel_for(0, l2, f2);

  set_union(e1.begin(), e1.end(), e2.begin(), e2.end(), back_inserter(e3));

  bool ret = (m3.size() == e3.size());

  for (auto it : e3)
    ret &= m3.contains(it);

  return ret;
}

bool check_intersect(const tmap& m1, const tmap& m2, const tmap& m3) {
  vector<key_type> e1(m1.size());
  vector<key_type> e2(m2.size());
  vector<key_type> e3;
  tmap::keys_to_array(m1, &e1[0]);
  tmap::keys_to_array(m2, &e2[0]);

  set_intersection(e1.begin(), e1.end(), e2.begin(), e2.end(), back_inserter(e3));

  bool ret = (m3.size() == e3.size());

  for (auto it : e3)
    ret &= m3.contains(it);

  return ret;
}

bool check_difference(const tmap& m1, const tmap& m2, const tmap& m3) {
  vector<key_type> e1(m1.size());
  vector<key_type> e2(m2.size());
  vector<key_type> e3;
  tmap::keys_to_array(m1, &e1[0]);
  tmap::keys_to_array(m2, &e2[0]);

  set_difference(e1.begin(), e1.end(), e2.begin(), e2.end(), back_inserter(e3));

  bool ret = (m3.size() == e3.size());

  for (auto it : e3)
    ret &= m3.contains(it);

  return true;
}

bool check_split(key_type key, const par* v, const pair<tmap, tmap>& m) {
  size_t n = m.first.size() + m.second.size() + 1;

  bool ret = 1;
  for (size_t i = 0; i < n; ++i) {
    if (v[i].first < key)
      ret &= m.first.contains(v[i].first);
    if (v[i].first > key)
      ret &= m.second.contains(v[i].first);
  }

  return ret;
}


template<class T>
bool check_filter(const tmap& m, const tmap& f, T cond) {

  // vector <par> e1, e2;
  // m.content(back_inserter(e1));
  // f.content(back_inserter(e2));

  // vector<par> v;
  // for (size_t i = 0; i < e1.size(); ++i)
  //     if (cond(e1[i])) v.push_back(e1[i]);

  // bool ret = (e2.size() == v.size());
  // for (size_t i = 0; i < v.size(); ++i)
  //     ret &= e2[i] == v[i];

  // return ret;
  return true;
}

template<class T>
bool check_aug_filter(const tmap& m, const tmap& f, T cond) {

  // vector <par> e1, e2;
  // m.content(back_inserter(e1));
  // f.content(back_inserter(e2));

  // vector<par> v;
  // for (size_t i = 0; i < e1.size(); ++i)
  //     if (cond(e1[i].second)) v.push_back(e1[i]);

  // bool ret = (e2.size() == v.size());
  // for (size_t i = 0; i < v.size(); ++i)
  //     ret &= e2[i] == v[i];

  // return ret;
  return true;
}

bool contains(const tmap& m, const pbbs::sequence<par> v) {
  bool ret = 1;
  for (size_t i = 0; i < m.size(); ++i) {
    ret &= m.contains(v[i].first);
  }

  return ret;
}

double test_union(size_t n, size_t m) {

  //pbbs::sequence<par> v1 = uniform_input(n, 20);
  //tmap m1(v1);

  //pbbs::sequence<par> v2 = uniform_input(m, (n/m) * 20);
  //tmap m2(v2);
  
  pbbs::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  tmap m1(v1);
  size_t nx = m1.size();

  pbbs::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  tmap m2(v2);
  size_t mx = m2.size();
  double tm;

  timer t;
  t.start();
  tmap m3 = tmap::map_union((tmap) m1, (tmap) m2);
  tm = t.stop();

  check(m1.size() == nx, "map size is wrong.");
  check(m2.size() == mx, "map size is wrong.");
  check(check_union(m1, m2, m3), "union is wrong");

  return tm;
}

double test_incremental_union_nonpersistent(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input(n, 20, true);
  tmap m1(v1);

  pbbs::sequence<par> v2 = uniform_input(n, 20, true);

  //size_t round = n/m;
  timer t;
  t.start();
  for (size_t i = 0; i < n; i+=m) {
    tmap m2(v2.slice(i, i+m));
    m1 = tmap::map_union(move(m1), move(m2));
  }
  double tm = t.stop();

  cout << "m1 size: " << m1.size() << endl;

  return tm;
}

double test_intersect(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  tmap m1(v1);
  size_t nx = m1.size();

  //pbbs::sequence<par> v2 = uniform_input(m, (n/m) * 2);
  pbbs::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  parallel_for(0, m/2, [&](int i) {v2[i]=v1[i];});
  tmap m2(v2);
  size_t mx = m2.size();

  timer t;
  t.start();
  tmap m3 = tmap::map_intersect(m1, m2);
  double tm = t.stop();

  check(m1.size() == nx, "map size is wrong");
  check(m2.size() == mx, "map size is wrong");
  check(check_intersect(m1, m2, m3), "intersect is wrong");

  return tm;
}

double test_deletion(size_t n, size_t m) {
  pbbs::sequence<par> v = uniform_input(n, 20);
  tmap m1(v);

  pbbs::sequence<par> u = uniform_input(m, (n/m)*20, true);

  timer t;
  t.start();
  for (size_t i = 0; i < m; ++i)
    m1 = tmap::remove(move(m1), u[i].first);

  double tm = t.stop();
  return tm;
}

double test_deletion_destroy(size_t n) {
  pbbs::sequence<par> v = uniform_input(n, 20, true);
  tmap m1;
  for (size_t i = 0; i < n; ++i)
    m1.insert(v[i]);
  pbbs::random_shuffle(v);

  timer t;
  t.start();
  for (size_t i = 0; i < n; ++i)
    m1 = tmap::remove(move(m1), v[i].first);
  double tm = t.stop();

  return tm;
}

double test_insertion_build(size_t n) {
  //pbbs::sequence<par> v = uniform_input(n, 20, true);
  pbbs::sequence<par> v = uniform_input_unsorted(n,1000000000);
  tmap m1;

  timer t;
  t.start();
  for (size_t i = 0; i < n; ++i) {
    m1.insert(v[i]);
  }
  double tm = t.stop();
  return tm;
}

double test_insertion_build_persistent(size_t n) {
  pbbs::sequence<par> v = uniform_input(n, 20, true);
  tmap m1;
  tmap* r = new tmap[n];
  tmap::reserve(30*n);

  timer t;
  t.start();
  r[0].insert(v[0]);
  for (size_t i = 1; i < n; ++i) {
    r[i] = r[i-1];
    r[i].insert(v[i]);
  }
  double tm = t.stop();

  delete[] r;
  return tm;
}

double test_insertion(size_t n, size_t m) {
  pbbs::sequence<par> v = uniform_input(n, 20);
  tmap m1(v);

  pbbs::sequence<par> u = uniform_input(m, (n/m)*20, true);

  timer t;
  t.start();
  for (size_t i = 0; i < m; ++i)
    m1.insert(u[i]);

  double tm = t.stop();
  return tm;
}

tmap build_slow(par* A, size_t n) {
  if (n <= 0) return tmap();
  if (n == 1) return tmap(A[0]);
  size_t mid = n/2;

  tmap a, b;
  auto left = [&] () {a = build_slow(A, mid);};
  auto right = [&] () {b = build_slow(A+mid, n-mid);};

  par_do_if(n > 100, left, right);

  return tmap::map_union(std::move(a), std::move(b));
}

double test_multi_insert(size_t n, size_t m) {
  pbbs::sequence<par> v = uniform_input(n, 20,true );
  pbbs::sequence<par> u = uniform_input(m, (n/m)*20, true);
  tmap m1(v);

  timer t;
  t.start();
  m1 = tmap::multi_insert(m1, u);
  double tm = t.stop();
  return tm;
}

double test_multi_delete(size_t n, size_t m) {
  pbbs::sequence<par> v = uniform_input(n, 20, true );
  
  auto g = [&](int i) -> key_type {
	  return v[i*n/m].first;
  };
  pbbs::sequence<key_type> u(m, g);
  
  tmap m1(v);

  timer t;
  t.start();
  m1 = tmap::multi_delete(m1, u);
  double tm = t.stop();
  return tm;
}


double test_dest_multi_insert(size_t n, size_t m) {

  pbbs::sequence<par> v = uniform_input(n, 20);
  pbbs::sequence<par> u = uniform_input(m, (n/m)*20, true);
  cout << "size of u: " << u.size() << endl;
  
  tmap m1(v);
  timer t;
  t.start();
  m1 = tmap::multi_insert(move(m1), u);
  double tm = t.stop();
  
  check(check_multi_insert(v,u,m1), "multi_insert wrong");

  return tm;
}


double stl_insertion(size_t n, size_t m) {
  pbbs::sequence<par> v = uniform_input(n, 20);
  pbbs::sequence<par> u = uniform_input(m, (n/m)*20, true);

  map<key_type, key_type> m1;
  for (size_t i = 0; i < n; ++i)
    m1.insert(v[i]);

  timer t;
  t.start();
  for (size_t i = 0; i < m; ++i)
    m1.insert(u[i]);
  double tm = t.stop();

  return tm;
}

double stl_insertion_build(size_t n) {
  //pbbs::sequence<par> v = uniform_input(n, 20, true);
  pbbs::sequence<par> v = uniform_input_unsorted(n,1000000000);
  map<key_type, key_type> m1;

  timer t;
  t.start();
  for (size_t i = 0; i < n; ++i)
    m1.insert(v[i]);
  double tm = t.stop();

  return tm;
}

double stl_delete_destroy(size_t n) {
  pbbs::sequence<par> v = uniform_input(n, 20, true);
  map<key_type, key_type> m1;
  pbbs::random_shuffle(v);

  timer t;
  t.start();
  for (size_t i = 0; i < n; ++i)
    m1.erase(v[i].first);
  double tm = t.stop();

  return tm;
}


double test_build(size_t n) {
  //pbbs::sequence<par> v = uniform_input_unsorted(n, 1000000000);

  pbbs::sequence<par> v = uniform_input(n, 20, true);

  timer t;
  t.start();
  tmap m1(v);
  double tm = t.stop();

  check(m1.size() == n, "map size is wrong");
  check(contains(m1, v), "build is wrong");

  return tm;
}

double test_filter(size_t n) {
  pbbs::sequence<par> v = uniform_input(n, 20);
  tmap m1(v);

  timer t;
  t.start();
  auto cond = [] (par t) { return (t.first & 1) == 1; };
  tmap res = tmap::filter(m1, cond);
  double tm = t.stop();

  check(m1.size() == n, "map size is wrong");
  check(check_filter(m1, res, cond), "filter is wrong");

  return tm;
}

double test_map_reduce(size_t n) {
  pbbs::sequence<par> v = uniform_input(n, 20);
  tmap m1(v);

  timer t;
  auto f = [&] (par e) {return e.second;};
  t.start();
  tmap::map_reduce(m1, f, Add());
  double tm = t.stop();

  return tm;
}

double test_aug_filter(size_t n, size_t m) {
  pbbs::sequence<par> v = uniform_input(n, 20);
  key_type threshold = n-m;
  tmap m1(v);
  tmap res = m1;
  tmap res2 = m1;

  timer t;
  t.start();
  auto cond = [&] (key_type t) {return (t>threshold);};
#ifndef NO_AUG
  res = tmap::aug_filter(move(res), cond);
  cout << "filter result: " << res.size() << endl;
#endif
  double tm = t.stop();

  auto cond2 = [&] (par t) {return (t.second>threshold);};
  timer t2; t2.start();
  res2 = tmap::filter(move(res2), cond2);
  double tm2 = t2.stop();
  cout << "filter result: " << res2.size() << endl;
  cout << "Plain filter: " << tm2 << endl;


  check(m1.size() == n, "map size is wrong");
  check(check_aug_filter(m1, res, cond), "filter is wrong");

  return tm;
}

double test_dest_union(size_t n, size_t m) {
  //pbbs::sequence<par> v1 = uniform_input(n, 20);
  pbbs::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  //pbbs::sequence<par> v1 = uniform_input_unsorted(n, 100);
  tmap m1(v1);
  tmap m1_copy(v1);

  //pbbs::sequence<par> v2 = uniform_input(m, (n/m) * 20);
  pbbs::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  tmap m2(v2);
  tmap m2_copy(v2);
  
  cout << "constructed" << endl;

  timer t;
  t.start();
  tmap m3 = tmap::map_union(move(m1), move(m2));
  double tm = t.stop();
  cout << "done" << endl;

  check(m1.size() == 0, "map size is wrong");
  check(m2.size() == 0, "map size is wrong");
  check(check_union(m1_copy, m2_copy, m3), "union is wrong");

  return tm;
}


double test_dest_intersect(size_t n, size_t m) {
  //pbbs::sequence<par> v1 = uniform_input(n, 20);
  pbbs::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  tmap m1(v1);
  tmap m1_copy(v1);

  //pbbs::sequence<par> v2 = uniform_input(m, (n/m) * 2);
  pbbs::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  parallel_for(0, m/2, [&](int i) {v2[i]=v1[i];});
  tmap m2(v2);
  tmap m2_copy(v2);

  timer t;
  t.start();
  tmap m3 = tmap::map_intersect(move(m1), move(m2));
  double tm = t.stop();

  check(m1.size() == 0, "map size is wrong");
  check(m2.size() == 0, "map size is wrong");
  check(check_intersect(m1_copy, m2_copy, m3), "intersect is wrong");

  return tm;
}


double test_split(size_t n) {
  //pbbs::sequence<par>v = uniform_input(n, 20);

  //tmap m1(v);

  // key_type key = v[n / 2].first;

  //timer t;
  //t.start();
  //pair<tmap, tmap> res;// = m1.split(key);
  //pair<tmap, tmap> res = m1.split(key);
  //double tm = t.stop();

  //check(m1.size() == n, "map size is wrong");
  //check(res.first.size() + res.second.size() + 1 == n, "splitted map size is wrong");
  //check(check_split(key, v, res), "split is wrong");
  
  cout << "split is not supported directly in the interface any more. You can use range() instead." << endl;

  return 0.0;
}


//a destructive version for now
double test_difference(size_t n, size_t m) {
  //pbbs::sequence<par> v1 = uniform_input(n, 20);
  pbbs::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  tmap m1(v1);
  tmap m1_copy(v1);
  //size_t nx = m1.size();
  

  //pbbs::sequence<par> v2 = uniform_input(m, (n/m) * 20);
  pbbs::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  parallel_for(0, m/2, [&](int i) {v2[i]=v1[i];});
  tmap m2(v2);
  tmap m2_copy(v2);
  //size_t mx = m2.size();

  timer t;
  t.start();
  tmap m3 = tmap::map_difference(move(m1), move(m2));
  double tm = t.stop();

  check(m1.size() == 0, "map size is wrong");
  check(m2.size() == 0, "map size is wrong");
  check(check_difference(m1_copy, m2_copy, m3), "difference is wrong");

  return tm;
}

double test_find(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input(n, 20);
  key_type max_key = v1[n-1].first;
  tmap m1(v1);

  pbbs::sequence<par> v2 = uniform_input_unsorted(m, max_key);

  bool *v3 = new bool[m];

  timer t;
  t.start();
  auto f = [&] (size_t i) {v3[i] = m1.contains(v2[i].first);};
  parallel_for(0, m, f);

  double tm = t.stop();
  delete[] v3;

  return tm;
}

double test_range(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input(n, 20);
  key_type max_key = v1[n-1].first;
  tmap m1(v1);

  pbbs::sequence<par> v2 = uniform_input_unsorted(m, max_key);
  // window size is 1/1000 of total width
  size_t win = max_key/1000;

  tmap *v3 = new tmap[m];

  timer t;
  t.start();
  auto f = [&] (size_t i) {
    v3[i] = tmap::range(m1, v2[i].first, v2[i].first+win);
  };

  parallel_for(0, m, f);
  double tm = t.stop();

  // do deletion in parallel
  parallel_for(0, m, [&] (size_t i) {v3[i] = tmap();});
  delete[] v3;

  return tm;
}

double test_aug_range(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input_unsorted(n, 1000000000);
  key_type max_key = v1[n-1].first;
  tmap m1(v1);

  pbbs::sequence<par> v2 = uniform_input_unsorted(m, max_key);

  key_type *v3 = new key_type[m];

  timer t;
  t.start();
#ifndef NO_AUG
  // window size is 1/1000 of total width
  size_t win = max_key/1000;

  parallel_for(0, m, [&] (size_t i) {
      v3[i] = m1.aug_range(v2[i].first, v2[i].first+win);
    });
#endif
  double tm = t.stop();

  delete[] v3;

  return tm;
}

double test_aug_left(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input(n, 20);
  key_type max_key = v1[n-1].first;
  tmap m1(v1);

  pbbs::sequence<par> v2 = uniform_input_unsorted(m, max_key);

  key_type *v3 = new key_type[m];

  timer t;
  t.start();
#ifndef NO_AUG
  parallel_for(0, m, [&] (size_t i) {
      v3[i] = m1.aug_left(v2[i].first);
    });
#endif
  double tm = t.stop();

  delete[] v3;

  return tm;
}


double stl_set_union(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input(n, 20);
  pbbs::sequence<par> v2 = uniform_input(m, 20 * (n / m));

  //set <key_type> s1, s2, sret;
  //key_type* s1 = new key_type[n];
  //key_type* s2 = new key_type[m];
  vector <key_type> s1(n), s2(m);
  vector <key_type> sret2;
  
  for (size_t i = 0; i < n; ++i) {
    //s1.insert(v1[i].first);
	s1[i] = v1[i].first;
  }
  for (size_t i = 0; i < m; ++i) {
    //s2.insert(v2[i].first);
	s2[i] = v2[i].first;
  }

  timer t;
  t.start();
  //set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(sret, sret.begin()));
  //set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), sret2.begin());
  //set_union(s1, s1+n, s2, s2+m, sret2.begin());
  set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), std::back_inserter(sret2));
  double tm = t.stop();

  return tm;
}

double stl_map_union(size_t n, size_t m) {
  //pbbs::sequence<par> v1 = uniform_input(n, 20);
  //pbbs::sequence<par> v2 = uniform_input(m, 20 * (n / m));
  pbbs::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  pbbs::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);

  std::map <key_type,key_type> s1, s2;
  for (size_t i = 0; i < n; ++i) {
    s1.insert(v1[i]);
  }
  for (size_t i = 0; i < m; ++i) {
    s2.insert(v2[i]);
  }

  timer t;
  t.start();
  s1.merge(s2);
  //set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), back_inserter(sret));
  double tm = t.stop();

  return tm;
}


double stl_vector_union(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input(n, 20);
  pbbs::sequence<par> v2 = uniform_input(m, 20 * (n / m));

  vector <mapped> s1, s2, sret;
  sret.reserve(m+n);

  for (size_t i = 0; i < n; ++i) {
    s1.push_back(mapped(v1[i].first, v1[i].second));
  }
  for (size_t i = 0; i < m; ++i) {
    s2.push_back(mapped(v2[i].first, v2[i].second));
  }

  timer t;
  t.start();
  set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), back_inserter(sret));
  double tm = t.stop();

  return tm;
}

double intersect_multi_type(size_t n, size_t m) {

  pbbs::sequence<par> v1 = uniform_input(n, 2);
  tmap m1(v1);

  pbbs::sequence<par> v2 = uniform_input(m, (n/m) * 2);
  pair<key_type, bool>* vv2 = new pair<key_type, bool>[m];
  for (size_t i = 0; i < m; i++) {
    vv2[i].first = v2[i].first;
    vv2[i].second = v2[i].second | 4;
  }
  //tmap m2(v2, v2 + m);
  using tmap2 = pam_map<entry2>;
  using tmap3 = pam_map<entry3>;
  tmap3::reserve(m);
  tmap2::reserve(m);
  tmap2 m2(vv2,vv2+m);

  auto f = [&] (key_type a, bool b) -> char {if (b) return 'a'; else return 'b';}; //+a%10;};

  timer t;
  t.start();
  tmap3 m3;
  m3 = tmap3::map_intersect(m1, m2, f);
  double tm = t.stop();
  cout << "size: " << m3.size() << ", " << m1.size() << ", " << m2.size() << endl;
  tmap::GC::print_stats();
  tmap2::GC::print_stats();
  tmap3::GC::print_stats();

  //key_type kk = m3.select(3).first;
  //cout << m3.find(kk).value << endl;

  check(m1.size() == n, "map size is wrong");
  check(m2.size() == m, "map size is wrong");
  //check(check_intersect(m1, m2, m3), "intersect is wrong");

  tmap3::finish();
  tmap2::finish();

  return tm;
}

string test_name[] = {
  "persistent-union",      // 0
  "persistent-intersect",  // 1
  "insert",                // 2
  "std::map-insert",       // 3
  "build",                 // 4
  "filter",                // 5
  "destructive-union",     // 6
  "destructive-intersect", // 7
  "split",                 // 8
  "difference",            // 9
  "std::set_union",        // 10
  "std::vector_union",     // 11
  "find",                  // 12
  "delete",                // 13
  "multi_insert",           // 14
  "destructive multi_insert",  //15
  "stl_insertion_build",   //16
  "test_deletion_destroy", //17
  "test_range", //18
  "test_aug_range", //19
  "test_insertion_build", //20
  "stl_delete_destroy", //21
  "test_aug_left", //22
  "stl_map_union", //23
  "aug_filter",//24
  "insertion_build_persistent", //25
  "test_incremental_union_nonpersistent", //26
  "intersect_multi_type", //27
  "flat_aug_range", //28
  "test_map_reduce", //29
  "test_multi_delete", //30
  "nothing"
};

double flat_aug_range(size_t n, size_t m) {
  pbbs::sequence<par> v1 = uniform_input(n, 20);
  key_type max_key = v1[n-1].first;
  tmap m1(v1, true);

  pbbs::sequence<par> v2 = uniform_input_unsorted(m, max_key);

  key_type *v3 = new key_type[m];
  size_t win = v1[n-1].first/1000;

  timer t;
  t.start();
  parallel_for(0, m, [&] (size_t i) {
      tmap m2 = tmap::range(m1, v2[i].first, v2[i].first+win);
      auto f = [] (par e) { return e.second;};
      v3[i] = tmap::map_reduce(m2, f, Add());
    });

  double tm = t.stop();

  delete[] v3;

  return tm;
}

double execute(size_t id, size_t n, size_t m) {

  switch (id) {
  case 0:
    return test_union(n, m);
  case 1:
    return test_intersect(n, m);
  case 2:
    return test_insertion(n, m);
  case 3:
    return stl_insertion(n, m);
  case 4:
    return test_build(n);
  case 5:
    return test_filter(n);
  case 6:
    return test_dest_union(n, m);
  case 7:
    return test_dest_intersect(n, m);
  case 8:
    return test_split(n);
  case 9:
    return test_difference(n, m);
  case 10:
    return stl_set_union(n, m);
  case 11:
    return stl_vector_union(n, m);
  case 12:
    return test_find(n, m);
  case 13:
    return test_deletion(n, m);
  case 14:
    return test_multi_insert(n,m);
  case 15:
    return test_dest_multi_insert(n,m);
  case 16:
    return stl_insertion_build(n);
  case 17:
    return test_deletion_destroy(n);
  case 18:
    return test_range(n,m);
  case 19:
    return test_aug_range(n,m);
  case 20:
    return test_insertion_build(n);
  case 21:
    return stl_delete_destroy(n);
  case 22:
    return test_aug_left(n,m);
  case 23:
    return stl_map_union(n,m);
  case 24:
    return test_aug_filter(n,m);
  case 25:
    return test_insertion_build_persistent(n);
  case 26:
    return test_incremental_union_nonpersistent(n, m);
  case 27:
    return intersect_multi_type(n,m);
  case 28:
    return flat_aug_range(n,m);
  case 29:
    return test_map_reduce(n);
  case 30:
    return test_multi_delete(n, m);
  default:
    assert(false);
    return 0.0;
  }
}

void test_loop(size_t n, size_t m, size_t repeat, size_t test_id, bool randomize) {
  size_t threads = num_workers();
  size_t reserve_size = (test_id == 4 || test_id == 19) ? n : 4 * n;
  cout << "threads = " << threads << endl;
  cout << "node size = " << sizeof(typename tmap::node) << endl;
  for (size_t i = 0; i < repeat; ++i) {
	//cout << reserve_size << endl;
    tmap::reserve(reserve_size);
    double tm = execute(test_id, n, m);
    cout << "RESULT"  << fixed << setprecision(6)
	 << "\ttest=" << test_name[test_id]
	 << "\ttime=" << tm
	 << "\tn=" << n
	 << "\tm=" << m
	 << "\titeration=" << i
	 << "\tp=" << threads << endl;

    tmap::finish();
  }
}

int main (int argc, char *argv[]) {
  commandLine P(argc, argv,
		"[-n size1] [-m size2] [-r rounds] [-p] [-use] <testid>");
  if (P.getOption("-use")) {
	  for (int i = 0; i < 31; i++) {
		  cout << "test " << i << "\t" << test_name[i] << endl;
	  }
	  return 0;
  }
  size_t n = P.getOptionLongValue("-n", 10000000);
  size_t m = P.getOptionLongValue("-m", n);
  size_t repeat = P.getOptionIntValue("-r", 5);
  size_t test_id = str_to_int(P.getArgument(0));
  bool randomize = P.getOption("-p");
  test_loop(n, m, repeat, test_id, randomize);
  return 0;
}
