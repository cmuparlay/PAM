#pragma once
#include "balance_utils.h"

// *******************************************
//   RED BLACK TREES
//   From the paper:
//   Just Join for Parallel Ordered Sets
//   G. Blelloch, D. Ferizovic, and Y. Sun
//   SPAA 2016
// *******************************************

struct red_black_tree {

  typedef enum { RED, BLACK } Color;
  struct data { unsigned char height; Color color;};

  // defines: node_join, is_balanced
  // redefines: update, single
  // inherits: make_node
  template<class Node>
  struct balance : Node {
    using node = typename Node::node;
    using t_utils = balance_utils<balance>;
    using ET = typename Node::ET;
    using GC = gc<balance>;
    
    static bool is_balanced(node* t) {
      return (!t ||
	      (((t->color == BLACK) ||
		(color(t->lc) == BLACK && color(t->rc) == BLACK))
	       && (height(t->lc) == height(t->rc))));
    }

    static void update(node* t) {
      Node::update(t);
      t->height = height(t->lc) + (t->color == BLACK);
    }
    
    static node* single(ET e) {
      node *o = Node::single(e);
      o->height = 1;
      o->color = BLACK;
      return o;
    }

    // main function 
    static node* node_join(node* t1, node* t2, node* k) {
      if (height(t1) > height(t2)) {
	node* t = right_join(t1, t2, k);
	if (t->color == RED && color(t->rc) == RED) {
	  t->color = BLACK; t->height++;}
	return t;
      }
      if (height(t2) > height(t1)) {
	node* t = left_join(t1, t2, k);
	if (t->color == RED && color(t->lc) == RED) {
	  t->color = BLACK; t->height++;}
	return t;
      }
      if (color(t1) == BLACK && color(t2) == BLACK)
	return balanced_join(t1,t2,k,RED);
      return balanced_join(t1,t2,k,BLACK);
    }

  private:

    static int height(node* a) {
      return (a == NULL) ? 0 : a->height;
    }

    static int color(node* a) {
      return (a == NULL) ? BLACK : a->color;
    }

    // a version without rotation 
    static node* balanced_join(node* l, node* r, node* e, Color color) {
      e->color = color;
      e->lc = l;
      e->rc = r;
      update(e);
      return e;
    }

    static node* right_join(node* t1, node* t2, node* k) {
      if (height(t1) == height(t2) && color(t1) == BLACK) 
	return balanced_join(t1, t2, k, RED);
      node* t = GC::copy_if_needed(t1);
      t->rc = right_join(t->rc, t2, k);

      // rebalance if needed
      if (t->color == BLACK && color(t->rc) == RED && color(t->rc->rc) == RED) {
	t->rc->rc->color = BLACK;
	t->rc->rc->height++;
	t = t_utils::rotate_left(t);
      } else update(t);
      return t;
    }

    static node* left_join(node* t1, node* t2, node* k) {
      if (height(t1) == height(t2) && color(t2) == BLACK) 
	return balanced_join(t1, t2, k, RED);
      node* t = GC::copy_if_needed(t2);
      t->lc = right_join(t1, t->lc, k);

      // rebalance if needed
      if (t->color == BLACK && color(t->lc) == RED && color(t->lc->lc) == RED) {
	t->lc->lc->color = BLACK;
	t->lc->lc->height++;
	t = t_utils::rotate_right(t);
      } else update(t);
      return t;
    }

  };
};
