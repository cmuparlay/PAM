#include <algorithm>
#include <iostream>
#include <cstdio>
#include "pam.h"
#include "pbbslib/random_shuffle.h"
#include "pbbslib/sequence_ops.h"

using namespace std;

using key_type = size_t;

void check(bool test, string message) {
  if (!test) {
    cout << "Failed test: " << message << endl;
    exit(1);
  }
}

struct entry {
  using key_t = key_type;
  using val_t = key_type;
  using aug_t = key_type;
  static inline bool comp(key_t a, key_t b) { return a < b;}
  static aug_t get_empty() { return 0;}
  static aug_t from_entry(key_t k, val_t v) { return v;}
  static aug_t combine(aug_t a, aug_t b) { return std::max(a,b);}
};

struct entry2 {
  using key_t = key_type;
  using val_t = bool;
  static inline bool comp(key_t a, key_t b) { return a < b;}
};

using par = pair<key_type, key_type>;

#ifdef NO_AUG
//using tmap  = pam_map<entry, avl_tree>;
using map  = pam_map<entry>;
#else
using map  = aug_map<entry>;
#endif

int main (int argc, char *argv[]) {
  //pbbs::sequence<par> x(70);
  //pbbs::sequence<par> y(40);
  pbbs::sequence<par> input_elt(100);
  for (int i = 0; i < 100; i++) input_elt[i] = par(i, i*2);
  pbbs::random_shuffle(input_elt);
  
  par x[70];
  par y[40];
  for (int i = 0; i < 70; i++) x[i] = input_elt[i];
  for (int i = 0; i < 40; i++) y[i] = input_elt[i+60];
  
  map mapx(x, x+70);
  map mapy(y, y+40);
  map mapz = map::map_union(mapx, mapy);
  check(mapz.size() == 100, "size check for mapz (should be 100)");
  for (int i = 0; i< 100; i++) check(mapz.find(i), "check all 100 elements");
  check(mapz.check_balance(), "z is balanced");
  
  cout << "Pass! ^o^" << endl;
  return 0;
}
