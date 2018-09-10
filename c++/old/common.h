#pragma once

#include "pbbs-include/utilities.h"
#include "defs.h"
#include "avl.h"

template<class V>
struct get_left {
    V operator () (const V& a, const V& b) const
        { return a; }
};


template <class T> 
T* rebalance(T* current);

template <class T> 
bool is_balanced(T* current);

template <class T> 
struct tree_commons {
	static T* rotate_right(T* t) {
		T* root = t->lc;
		T* rsub = root->rc;
		root->rc = t, t->lc = rsub;
		root->rc->update();
		root->update();
		return root;
	}

	static T* rotate_left(T* t) {
		T* root = t->rc; 
		T* lsub = root->lc;
		root->lc = t, t->rc = lsub;
		root->lc->update();
		root->update();
		return root;
	}

	static T* double_rotate_right(T* t) {
		if (t->lc->rc && t->lc->rc->ref_cnt > 1) {
		   T* tmp = t->lc->rc;
		   t->lc->rc = t->lc->rc->copy();
		   decrease_recursive(tmp);
		}
		t->lc = rotate_left(t->lc);
		return rotate_right(t);
	}

	static T* double_rotate_left(T* t) {
		if (t->rc->lc && t->rc->lc->ref_cnt > 1) {
		   T* tmp = t->rc->lc;
		   t->rc->lc = t->rc->lc->copy();
		   decrease_recursive(tmp);
		}			
		t->rc = rotate_right(t->rc);
		return rotate_left(t);
	}

	static T* join_node(T* t1, T* t2, T* k) {
	  k->lc = t1,
	  k->rc = t2;
	  k->update();
	  return k;
	}
};

template <class T> 
size_t get_height(T* t) {
    if (!t) return 0;
    return 1 + max(get_height(t->lc), get_height(t->rc));
}

template <class T> 
bool check_balance(T* t) {
    if(!t) return 1;
    bool ret = is_balanced(t);
    ret &= check_balance(t->rc);
    ret &= check_balance(t->lc);
    return ret;
}

template <class T> bool check_bst(T* t) {
    if (!t) return 1;
    bool ret = 1;
    if (t->rc) ret &= t->key <= t->rc->key;
    if (t->lc) ret &= t->key >= t->lc->key;

    ret &= check_bst(t->lc);
    ret &= check_bst(t->rc);
    return ret;
}

template <class T> 
inline size_t get_node_count(T* t) {
    return !t ? 0 : t->node_cnt;
}

template <class T> 
bool decrease(T* t) {
    if (t) { 
      if (pbbs::fetch_and_add(&t->ref_cnt, -1) == 1) {
            t->collect();
            return true;
        }
    }
    return false;
}

template <class T> 
inline void increase(T* t) {
  if (t) pbbs::write_add(&t->ref_cnt, 1);
}

template <class T> 
inline T* inc(T* t) {
  increase(t); return t;
}

template <class T> 
void decrease_recursive(T* t) {
    if (!t) return;

    T* lsub = t->lc;
    T* rsub = t->rc;

    if (decrease(t)) {
        size_t sz = get_node_count(lsub);
        if (sz >= node_limit) {
            cilk_spawn decrease_recursive(lsub);
            decrease_recursive(rsub);
            cilk_sync;
        } else {
            decrease_recursive(lsub);
            decrease_recursive(rsub);
       }
    }
}

// copy node if reference count is > 1
template<class T>
static T* copy_if_needed(T* t) {
  T* res = t;
  if (t->ref_cnt > 1) {
    res = t->copy();
    decrease_recursive(t);
  }
  return res;
}

template<class T>
void print_stats() {
  using alloc = typename T::allocator;
  if (!alloc::initialized) 
    std::cout << "allocator not initialized" << std::endl;
  else {
    size_t used = alloc::num_used_blocks();
    size_t allocated = alloc::num_allocated_blocks();
    size_t size = alloc::block_size();
    std::cout << "used: " << used << ", allocated: " << allocated
	      << ", node size: " << size 
	      << ", bytes: " << size*allocated << std::endl;
  }
}
