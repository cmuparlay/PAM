#pragma once

#include "tree_operations.h"
#include <vector>
#include "types.h"
#include <typeinfo>
#include "common.h"
#include "pbbs-include/get_time.h"
#include "pbbs-include/sample_sort.h"

using namespace std;

template<class K, class V, class AugmOp, class Compare>
class augmented_map;

template<class K, class V, class Compare = std::less<K> >
using tree_map = augmented_map<K, V, noop<K,V>, Compare>;

template<class K>
class tree_set;

template<class amap> amap map_union(amap, amap);
template<class amap> amap map_join2(amap, amap);
template<class amap> amap map_intersect(amap, amap);
template<class amap> amap map_difference(amap, amap);

template<class amap, class BinaryOp>
amap map_union(amap, amap, const BinaryOp&);

template<class amap, class BinaryOp>
amap map_intersect(amap, amap, const BinaryOp& f);

template <class K, class V, class AugmOp, class Compare = std::less<K> >
class augmented_map {
 public:
    typedef K                                     key_type;
    typedef V                                     value_type;
    typedef std::pair<K,V>                        entry_type;
    typedef maybe<K>                              maybe_key;
    typedef maybe<V>                              maybe_value;
    typedef maybe<entry_type>                     maybe_entry;
    typedef typename AugmOp::aug_t                aug_type;
    typedef std::pair<K, V>                       tuple;
    typedef node<K, V, AugmOp, Compare>           node_type;
    typedef typename node_type::tree_type         tree_type;
    typedef augmented_map<K, V, AugmOp, Compare>  map_type;
    typedef std::pair<map_type, map_type>         map_pair;
    typedef tree_operations<node_type>            tree_ops;
    typedef typename node_type::allocator         allocator;
    typedef typename tree_ops::split_info         split_info;
    typedef Compare                               compare_type;

    // empty constructor
    augmented_map() : root(NULL) { allocator::init(); }

    // copy constructor, increment reference counce
    augmented_map(const map_type& m) {root = m.root; increase(root);}

    // move constructor, clear the source, leave reference count as is
    augmented_map(map_type&& m) { root = m.root; m.root = NULL;}

    // singleton
    augmented_map(const entry_type& e) { 
        allocator::init();
        root = new node_type(e);
    }
    
    // construct from an array keeping one of the equal keys
    augmented_map(entry_type* s, entry_type* e, 
		  bool is_sorted = false,
		  bool sequential = false,
		  bool in_place = false) : root(NULL) {
      allocator::init();
      multi_insert(s, e, is_sorted, sequential, in_place);
    }

    // construct from an array with combining of equal keys
    template<class Combine>
    augmented_map(entry_type* s, entry_type* e, const Combine& f, 
		  bool is_sorted = false, bool sequential = false) : root(NULL) {
      allocator::init();
      multi_insert(s, e, f, is_sorted, sequential);
    }

    // clears contents, decrementing ref counts
    void clear() {
      if (allocator::initialized) decrease_recursive(root);
      root = NULL;
    }

    // destruct.   
    ~augmented_map() { clear(); }

    // copy assignment, increase reference count
    map_type& operator = (const map_type& m) {
      if (this != &m) { clear(); root = m.root; increase(root); }
      return *this;
    }

    // move assignment, clear the source, leave reference count as is
    map_type& operator = (map_type&& m){
      if (this != &m) { clear(); root = m.root; m.root = NULL;}
      return *this; 
    }

    // some basic functions
    size_t size() const { return get_node_count(root); }
  
    void insert(const tuple& p) {
      auto replace = [] (const value_type& a, const value_type& b) {return b;};
      root = tree_ops::t_insert(root, p, replace); }

    template <class Func>
    void insert(const tuple& p, const Func& f) {
      root = tree_ops::t_insert(root, p, f); }

    void insert_lazy(const tuple& p) { root = tree_ops::t_insert_lazy(root, p); }
	template <class Func>
	void insert_lazy(const tuple& p, const Func& f) { root = tree_ops::t_insert_lazy(root, p, f); }
    void remove(const key_type& k) { root = tree_ops::t_delete(root, k); }
    bool empty() {return root == NULL;}

    // filters elements that satisfy the predicate when applied
    // to the key-value pair.   To keep old version asssign to another var.
    template<class Func>
    void filter(const Func& f) { root = tree_ops::t_filter(root, f); }
	
    template<class Func>
    void aug_filter(const Func& f) { root = tree_ops::t_aug_filter(root, f); }

