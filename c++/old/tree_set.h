#pragma once

#include "augmented_map.h"
#include "tree_operations.h"

template<class K>
class tree_set;

template<class K>
tree_set<K> set_union(tree_set<K>&, tree_set<K>&);

template<class K>
tree_set<K> set_intersect(tree_set<K>&, tree_set<K>&);

template<class K>
tree_set<K> set_difference(tree_set<K>&, tree_set<K>&);

template<class K>
tree_set<K>& final(tree_set<K>&);

template <class K>
class tree_set {
public:
    typedef K                                        key_type;
    typedef maybe<K>                                 maybe_key;
    typedef tree_set<K>                              set_type;
    typedef tree_map<K, nill>                        map_type;
    typedef std::pair<set_type, set_type>            set_pair;
    typedef typename tree_map<K, nill>::node_type    node_type;
    typedef typename tree_map<K, nill>::allocator    allocator;                

    tree_set();
    tree_set(const key_type&);
    tree_set(const tree_set<key_type>& m);
    //tree_set(const Iterator, const Iterator, const bool is_sorted = false);

    ~tree_set();
    
    size_t size() const;
    void clear();
    bool empty();
	
	//node_type* get_root() {return m.root;}
    
    template<class Iterator>
    void build(const Iterator begin, const Iterator end, bool is_sorted = false);
    
    template<class OutIterator>
    void content(OutIterator);
	node_type* get_root() { return m.root; }
    
    bool find(const key_type&);
    
    maybe_key next(const key_type&);
    maybe_key previous(const key_type&);
    
    size_t rank(const key_type&);
    key_type select(size_t);
    
    void insert(const key_type&);
    void remove(const key_type&);
    
    set_type range(key_type, key_type);
    set_pair split(const key_type&);
    
    template<class Func>
    set_type filter(const Func&);
    
    friend set_type& final<K>(set_type&);

    friend set_type set_union<K>(set_type&, set_type&);
    friend set_type set_difference<K>(set_type&, set_type&);
    friend set_type set_intersect<K>(tree_set<K>&, set_type&);
    
    static void init();
    static void reserve(size_t n);
    static void finish();
    
    set_type& operator = (const set_type& s);
	
	template<class OutIterator>
    static void collect_entries(const node_type*, OutIterator&);
    
private:
    template<class Iterator>
    node_type* construct(const Iterator, const Iterator);
    template<class Iterator>
    node_type* construct_sorted(const Iterator, const Iterator);

    tree_map<K, nill> m;
};

template<class K>
tree_set<K>::tree_set() {
    m.root = NULL;
}

template<class K>
tree_set<K>::tree_set(const key_type& k) {
    m.root = new node_type(std::make_pair(k, nill()));
}


template<class K>
tree_set<K>::tree_set(const set_type& s) {
    m = s.m;
}

template<class K>
tree_set<K>::~tree_set() {
    m.clear();
}

template<class K>
void tree_set<K>::reserve(size_t n) {
  allocator::reserve(n);
}

template<class K>
void tree_set<K>::init() {
    if (!allocator::initialized) {
        allocator::init();
    }
}

template<class K>
void tree_set<K>::finish() {
    if (allocator::initialized) {
        allocator::finish();
    }
}

template<class K>
size_t tree_set<K>::size() const {
    return m.size();
}

template<class K>
bool tree_set<K>::empty() {
    return m.empty();
}

template<class K>
tree_set<K>& tree_set<K>::operator = (const set_type& s) {
    m = s.m;
    return *this;
}

template<class K>
bool tree_set<K>::find(const key_type& key) {
    node_type* curr = m.root;
    
    bool found = false;
    while (curr) {
        if (curr->key < key) 
            curr = curr->rc;
        else if (curr->key < key) 
            curr = curr->lc;
        else {
            found = true;
            break;
        }
    }
    
    return found;
}

template<class K>
template<class OutIterator>
void tree_set<K>::collect_entries(const node_type* curr, OutIterator& out) {
    if (!curr) return;
    
    collect_entries(curr->lc, out);
    *out = curr->get_key(); ++out;
    collect_entries(curr->rc, out);
}

