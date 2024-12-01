#include <iostream>
#include <algorithm>

#include <pam/pam.h>
#include <parlay/random.h>

#include "../examples/index/index.h"

using namespace std;

struct entry {
  using key_t = int;
  using val_t = int;
  using aug_t = float;
  static inline bool comp(key_t a, key_t b) { return a < b;}
  static aug_t get_empty() { return 0;}
  static aug_t from_entry(key_t k , val_t v) { return v/2.0;}
  static aug_t combine(aug_t a, aug_t b) { return a + b;}
  // following just used for treaps
  static size_t hash(pair<key_t,val_t> e) { return parlay::hash64(e.first);}
};

struct entry_max {
  using key_t = int;
  using val_t = float;
  using aug_t = int;
  static inline bool comp(key_t a, key_t b) { return a < b;}
  static aug_t get_empty() { return 0;}
  static aug_t from_entry(key_t k, val_t v) { return (int) v;}
  static aug_t combine(aug_t a, aug_t b) { return std::max(a,b);}
};

struct entry2 {
  using key_t = int;
  using val_t = bool;
  static inline bool comp(key_t a, key_t b) { return a < b;}
};
struct entry3 {
  using key_t = int;
  using val_t = char;
  static inline bool comp(key_t a, key_t b) { return a < b;}
};


using elt = pair<int,int>;
using map  = aug_map<entry>;

using eltm = pair<int,float>;
using map_max = aug_map<entry_max>;

using elt2 = pair<int, bool>;
using map2 = pam_map<entry2>;

using elt3 = pair<int, char>;
using map3 = pam_map<entry3>;


void check(bool test, string message) {
  if (!test) {
    cout << "Failed test: " << message << endl;
    exit(1);
  }
}

void test_aug() {
  eltm a[5] = {eltm(2,4.0), eltm(4,5.0), eltm(6,8.0),
	       eltm(9, 1.0), eltm(11, 12.0)};

  map_max ma(a,a+5);
  auto gt = [] (float x) {return x > 7.0;};
  map_max mb = map_max::aug_filter(ma, gt);
  check(mb.size() == 2, "aug_filter size check");
  check(ma.size() == 5, "aug_filter size check 2");
  check(*mb.find(11) == 12.0, "aug_filter check 11");
  check(!mb.find(4), "aug_filter check 4");
}