    // insert multiple keys from an array
    void multi_insert(entry_type* s, entry_type* e, 
		      bool is_sorted = false, bool sequential = false) {
      root = tree_ops::multi_insert(root, s, e-s, is_sorted, sequential);}

    // insert multiple keys from an array with an associative combiner
    template<class Combine>
    void multi_insert(entry_type* s, entry_type* e, const Combine& f, 
		      bool is_sorted = false, bool sequential = false) {
      root = tree_ops::multi_insert(root, s, e-s, f, is_sorted, sequential);}

    // build a new tree with a reduction function for combining duplicates
    template<class Vin, class Reduce>
    void build_reduce(std::pair<K,Vin>* s, std::pair<K,Vin>* e,
		      const Reduce& reduce, bool is_sorted = false) {
      clear();
      root = tree_ops::build_reduce(s, e-s, reduce, is_sorted);}

    // basic search routines
    maybe_value find(const key_type& key) const {
      return node_to_val(tree_ops::t_find(root, key));}
    bool contains(const key_type& key) const {
      return (tree_ops::t_find(root, key) != NULL) ? true : false;}
    maybe_entry next(const key_type& key) const {
      return node_to_entry(tree_ops::t_next(root, key));}
    maybe_entry previous(const key_type& key) const {
      return node_to_entry(tree_ops::t_previous(root, key));}

    // rank and select
    size_t rank(const key_type& key) { return tree_ops::t_rank(root, key);}
    entry_type select(const size_t rank) const {
      const node_type* n = tree_ops::t_select(this->root, rank);
      return (n == NULL) ? entry_type() : n->get_entry();
    }

    // equality 
    bool operator == (const map_type& m) const {
      return (size() == m.size()) && (size() == map_union(*this,m).size());
    }
    bool operator != (const map_type& m) const { return !(*this == m); }

    // extract the augmented values
    aug_type aug_val() {
      if (root) return root->aug_val;
      return AugmOp::get_empty();
    }
    aug_type aug_left (const key_type& key) {
      typename tree_ops::range_sum_t a;
      tree_ops::t_sum_left(root, key, a);
      return a.result;}
    aug_type aug_right(const key_type& key) {
      typename tree_ops::range_sum_t a;
      tree_ops::t_sum_right(root, key, a);
      return a.result;}
    aug_type aug_range(const key_type& key_left, const key_type& key_right) {
      typename tree_ops::range_sum_t a;
      tree_ops::t_sum_range(root, key_left, key_right, a);
      return a.result;}

    template <class restricted_sum>
    void range_sum(const key_type& key_left, 
		   const key_type& key_right,
		   restricted_sum& rs) {
      return tree_ops::t_sum_range(root, key_left, key_right, rs);};

    template <class Func>
    maybe_entry aug_select(Func f) {
      return node_to_entry(tree_ops::aug_select(root, f));};

    // union, intersection and difference
    template<class amap> friend amap map_union(amap, amap);
	template<class amap> friend amap map_join2(amap, amap);
    template<class amap> friend amap map_difference(amap, amap);
    template<class amap> friend amap map_intersect(amap, amap);
    template<class amap, class BinaryOp>
    friend amap map_union(amap, amap, const BinaryOp&);
    template<class amap, class BinaryOp>
    friend amap map_intersect(amap, amap, const BinaryOp&);

    map_type range(const key_type& low, const key_type& high) {
      //increase(this->root);
      return tree_ops::t_range(root, low, high);
    }

  // back compatibility, should remove
    map_type range_fast(const key_type& low, const key_type& high) {
      return tree_ops::t_range(root, low, high);
    }

    map_pair split(const key_type& key) {
      split_info split = tree_ops::t_split(this->root, key);
      return std::make_pair(map_type(split.first), map_type(split.second));
    }

    // extract entries from the map sequentially into an output iterator
    template<class OutIterator>
    void content(OutIterator out) const {
      auto f = [&] (entry_type e) {*out = e; ++out;};
      tree_ops::t_foreach_seq(root, f);}

    template<class OutIterator>
    void keys(OutIterator out) const {
      auto f = [&] (entry_type e) {*out = e.first; ++out;};
      tree_ops::t_foreach_seq(root, f);}

    template<class OutIterator>
    void keys_in_range(const key_type& key_left, const key_type& key_right,
		       OutIterator out) {
      auto f = [&] (entry_type e) {*out = e.first; ++out;};
      tree_ops::t_foreach_seq_range(root, key_left, key_right, f);}

