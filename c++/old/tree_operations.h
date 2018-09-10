#pragma once

#include "common.h"
#include "abstract_node.h"
#include "pbbs-include/binary_search.h"
#include "pbbs-include/sample_sort.h"
#include "pbbs-include/get_time.h"

template<class Node>
struct seq_operations {
  using K           = typename Node::key_type;
  using E           = typename Node::entry_type;
  using tree_type   = typename Node::tree_type;
  
  static Node* t_join3(Node* b1, Node* b2, Node* k) {
      return tree_type::t_join(b1, b2, k);
  }

  // for two input sizes of n and m, should we do a parallel fork
  static bool do_parallel(size_t n, size_t m) {
    if (m > n) std::swap(n,m);
    return (n > 256 && m > 12);
  }

  // fork-join parallel call, returning a pair of values
  template <class RT, class Lf, class Rf>
  static std::pair<RT,RT> fork(bool do_parallel, Lf left, Rf right) {
    if (do_parallel) {
      RT r = cilk_spawn right();
      RT l = left();
      cilk_sync;
      return std::pair<RT,RT>(l,r);
    } else {
      RT l = left(); 
      RT r = right();
      return std::make_pair(l,r);
    }
  }

  // fork-join parallel call, returning nothing
  template <class Lf, class Rf>
  static void fork_no_result(bool do_parallel, Lf left, Rf right) {
    if (do_parallel) {cilk_spawn right(); left(); cilk_sync; }
    else {left(); right();}
  }

  struct split_info {
    split_info(Node* first, Node* second, bool removed)
      : first(first), second(second), removed(removed) {};
    split_info(Node* first, E value, Node* second)
      : first(first), second(second), value(value), removed(true) {};

    Node* first;
    Node* second;
    E value;
    bool removed;
  };

  static inline Node* inc_if(Node* t, bool copy) {
    if (copy) increase(t);
    return t;
  }

  static inline void dec_if(Node* t, bool copy, bool extra_ptr) {
    if (!extra_ptr) {
      if (copy) decrease_recursive(t);
      else decrease(t);
    }
  }

  // be careful of placement
  // needs to go after last use of t or t's children, since it can
  // delete t, and its descendants
  static inline Node* copy_if(Node* t, bool copy, bool extra_ptr) {
    if (copy) {
      Node* r = new Node(t->get_entry(), t->lc, t->rc, false);
      if (!extra_ptr) decrease_recursive(t);
      return r;
    } else return t;
  }

  static Node* t_join2_i(Node* b1, Node* b2, bool extra_b1, bool extra_b2) {
    if (!b1) return inc_if(b2, extra_b2);
    if (!b2) return inc_if(b1, extra_b1);
    
    if (b1->rank > b2->rank) {
      bool copy_b1 = extra_b1 || b1->ref_cnt > 1;
      Node* l = inc_if(b1->lc, copy_b1);
      Node* r = t_join2_i(b1->rc, b2, copy_b1, extra_b2);
      return t_join3(l, r, copy_if(b1, copy_b1, extra_b1));
    }
    else {
      bool copy_b2 = extra_b2 || b2->ref_cnt > 1;
      Node* l = t_join2_i(b1, b2->lc, extra_b1, copy_b2);
      Node* r = inc_if(b2->rc, copy_b2);
      return t_join3(l, r, copy_if(b2, copy_b2, extra_b2));
    }
  }

  static Node* t_join2(Node* b1, Node* b2) {
    return t_join2_i(b1, b2, false, false);
  }
  
  template<class Func>
  static Node* t_filter_i(Node* b, const Func& f, bool extra_ptr) {
    if (!b) return NULL;
    bool copy = extra_ptr || (b->ref_cnt > 1);
    
    auto P = fork<Node*>(get_node_count(b) >= node_limit,
      [&]() {return t_filter_i(b->lc, f, copy);},
      [&]() {return t_filter_i(b->rc, f, copy);});

    if (f(b->get_entry())) {
      return t_join3(P.first, P.second, copy_if(b, copy, extra_ptr));
    } else {
      dec_if(b, copy, extra_ptr);
      return t_join2(P.first, P.second);
    }
  }

  template<class Func>
  static inline Node* t_filter(Node* b, const Func& f) {
    return t_filter_i(b, f, false);
  }
  
  // Assumes the input is sorted and there are no duplicate keys
  static Node* t_from_sorted_array(E* A, size_t n) {
      if (n <= 0) return NULL;
      if (n == 1) return new Node(A[0]);

      size_t mid = n/2;
      Node* m = new Node(A[mid]);

      auto P = fork<Node*>(n >= node_limit,
      [&]() {return t_from_sorted_array(A, mid);},
      [&]() {return t_from_sorted_array(A+mid+1, n-mid-1);});

      return t_join3(P.first, P.second, m);
  }
 
  template<class NodeType, class Func>
  static void t_forall(Node* b, const Func& f, NodeType*& join_node) {
    if (!b) {
      join_node = NULL;
      return;
    }

    typename NodeType::entry_type entry
      = make_pair(b->get_key(), f(b->get_entry()));

    join_node = new NodeType(entry);

    size_t mn = get_node_count(b);
    auto P = fork<Node*>(mn >= node_limit,
       [&] () {return t_forall(b->lc, f, join_node->lc);},
       [&] () {return t_forall(b->rc, f, join_node->rc);});

    join_node->update();
  }

  static Node* t_select(Node* b, size_t rank) {    
    size_t lrank = rank;
    while (b) {
      size_t left_size = get_node_count(b->lc);
      if (lrank > left_size) {
	lrank -= left_size + 1;
	b = b->rc;  
      }
      else if (lrank < left_size) 
	b = b->lc;
      else 
	return b;
    }
    return NULL;
  }

