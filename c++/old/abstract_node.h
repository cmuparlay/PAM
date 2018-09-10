#pragma once
#include "pbbs-include/list_allocator.h"
#include "defs.h"

// Definitions in this file are independent of balance criteria beyond
// maintaining an abstract "rank"
template<class Node>
class avl_tree;

template<class Treetype, class E>
class basic_node {
public:
	//using E = typename Treetype::E;
	using key_type    = typename E::K;
	using entry_type  = typename E::K;
	using node_type   = basic_node<Treetype, E>;
	using allocator   = list_allocator<node_type>;
	using tree_type   = Treetype;
	
	basic_node(const key_type&, node_type* lc, node_type* rc, bool do_update = 1);
	basic_node(const key_type&, bool initialize = 1);
    basic_node() : ref_cnt(1) {};
	
	const key_type& get_key() const { return key; }
	const entry_type& get_entry() const { return key; }
	
	void set_key(const key_type _key) { key = _key; }
	
	static void* operator new(size_t size) { return allocator::alloc(); }
    inline node_type* copy();
    inline void update();
	inline void update_cnt();
	
    static inline node_type* connect_with_update(node_type* t1, node_type* t2, node_type* k) {
		k->lc = t1, k->rc = t2;
		k->update();
		return k;
    }
	
    static inline node_type* connect_without_update(node_type* t1, node_type* t2, node_type* k) {
		k->lc = t1, k->rc = t2;
		k->update_cnt();
		return k;
    }
	
	inline void collect();

    node_type* lc;  // left child
    node_type* rc;  // right child
    key_type key;
	unsigned char rank; // safe as a height, but not a weight
	tree_size_t node_cnt; // subtree size
    tree_size_t ref_cnt; // reference count
};

template<class Treetype, class E>
inline tree_size_t get_rank(const basic_node<Treetype, E>* t) {
  return t ? t->rank : 0;
}

template<class Treetype, class E>
inline void basic_node<Treetype, E>::collect() {
    get_key().~key_type();
    allocator::free(this);
}

template<class Treetype, class E>
inline basic_node<Treetype, E>* basic_node<Treetype, E>::copy() {
    node_type* ret = new node_type(get_key(), lc, rc, 0);
    ret->rank = rank;
    increase(lc);
    increase(rc);
    return ret;
}

template<class Treetype, class E>
inline void basic_node<Treetype, E>::update() {
    rank = tree_type::combine_ranks(get_rank(lc), get_rank(rc));
    node_cnt = 1 + get_node_count(lc) + get_node_count(rc);
}

template<class Treetype, class E>
    basic_node<Treetype, E>::basic_node(const key_type& k, node_type* left, node_type* right,
                 bool do_update) {
    set_key(k);
    ref_cnt = 1;
    lc = left;
    rc = right;
    if (do_update) update();
}

template<class Treetype, class E>
  basic_node<Treetype, E>::basic_node(const key_type& k, bool initialize) {
    set_key(k);
    ref_cnt = 1;
    if (initialize) {
      lc = rc = NULL;
      rank = tree_type::singleton_rank();
      node_cnt = 1;
    }
}

template<class Treetype, class E>
int get_real_height(basic_node<Treetype, E>* r) {
	if (r == NULL) return 0;
	else return max(get_real_height(r->lc), get_real_height(r->rc))+1;
}