template <class map>
void test_map(int balance_type) {
  char names[4][10] = {"WB", "RB", "treap", "AVL"};
  cout << "testing: " << names[balance_type] << endl;

  parlay::sequence<elt> a = {elt(2,4), elt(4,5), elt(6,8)};

  map ma(a);
  check(ma.size() == 3, "size check ma");

  check(*ma.find(4) == 5, "find check 4");
  check(!ma.find(3), "find check 3");

  check(*ma.next(3) == elt(4,5), "next check 3");
  check(*ma.next(4) == elt(6,8), "next check 4");
  check(!ma.next(6), "next check 6");
  check(*ma.previous(3) == elt(2,4), "previous check 3");
  check(*ma.previous(4) == elt(2,4), "previous check 4");
  check(!ma.previous(2), "previous check 2");

  check(ma.rank(4) == 1, "rank test 4");
  check(ma.rank(5) == 2, "rank test 5");
  check(ma.rank(7) == 3, "rank test 5");
  check(*(ma.select(0)) == elt(2,4), "select test 0");
  check(*(ma.select(2)) == elt(6,8), "select test 2");

  check(ma.aug_val() == 8.5, "aug test");
  check(ma.aug_left(4) == 4.5, "aug left test");
  check(ma.aug_right(4) == 6.5, "aug right test");
  check(ma.aug_range(3,11) == 6.5, "aug range test");
  check(ma.aug_range(3,5) == 2.5, "aug range test 2");
  check(ma.aug_range(0,5) == 4.5, "aug range test 3");
  
  elt2 b2[4] = {elt2(2, true), elt2(6, false), elt2(8, false), elt2(17, true)};
  map2 mb2(b2, b2+4);
  map3 mc2;
  auto f = [&] (int a, bool b) -> char {if (b) return 'a'; else return 'b'+a%10;};
  mc2 = map3::map_intersect(ma, mb2, f);
  check(mc2.size() == 2, "intersection size");
  check(mc2.find(2).value() == 'a', "check intersection result 2");
  check(mc2.find(6).value() == 'j', "check intersection result 6");
  mb2.clear(); mc2.clear();

  ma = map::insert(move(ma), elt(9,11));
  // ma == {elt(2,4), elt(4,5), elt(6,8), elt(9,11)};
  check(ma.size() == 4, "size check insert");
  check(*ma.find(9) == 11, "find check insert");
  
  ma.insert(elt(4,12));  // insert with overwrite
  // ma == {elt(2,4), elt(4,12), elt(6,8), elt(9,11)};
  check(ma.size() == 4, "size check insert second");
  check(*ma.find(4) == 12, "find check insert second");

  auto add = [] (int a, int b) {return a + b;};
  ma = map::insert(move(ma), elt(6,5), add);  // insert with sum
  // ma == {elt(2,4), elt(4,12), elt(6,13), elt(9,11)};
  check(ma.size() == 4, "size check insert with sum");
  check(*ma.find(6) == 13, "find check insert with sum");

  parlay::sequence<elt> b = {elt(1,6), elt(4,3)};

  map *mb_p = new map(b);
  check(mb_p->size() == 2, "size check mb");
  check(*(mb_p->find(4)) == 3, "find check mb");

  map mc = map::map_union(ma,*mb_p);
  // mc == {elt(1,6), elt(2,4), elt(4,12), elt(6,13), elt(9,11)};
  check(mc.size() == 5, "size check union");
  check(*ma.find(4) == 12, "find check union");

  elt r[6];
  r[5] = elt(0,0);
  map::entries(mc, r);
  check(r[0] == elt(1,6) && r[1] == elt(2,4) && r[4] == elt(9,11)
	&& r[5] == elt(0,0),
	"check entries");
  
  auto mult = [] (int a, int b) { return a*b;};
  
  mc = map::map_union(ma,*mb_p,mult);
  // mc == {elt(1,6), elt(2,4), elt(4,36), elt(6,13), elt(9,11)};
  check(mc.size() == 5, "size check union with mult");
  check(*mc.find(4) == 36, "find check union with mult");

  auto is_odd = [] (elt a) { return (bool) (a.first & 1);};

  map md = map::range(mc,2,4);
  // md == {elt(2,4), elt(4,36)};
  check(md.size() == 2, "size check after range");
  check(md.aug_val() == 20.0, "aug test after range");

  md = map::range(mc,0,7);
  // md == {elt(1,6), elt(2,4), elt(4,36), elt(6,13)};
  check(md.size() == 4, "size check after range 2");
  check(md.aug_val() == 29.5, "aug test after range 2");
  md.clear();

  // mc == {elt(1,6), elt(2,4), elt(4,36), elt(6,13), elt(9,11)};
  mc = map::filter(move(mc),is_odd);
  // mc == {elt(1,6), elt(9,11)};
  check(mc.size() == 2, "size check after filter");
  check(mc.aug_val() == 8.5, "aug test after filter");

  mc = map::map_intersect(*mb_p, ma),
  // mc == {elt(4,12)};
  check(mc.size() == 1, "size check intersect");
  check(*mc.find(4) == 12, "find check intersect");

  mc = map::map_intersect(ma,*mb_p,mult);
  // mc == {elt(4,36)};
  check(*mc.find(4) == 36, "find check intersect with mult");

  // ma == {elt(2,4), elt(4,12), elt(6,13), elt(9,11)};
  // mb_p == {elt(1,6), elt(4,3)};
  mc = map::map_difference(ma,*mb_p);
  // mc == {elt(2,4), elt(6,13), elt(9,11)};
  check(mc.size() == 3, "size check difference");
  check(mc.contains(2), "find check after diff 2");
  check(mc.contains(6), "find check after diff 6");
  check(!mc.find(4), "find check after diff 4");

  auto g = [] (pair<int,int> e) -> float {return e.second + 1.0;};

  map_max mx = map_max::map(mc, g);
  // mx == {eltm(2,5.0), eltm(6,14.0), eltm(9,12.0)};
  check(mx.size() == 3, "size check after map");
  check(*mx.find(2) == 5.0, "find check after map 2");
  check(mx.aug_val() == 14, "aug_val check after map");

  mc.clear();
  check(mc.size() == 0, "size check after clear");

  check(map::GC::num_used_nodes() == 4 + 2, "used nodes"); 

  mc = map::map_union(std::move(*mb_p),std::move(ma)),
  // mc == {elt(1,6), elt(2,4), elt(4,12), elt(6,13), elt(9,11)};
  check(mc.size() == 5, "size check union move");
  check(ma.size() == 0, "size check ma union move");
  check(mb_p->size() == 0, "size check mb union move");
  check(map::GC::num_used_nodes() == 5, "used nodes after move");

  mc = map::remove(move(mc), 2);
  // mc == {elt(1,6), elt(4,12), elt(6,13), elt(9,11)};
  check(mc.size() == 4, "size check remove 2");
  check(!mc.find(2), "find check remove 2");

  mc = map::remove(move(mc),3);
  // mc == {elt(1,6), elt(4,12), elt(6,13), elt(9,11)};
  check(mc.size() == 4, "size check remove 3");
  check(mc.aug_val() == 21, "aug test after remove 3");
  check(map::GC::num_used_nodes() == 4, "used nodes after remove 3");


  mc.clear();
  check(map::GC::num_used_nodes() == 0, "used nodes after clear");
  md.clear();
  for (size_t i = 0; i < 100; ++i) {
    ma = map::insert(move(ma),make_pair(i, i));
    if (i < 30) mc = map::insert(move(mc), make_pair(i, i));
    else md = map::insert(move(md), make_pair(i, i));
  }

  check(ma.size() == 100, "size check for ma insert");
  check(ma == map::map_union(std::move(mc), std::move(md)), 
    "check map equality");
  check(ma.find(50).has_value(), "check ma contains 50");
  check(!ma.find(100), "check ma does not contain 100");
  
  ma = map::remove(move(ma), 50);
  check(ma.size() == 99, "size check for ma after delete");
  check(!ma.find(50), "check ma does not contain 50 after delete");

  delete mb_p;
  
  //added a more complicated union query
  parlay::sequence<elt> x(70);
  parlay::sequence<elt> y(40);
  parlay::sequence<elt> input_elt(100);
  for (int i = 0; i < 100; i++) input_elt[i] = elt(i, i);
  parlay::random_shuffle(input_elt);
  for (int i = 0; i < 70; i++) x[i] = input_elt[i];
  for (int i = 0; i < 40; i++) y[i] = input_elt[i+60];
  
  map mapx(x);
  map mapy(y);
  map mapz = map::map_union(mapx, mapy);
  check(mapz.size() == 100, "size check for mapz (should be 100)");
  for (int i = 0; i< 100; i++) check(mapz.find(i).has_value(), "check all 100 elements");
  check(mapz.check_balance(), "z is balanced");
}    

