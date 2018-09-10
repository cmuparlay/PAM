// Weight Balanced Tree balance operations
#pragma once

#include "common.h"
#include <cstddef> 
#include <utility>
#include "abstract_node.h"
const double alpha = 0.29;
const double beta  = (1-2*alpha) / (1-alpha);


template<class Node>
class wb_tree {
 public:

  static tree_size_t combine_ranks(tree_size_t l, tree_size_t r) {
    return 0;
  }

  static tree_size_t singleton_rank() { return 0; }

  static tree_size_t weight(const Node* t) {
    if (t == NULL) return 1;
    return t->node_cnt + 1;
  }

  static inline bool is_too_heavy(const Node* t1, const Node* t2) {
    return alpha * weight(t1) > (1 - alpha) * weight(t2);
  }

  static bool is_single_rotation(Node* t, bool child) {
    int balance = weight(t->lc);
    if (child) balance = weight(t) - balance;
    return balance <= beta * weight(t);
  }

  static Node* persist_rebalanceSingle(Node* t) {
    Node* ret = t;
    if (is_too_heavy(t->lc, t->rc)) {   
      if (is_single_rotation(t->lc, 1)) ret = rotate_right(t);
      else ret = double_rotate_right(t);
    
    } else if(is_too_heavy(t->rc, t->lc)) {
      if (is_single_rotation(t->rc, 0)) ret = rotate_left(t);
      else ret = double_rotate_left(t);
    }

    return ret;
  }

  static Node* t_join(Node* t1, Node* t2, Node* k) {
    Node* ret = NULL;
    
    if (is_too_heavy(t1, t2)) {
      ret = copy_if_needed(t1);
      ret->rc = t_join(ret->rc, t2, k);
    } else if (is_too_heavy(t2, t1)) {
      ret = copy_if_needed(t2);
      ret->lc = t_join(t1, ret->lc, k);
    } else join_node(t1,t2,k);
    
    ret->update();
    return persist_rebalanceSingle(ret);
  }

};
