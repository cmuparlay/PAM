#pragma once

#include "pam.h"

auto add = [] (float a, float b) -> float {
    return a + b;};

struct inv_index {
  using word = char*;
  using doc_id = int;
  using weight = float;

  using post_elt = pair<doc_id, weight>;

  struct doc_entry {
    using key_t = doc_id;
    using val_t = weight;
    static inline bool comp(key_t a, key_t b) { return a < b;}
    using aug_t = weight;
    static aug_t get_empty() {return 0;}
    static aug_t from_entry(key_t k, val_t v) {return v;}
    static aug_t combine(aug_t a, aug_t b) {
      return (b > a) ? b : a;}
    using entry_t = std::pair<key_t,val_t>;
  };

  using post_list = aug_map<doc_entry>;
  
  struct word_entry {
    using key_t = word;
    using val_t = post_list;
    static inline bool comp(const key_t& a, const key_t& b) {
      return strcmp(a, b) < 0;}
  };
  
  using index_elt = pair<word, post_elt>;
  using index_list = pair<word, post_list>;
  using index = pam_map<word_entry>;

  index idx;
  vector<index> idx_list;

  void reserve(size_t n) {
    post_list::reserve((size_t) round(.45*n));
    index::reserve(n/300);
}

  inv_index(index_elt* start, index_elt* end, size_t n = 0) {
	if (n == 0) n = end - start;
	timer t1; t1.start();
	reserve(n);
	n=end-start;
	t1.stop(); cout << "reserve: " << t1.get_total() << endl;
    auto reduce = [] (post_elt* s, post_elt* e) {
      return post_list(s,e,add); };
    //sequence<index_elt> S(start,end);
    idx = index::multi_insert_reduce(index(), start, n, reduce);
	cout << "idx size: " << idx.size() << endl;
  }
  
  void reserve_vector(size_t n) {
	  idx_list.reserve(n);
  }
  
  void update(index_elt* start, index_elt* end) {
    size_t n = end - start;		
    auto reduce = [] (post_elt* s, post_elt* e) {
      return post_list(s,e,add); };
	  
	auto f = [] (post_list a, post_list b) { return post_list::map_union(a,b, true);};
	//index idx2 = idx;
	index idx2 = idx;
	//idx2.get(idx);
	index bulk = index::multi_insert_reduce(index(), start, n, reduce);
	index new_idx = index::map_union(idx2,bulk,f,true);
	//idx_list.push_back(idx);
	idx = new_idx;
	//cout << "idx size: " << idx.size() << endl;
	//idx2.clear();
  }

  post_list get_list(index& idx2, const word w) {
	//index idx2 = idx;
	//idx2.get(idx);
	//return post_list();
	
    maybe<post_list> p;
	p = idx2.find(w);
	
    if (!p) return post_list();
	post_list p2 = p.value;
	return p2;
  }

  static post_list And(post_list a, post_list b) {
    return post_list::map_intersect(a,b,add);}

  static post_list Or(post_list a, post_list b) {
    return post_list::map_union(a,b,add);}

  static post_list And_Not(post_list a, post_list b) {
    return post_list::map_difference(a,b);}

  vector<post_elt> top_k(post_list a, int k) {
    int l = min<int>(k,a.size());
    vector<post_elt> vec(l);
    post_list b = a;
    for (int i=0; i < l; i++) {
      weight m = b.aug_val();
      auto f = [m] (weight v) {return v < m;};
      vec[i] = *b.aug_select(f);
      b = post_list::remove(move(b),vec[i].first);
    }
    return vec;
  }
  
  size_t size() {
	  struct MR {
		  using t = size_t;
		  static t from_entry(index::E e) {
			  return e.second.size();
		  }
		  static t combine(t a, t b) {return a+b;}
		  static t identity() {return 0;}
	  };
	  return index::map_reduce(idx,MR());
  }
	  
};