void test_map_reserve_finish() {
  size_t n = 1000;
  for (size_t i = 0; i < 10; i++) {
    map::reserve(20);
    {
      auto a = parlay::tabulate(n, [&] (size_t i) -> elt {
	  return elt(n*i + rand()%n,0);});
      auto b = parlay::tabulate(n, [&] (size_t i) -> elt {
	  return elt(n*i + rand()%n,0);});
      map ma(a);
      map mb(b);
      check(ma.size() == n, "size check map reserve");
      check(mb.size() == n, "size check map reserve");
    }
    map::finish();
  }
} 

void test_map_more() {

  { // equality test
    parlay::sequence<elt> a = {elt(2,4), elt(4,5), elt(6,8)};
    parlay::sequence<elt> b = {elt(2,4), elt(4,5)};    
    map ma(a);
    map mb(b);
    check (!(ma == mb), "equality check, not equal");
    ma = map::remove(move(ma),6);
    check (ma == mb, "equality check, equal");
  }

  { // join2 test
    parlay::sequence<elt> a = {elt(2,4), elt(4,5), elt(6,8)};
    parlay::sequence<elt> b = {elt(9,0), elt(11,7)};    
    map ma(a);
    map mb(b);
    map mc = map::join2(ma,mb);
    check (mc.size() == 5, "join2 size check");
    check (*mc.find(6) == 8, "join2 val test 6");
    check (*mc.find(9) == 0, "join2 val test 9");
  }

  { // map_reduce test
    struct Add {
      using T = long;
      static T identity() { return 0;}
      static T add(T a, T b) { return a + b;}
    };

    parlay::sequence<elt> a = {elt(2,4), elt(4,5), elt(6,8), elt(9,1), elt(12, 2)};
    map ma(a);
    auto f = [&] (elt e) -> long { return (long) 2 * e.second;};
    long x = map::map_reduce(ma, f, Add());
    check (x == 40, "map_reduce test");
  }

  { // map_filter test
    parlay::sequence<elt> a = {elt(2,4), elt(4,5), elt(6,8), elt(9,1), elt(12, 2)};
    auto f = [&] (elt a) {
      return (a.first > 4) ? std::optional<int>{2*a.second} : std::nullptropt;
    };
    map ma(a);
    map mb = map::map_filter(ma,f);
    check (mb.size() == 3, "map_filter len test");
    check (*mb.find(6) == 16, "map_filter val test 6");
    check (*mb.find(12) == 4, "map_filter val test 12");
  }

  { // self union test
    parlay::sequence<elt> a = {elt(2,4), elt(4,5), elt(6,8)};
    map ma(a);
    map mb = map::map_union(ma,ma);
    check (mb.size() == 3, "self union test");
  }  

  { // self intersection test
    parlay::sequence<elt> a = {elt(2,4), elt(4,5), elt(6,8)};
    map ma(a);
    map mb = map::map_intersect(ma,ma);
    check (mb.size() == 3, "self intersection test");
  }  

  { // build combine test
    parlay::sequence<elt> a = {elt(2,4), elt(4,5), elt(2,6)};
    parlay::sequence<elt> b = {elt(2,10), elt(4,5)};
    map ma(a, [] (int a, int b) -> int {return a + b;});
    map mb(b);
    check (mb.size() == 2, "length check, combine test");
    check ((*ma.select(0)).second == (4 + 6), "value check, combine test");
    check (ma == mb, "equality check, combine test");
  }  

  check(map::GC::num_used_nodes() == 0, "used nodes at end of test_map_more");   
}