template<class K>
template<class OutIterator>
void tree_set<K>::content(OutIterator out) {
    collect_entries(m.root, out);
}

template<class K>
maybe<K> tree_set<K>::next(const key_type& key) {
    return m.next(key);
}

template<class K>
maybe<K> tree_set<K>::previous(const key_type& key) {
    return m.previous(key);
}

template<class K>
size_t tree_set<K>::rank(const key_type& key) {
    return m.rank(key);
}

template<class K>
K tree_set<K>::select(size_t rank) {
    return m.select(rank);
}

template<class K>
void tree_set<K>::insert(const key_type& key) {
    m.insert(make_pair(key, nill()));
}

template<class K>
void tree_set<K>::remove(const key_type& key) {
    m.remove(key);
}

template<class K>
void tree_set<K>::clear() {
    m.clear();
}

template<class K>
template<class Iterator>
typename tree_set<K>::node_type* tree_set<K>::construct(const Iterator lo, const Iterator hi) {
    if (lo > hi)
        return NULL;
    if (lo == hi) 
        return new node_type(make_pair(*lo, nill()));
    
    const Iterator mid = lo + (std::distance(lo, hi) >> 1);
    
    node_type* l = cilk_spawn this->construct(lo, mid);
    node_type* r = construct(mid+1, hi);
    cilk_sync;
    
    return tree_operations<node_type>::t_union(l, r, get_left<nill>());
}

template<class K>
tree_set<K>& final(tree_set<K>& s) {
    //final(s.m);
	std::move(s.m);	
    return s;
}

template<class K>
template<class Iterator>
typename tree_set<K>::node_type* tree_set<K>::construct_sorted(const Iterator lo, const Iterator hi) {
    if (lo > hi)
        return NULL;
    if (lo == hi) 
        return new node_type(make_pair(*lo, nill()));
    
    const Iterator mid = lo + (std::distance(lo, hi) >> 1);
	node_type* mid_node = new node_type(make_pair(*mid, nill()));
    
    node_type* l = cilk_spawn this->construct(lo, mid-1);
    node_type* r = construct(mid+1, hi);
    cilk_sync;
    
    return tree_operations<node_type>::t_join3(l, r, mid_node);
}

template<class K>
template<class Iterator>
void tree_set<K>::build(const Iterator begin, const Iterator end, bool is_sorted) {
    clear();
    
    if (begin != end) {
		if (!is_sorted) m.root = construct(begin, end-1);
		else m.root = construct_sorted(begin, end-1);
	}
}

template<class K>
tree_set<K> tree_set<K>::range(key_type low, key_type high) {
    tree_set<K> ret;
    ret.m = m.range(low, high);
    return ret;
}

template<class K>
std::pair<tree_set<K>, tree_set<K> > tree_set<K>::split(const key_type& key) {
    std::pair<map_type, map_type> maps = m.split(key);
    
    tree_set<K> s1, s2;
    s1.m = maps.first;
    s2.m = maps.second;
    
    return make_pair(s1, s2); 
}

template<class K>
template<class Func>
tree_set<K> tree_set<K>::filter(const Func& f) {
    tree_set<K> ret;
    ret.m = m.filter([f] (std::pair<K, nill> p) { return f(p.first); });
    return ret;
}

template<class K>
tree_set<K> set_union(tree_set<K>& s1, tree_set<K>& s2) {
    tree_set<K> ret;
    ret.m = map_union(s1.m, s2.m);
    return ret;
}

template<class K>
tree_set<K> set_intersect(tree_set<K>& s1, tree_set<K>& s2) {
    tree_set<K> ret;
    ret.m = map_intersect(s1.m, s2.m);
    return ret;
}

template<class K>
tree_set<K> set_difference(tree_set<K>& s1, tree_set<K>& s2) {
    tree_set<K> ret;
    ret.m = map_difference(s1.m, s2.m);
    return ret;
}