    template<class OutIterator>
    void entries_in_range(const key_type& key_left, const key_type& key_right,
			  OutIterator out) {
      auto f = [&] (entry_type e) {*out = e; ++out;};
      tree_ops::t_foreach_seq_range(root, key_left, key_right, f);}

    // map f(e,i) on each entry e along with its index i in parallel
    template<class F>
    void map_index(F f) { tree_ops::t_foreach_index(root, 0, f);}

    // write entries into array out, in parallel
    void entries(entry_type* out) {
      auto f = [&] (entry_type e, size_t i) -> void {out[i] = e;};
      tree_ops::t_foreach_index(root, 0, f);}

      // write entries into array out, in parallel
    void keys2(key_type* out) {
      auto f = [&] (entry_type e, size_t i) -> void {out[i] = e.first;};
      tree_ops::t_foreach_index(root, 0, f);}

    void keys_in_range2(const key_type& key_left, const key_type& key_right,
		        key_type* out) {
      map_type r = range_fast(key_left, key_right);
      r.keys2(out);}

    // initializing, reserving and finishing
    static void init() { allocator::init(); }
    static void reserve(size_t n, bool randomize=false) {
      allocator::reserve(n, randomize);};
    static void finish() { allocator::finish(); }
  
    // some memory statistics
    static size_t num_allocated_nodes() {
        return allocator::num_allocated_blocks();}
    static size_t num_used_nodes(){
        return allocator::num_used_blocks();}
    static size_t node_size () {
        return allocator::block_size();}
    static void print_allocation_stats() { 
      print_stats<node_type>();}
    
    struct split_t {
      split_t(node_type* left, node_type* right,
          key_type key, value_type val) 
      : left(map_type(left)), right(map_type(right)),
    key(key), val(val) {};
      map_type left;
      map_type right;
      key_type key;
      value_type val;
    };

    split_t split_mid() {
      assert(root != NULL);
      increase(root->lc);
      increase(root->rc);
      return split_t(root->lc, root->rc, root->get_key(), root->get_value());
    }

    node_type* get_root() {return root;}
    friend class tree_set<K>;

 private:

    augmented_map(node_type* r) : root(r) {};
    
    maybe_value node_to_val(node_type* a) const {
      if (a != NULL) return maybe_value(a->get_value());
      else return maybe_value();
    }
 
   maybe_entry node_to_entry(node_type* a) const {
      if (a != NULL) return maybe_entry(a->get_entry());
      else return maybe_entry();
    }

    template<class OutIterator, class DataOut>
    void collect(const node_type*, OutIterator&, const DataOut&) const;

    node_type* move_root() {node_type* t = root; root = NULL; return t;};

    node_type* root;
};

template<class map>
map map_union(map m1, map m2) {
  return map_union(move(m1), move(m2), get_left<typename map::value_type>());
}

template<class map>
map map_intersect(map m1, map m2) {
  return map_intersect(move(m1), move(m2), get_left<typename map::value_type>());
}

template<class map, class BinaryOp>
map map_union(map m1, map m2, const BinaryOp& op) {
  return map(map::tree_ops::t_union(m1.move_root(), m2.move_root(), op));
}

template<class map>
map map_join2(map m1, map m2) {
  return map(map::tree_ops::t_join2(m1.move_root(), m2.move_root()));
}

template<class map, class BinaryOp>
map map_intersect(map m1, map m2, const BinaryOp& op) {
  return map(map::tree_ops::t_intersect(m1.move_root(), m2.move_root(), op));
}

template<class map>
map map_difference(map m1, map m2) {
  return map(map::tree_ops::t_difference(m1.move_root(), m2.move_root()));
}

template<class A>
struct __aug__ {
  typedef typename A::A aug_t;
  static inline aug_t get_empty() { return A::I();}
  static inline aug_t from_entry(typename A::K a, typename A::V b) {
    return A::g(a,b);}
  static inline aug_t combine(aug_t a, aug_t b) { return A::f(a,b);}
};

template<class A>
struct __less__ {
  bool operator() (const typename A::K& a, const typename A::K& b) const {
    return A::less(a,b);
  }
};

template<class A>
using aug_map = augmented_map<typename A::K, typename A::V, __aug__<A>, __less__<A> >;

struct simple {
  typedef int K;
  typedef int V;
  static bool less(K a, K b) {return a < b;};
  typedef int A;
  static A I() { return 0;}
  static A g(K a, V b) {return b;}
  static A f(A a, A b) {return a + b;}
};

using a = aug_map<simple>;