void test_index() {
  // Test index
  using index_elt = inv_index::index_elt;
  using post_elt = inv_index::post_elt;
  using post_list = inv_index::post_list;

  const size_t word_count = 12;
  const std::pair<string, size_t> words[] = { 
        {"apple", 0}, {"pie", 0}, {"delicious", 0}, 
        {"is", 1}, {"pumpkin", 1}, {"pie", 1}, {"tasty", 1} , 
        {"not", 2}, {"everyone", 2}, {"likes", 2}, {"apple", 2}, {"pie", 2}
  };

  auto KV = parlay::tabulate(word_count, [&] (size_t i) -> index_elt {
      return index_elt(parlay::to_sequence(words[i].first),
		       post_elt(words[i].second,0));
    });
    
  inv_index index(KV);

  auto list = [&] (string s) {
    return index.get_list(parlay::to_sequence(s));};

  post_list apple = list("apple");
  post_list pie = list("pie");
  post_list tasty = list("tasty");
  check(apple.size() == 2, "size check for apple");
  check(pie.size() == 3, "size check for pie");
  check(tasty.size() == 1, "size check for tasty");
    
  post_list res1 = std::move(inv_index::And(apple, pie));
  check(res1.size() == 2, "size check for And query result");
  check(res1.contains(0), "check if query result has document id 0");
  check(res1.contains(2), "check if query result has document id 2");

  post_list res2 = std::move(inv_index::Or(tasty, apple));
  check(res2.size() == 3, "size check for or query result");
  check(res2.contains(0), "check if or query result has document id 0");
  check(res2.contains(1), "check if or query result has document id 1");
  check(res2.contains(2), "check if or query result has document id 2");

  post_list res3 = std::move(inv_index::And_Not(pie, tasty));
  check(res3.size() == 2, "size check for or query result");
    
}