  // applies the function f(e, i) to every entry and its index i
  // works in parallel in O(log n) depth
  template<typename F>
  static void t_foreach_index(Node* a, size_t start, const F& f) {
    if (!a) return;
    size_t lsize = get_node_count(a->lc);
    fork_no_result(lsize >= node_limit,
      [&] () {t_foreach_index(a->lc, start, f);},
      [&] () {t_foreach_index(a->rc, start + lsize + 1,f);});
    f(a->get_entry(), start+lsize);
  }

  // similar to above but sequential using in-order traversal
  // usefull if using 20th century constructs such as iterators
  template<typename F>
  static void t_foreach_seq(Node* a, const F& f) {
    if (!a) return;
    t_foreach_seq(a->lc, f);
    f(a->get_entry());
    t_foreach_seq(a->rc, f);
  }
};


//set operations
template<class Node>
struct set_operations : public seq_operations {
  using K           = typename Node::key_type;
  using E           = typename Node::entry_type;
  using key_compare = typename Node::key_compare;
  using tree_type   = typename Node::tree_type;
  
    // A version that copies all nodes
  // Will not adjust ref count of root
  static split_info t_split_copy(Node* bst, const K& e) {
    if (!bst) return split_info(NULL, NULL, false);

    else if (comp(bst->get_key(), e)) {
      Node* join = new Node(bst->get_entry(), false);
      split_info bstpair = t_split_copy(bst->rc, e);
      increase(bst->lc);
      bstpair.first = t_join3(bst->lc, bstpair.first, join);
      return bstpair;
    }
    else if (comp(e, bst->get_key())) {
      Node* join = new Node(bst->get_entry(), false);
      split_info bstpair = t_split_copy(bst->lc, e);
      increase(bst->rc);
      bstpair.second = t_join3(bstpair.second, bst->rc, join);
      return bstpair;
    }
    else {
      increase(bst->lc);  increase(bst->rc);
      split_info ret(bst->lc, bst->rc, true);
      ret.value = bst->get_value();
      return ret;
    }
  }

  // A version that will reuse a node if ref count is 1
  // Will decrement ref count of root if it is copied
  static split_info t_split_inplace(Node* bst, const K& e) {
    if (!bst) return split_info(NULL, NULL, false);
    else if (bst->ref_cnt > 1) {
      split_info ret = t_split_copy(bst, e);
      decrease_recursive(bst);
      return ret;
    }
    else if (comp(bst->get_key(), e)) {
      split_info bstpair = t_split_inplace(bst->rc, e);
      bstpair.first = t_join3(bst->lc, bstpair.first, bst);
      return bstpair;
    }
    else if (comp(e, bst->get_key())) {
      split_info bstpair = t_split_inplace(bst->lc, e);
      bstpair.second = t_join3(bstpair.second, bst->rc, bst);
      return bstpair;
    }
    else {
      split_info ret(bst->lc, bst->rc, true);
      ret.value = bst->get_value();
      decrease(bst);
      return ret;
    }
  }

    static split_info t_split_i(Node* t, const K& e, bool extra_ptr) {
    if (!t) return split_info(NULL, NULL, false);
    bool copy = extra_ptr || (t->ref_cnt > 1);
    
    if (comp(t->get_key(), e)) {
      split_info tpair = t_split_i(t->rc, e, copy);
      Node* l = inc_if(t->lc, copy);
      tpair.first = t_join3(l, tpair.first, copy_if(t, copy, extra_ptr));
      return tpair;
    }
    else if (comp(e, t->get_key())) {
      split_info tpair = t_split_i(t->lc, e, copy);
      Node* r = inc_if(t->rc, copy);
      tpair.second = t_join3(tpair.second, r, copy_if(t, copy, extra_ptr));
      return tpair;
    }
    else {
      split_info r(inc_if(t->lc, copy), t->get_value(),
		   inc_if(t->rc, copy));
      dec_if(t, copy, extra_ptr);
      return r;
    }
  }

  static inline split_info t_split(Node* bst, const K& e) {
    return t_split_inplace(bst, e);
  }
 
};

template<class Node>
struct map_operations : public set_operations {
	  // Works in-place when possible.
  // extra_b2 means there is an extra pointer to b2 not included
  // in the reference count.  It is used as an optimization to reduce
  // the number of ref_cnt updates.
  template <class BinaryOp> 
  static Node* t_union_i(Node* b1, Node* b2, const BinaryOp& op,
			 bool extra_b2) {
    if (!b1) return inc_if(b2, extra_b2);
    if (!b2) return b1;
    bool copy = extra_b2 || (b2->ref_cnt > 1);
    Node* r = copy ? new Node(b2->get_entry(), false) : b2;
    split_info bsts = t_split(b1, b2->get_key());

    auto P = fork<Node*>(do_parallel(get_node_count(b1), get_node_count(b2)),
      [&] () {return t_union_i(bsts.first, b2->lc, op, copy);},
      [&] () {return t_union_i(bsts.second, b2->rc, op, copy);});

    if (copy && !extra_b2) decrease_recursive(b2);

    if (bsts.removed) r->set_value(op(bsts.value, r->get_value()));
    return t_join3(P.first, P.second, r);
  }

  template <class BinaryOp> 
  static inline Node* t_union(Node* b1, Node* b2, const BinaryOp& op) {
    //return t_union_old(b1, b2, op);
    return t_union_i(b1, b2, op, false);
  }
};