void test_intervals() {

  struct intervals {
    
    struct entry_t {
      using key_t = int;
      using val_t = float;
      using aug_t = float;
      static inline bool comp(key_t a, key_t b) { return a < b;}
      static aug_t get_empty() { return 0.0;}
      static aug_t from_entry(key_t k, val_t v) { return v;}
      static aug_t combine(aug_t a, aug_t b) {return a + b;}
    };

    using amap = aug_map<entry_t>;

    amap m;

    void insert(int left, int right, int w) {
      auto add = [&] (float a, float b) {return a + b;};
      m = amap::insert(m, make_pair(left, w), add);
      m = amap::insert(m, make_pair(right, -w), add);
    }

    float stab(int loc) {return m.aug_left(loc);}
  };

  intervals im;
  intervals::amap::init();
	
  im.insert(2, 8, 2.0);
  im.insert(2, 5, -1.0);
  im.insert(4, 10, 6.0);
  check(im.stab(1) == 0, "interval: stab(1)");
  check(im.stab(2) == 1, "interval: stab(2)");
  check(im.stab(3) == 1, "interval: stab(3)");
  check(im.stab(4) == 7, "interval: stab(4)");
  check(im.stab(5) == 8, "interval: stab(5)");
  check(im.stab(6) == 8, "interval: stab(6)");
  check(im.stab(8) == 6, "interval: stab(8)");
  check(im.stab(9) == 6, "interval: stab(9)");
  check(im.stab(10) == 0, "interval: stab(10)");
  check(im.stab(11) == 0, "interval: stab(11)");
}


void test_set () {
  struct set_entry {
    using key_t = int;
    static inline bool comp(key_t a, key_t b) { return a < b;}
  };
  using set = pam_set<set_entry>;
  int a[7] = {1, 4, 7, 11, 14, 20, 33};
  set sa(a,a+7);
  check(sa.size() == 7, "set size check sa");

  check(sa.contains(11), "set contains check 11");
  check(!sa.contains(3), "set contains check 3");

  check(*sa.next(3) == 4, "set next check 3");
  check(*sa.next(4) == 7, "set next check 4");
  check(sa.rank(4) == 1, "set rank test 4");
  check(sa.rank(11) == 3, "set rank test 11");
  check(sa.rank(17) == 5, "set rank test 17");

  int b[4] = {2, 7, 15, 22};
  set sb(b,b+4);
  set sc = set::map_union(sa,sb);
  check(sc.size() == 10, "set size check sc");
}

using wb_map  = aug_map<entry,weight_balanced_tree>;
using rb_map  = aug_map<entry,red_black_tree>;
using treap_map  = aug_map<entry,treap<entry>>;
using avl_map  = aug_map<entry,avl_tree>;

void test_all() {
  test_map<wb_map>(0);
  test_map<treap_map>(2); 
  test_map<avl_map>(3);
  test_map<rb_map>(1);
  test_set();
  test_map_more();
  test_aug();
  test_index();
  test_intervals();
  check(map::GC::num_used_nodes() == 0, "used nodes at end");
  check(map_max::GC::num_used_nodes() == 0, "used max nodes at end");
  test_map_reserve_finish();
  cout << "passed!!" << endl;
}

int main() {
  test_all();
}


  
