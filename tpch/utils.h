const double epsilon = 0.00001;
template <class Val>
struct keyed_entry {
  using key_t = dkey_t;
  using val_t = Val;
  static bool comp(const key_t& a, const key_t& b) {return a < b;}
};

template <class Val>
using keyed_map = pam_map<keyed_entry<Val>>;

using key_pair = pair<dkey_t,dkey_t>;

template <class Val>
struct paired_key_entry {
  using key_t = key_pair;
  using val_t = Val;
  static bool comp(const key_t& a, const key_t& b) {return a < b;}
};

template <class Val>
using paired_key_map = pam_map<paired_key_entry<Val>>;

template <class Val>
struct date_entry {
  using key_t = Date;
  using val_t = Val;
  static bool comp(key_t a, key_t b) {return key_t::less(a,b);}
};

template <class Val>
using date_map = pam_map<date_entry<Val>>;

// Takes a nested map a and flattens it into a sequence
// Each value of the outer map is another map
// The function g is applied to each element of the inner map
// Returns a sequence of values returned by g
// OT is the type of the result
template <class OT,  class M, class G>
pbbs::sequence<OT> flatten(M a, G g) {
  using outer_elt_type = typename M::E;
  using inner_tree_type = typename M::V::Tree;
  using inner_elt_type = typename M::V::E;
  // uses node to avoid incrementing reference count
  using inner_node_p = typename M::V::node*;

  size_t n = a.size();

  pbbs::sequence<inner_node_p> inner_maps(n);
  M::foreach_index(a, [&] (const outer_elt_type& e, size_t i) -> void {
      inner_maps[i] = e.second.root;});

  pbbs::sequence<size_t> sizes(n, [&] (size_t i) -> size_t {
      return inner_tree_type::size(inner_maps[i]);});
  size_t total = pbbs::scan_inplace(sizes.slice(), pbbs::addm<size_t>());

  pbbs::sequence<OT> result = pbbs::sequence<OT>::no_init(total);

  parallel_for(0, n, [&] (size_t i) {
    size_t start = sizes[i];
    int granularity = 100;
    auto h = [&] (inner_elt_type& e, size_t j) -> void {
      pbbs::assign_uninitialized(result[start+j], g(e));};
    inner_tree_type::foreach_index(inner_maps[i], 0, h, granularity, true);    
    });
  return result;
}

// Takes a nested map a and flattens it into a sequence
// Each value of the outer map is a pair of a value and another map
// The function g is applied to both the element
//    of the outer map and the inner map
// Returns a sequence of values returned by g
// OT is the type of the result
template <class OT, typename M, class G>
pbbs::sequence<OT> flatten_pair(M& a, const G& g, size_t granu = 40) {
  timer tt;
  using OE = typename M::E;
  using inner_map = typename M::V::second_type;
  using IE = typename inner_map::E;
  // get sizes of inner map
  pbbs::sequence<uint> sizes(a.size());
  auto set_size = [&] (OE& e, size_t i) -> void {
    sizes[i] = e.second.second.size();};
  M::foreach_index(a, set_size);

  // generate offsets, and allocate result
  size_t total = pbbs::scan_inplace(sizes.slice(), pbbs::addm<uint>());
  pbbs::sequence<OT> result = pbbs::sequence<OT>::no_init(total);
  
  auto outer_f = [&] (OE& oe, size_t i) -> void {
    auto inner_f = [&] (IE& ie, size_t j) -> void {
      pbbs::assign_uninitialized(result[j], g(oe, ie));};
    // go over inner maps
    inner_map::map_index(oe.second.second, inner_f, granu, sizes[i]);
  };
  size_t granularity = 1 + 100 / (1 + total/a.size());

  // go over outer map
  M::foreach_index(a, outer_f, 0, granularity);
  return result;
}

template <class OT, class M, class G>
pbbs::sequence<OT> flatten_b(M a, G g) {
  using outer_t = typename M::E;
  using outer_pair = typename M::V;
  using outer_val = typename outer_pair::first_type;
  using inner_tree = typename outer_pair::second_type::Tree;
  // uses node to avoid incrementing reference count
  using inner_node_p = typename inner_tree::node*;
  using inner_t = typename outer_pair::second_type::E;
  size_t n = a.size();

  pbbs::sequence<outer_val> outer_vals(n);
  pbbs::sequence<inner_node_p> inner_maps(n);
  M::foreach_index(a, [&] (const outer_t& e, size_t i) -> void {
      outer_vals[i] = e.second.first;
      inner_maps[i] = e.second.second.root;
    });

  pbbs::sequence<size_t> sizes(n, [&] (size_t i) -> size_t {
      return inner_tree::size(inner_maps[i]);});
  size_t total = pbbs::scan_inplace(sizes.slice(), pbbs::addm<size_t>());

  pbbs::sequence<OT> result = pbbs::sequence<OT>::no_init(total);
  
  auto f = [&] (size_t i) {
    size_t start = sizes[i];
    outer_val e_outer = outer_vals[i];
    int granularity = 100;
    auto h = [&] (inner_t e_inner, size_t j) -> void {
      pbbs::assign_uninitialized(result[start+j], g(e_outer,e_inner));};
    // last argument to indicate that should not be collected
    inner_tree::foreach_index(inner_maps[i], 0, h, granularity, true);
  };

  foreach(0, n, f);
  return result;
}

template <class OT, typename M, class G>
pbbs::sequence<OT> flatten_pair_first(M a, G g) {
  using OE = typename M::E;
  using inner_map = typename M::V;
  using IE = typename inner_map::E;
  
  // get sizes of inner map
  auto get_size = [] (OE& e) -> uint {return e.second.size();};
  pbbs::sequence<uint> sizes = M::template to_seq<uint>(a, get_size, 40);

  // generate offsets, and allocate result
  size_t total = pbbs::scan_inplace(sizes.slice(), pbbs::addm<uint>());
  pbbs::sequence<OT> result = pbbs::sequence<OT>::no_init(total);

  auto outer_f = [&] (OE& oe, size_t i) -> void {
    auto inner_f = [&] (IE& ie, size_t j) -> void {
      pbbs::assign_uninitialized(result[j], g(ie));};
    // go over inner maps
    inner_map::foreach_index(oe.second, inner_f, sizes[i]);
  };
  size_t granularity = 1 + 100 / (total/a.size());

  // go over outer map
  M::foreach_index(a, outer_f, 0, granularity);
  return result;
}

template <class Map, class Reduce>
typename Reduce::t map_reduce_nested(Map m, Reduce R) {

  using E = typename Map::E;
  using V = typename Map::V;
  
  struct reduce_out {
    using t = typename Reduce::t;
    Reduce R;
    reduce_out(Reduce R) : R(R) {}
    t identity() { return R.identity();}
    t combine(const t& a, const t& b) { return R.combine(a, b);}
    t from_entry(const E& s) {
      return  V::map_reduce(s.second, R);
    }
  };

  return Map::map_reduce(m, reduce_out(R), 1);
};

template <class Index, class X, class F>
Index primary_index(pbbs::sequence<X> const &items, F get_key) {
  using K = typename Index::K;
  pbbs::sequence<pair<K,X>> entries(items.size(),[&] (size_t i) {
      return make_pair(get_key(items[i]), items[i]);
    });
  return Index(entries);
}

template <class Index, class X, class F>
Index secondary_index(pbbs::sequence<X> const &items, F secondary_key) {
  using outer_key = typename Index::K;
  using inner_map = typename Index::V;
  using inner_item = X; // same as inner_map::E;
  using P = pair<outer_key,inner_item>;
  pbbs::sequence<P> entries(items.size(), [&] (size_t i) {
      return make_pair(secondary_key(items[i]), items[i]);
    });
  return Index::multi_insert_reduce(Index(), entries,
				    [&] (pbbs::sequence<X> a) {
				      return inner_map(a);});
}

// Makes an index of items based on key extracted from each item by get_key
// The index is nested, first by secondary_key, and then by items
template <class Index, class Map, class F>
Index secondary_index(Map m, F secondary_key) {
  using outer_key = typename Index::K;
  using inner_item = typename Map::E;
  using P = pair<outer_key,inner_item>;
  pbbs::sequence<P> entries =
    Map::template to_seq<P>(m, [&] (inner_item& e) {
	return P(secondary_key(e), e);});
  return Index::multi_insert_reduce(Index(), entries,
				    [&] (pbbs::sequence<inner_item> a) {
				      return Map(a);});
}

template <class Index, class I1, class I2>
Index paired_index(I1 const &index1, I2 const &index2) {
  using v1 = typename I1::V;
  using v2 = typename I2::V;
  return Index::map_intersect(index1, index2, [&] (v1 a, v2 b) {
      return make_pair(a,b);});
}

// Makes an index of items based on key extracted from each item by get_key
// The index is nested, first by key, and then by items
template <class Index, class X, class F>
Index make_index(pbbs::sequence<X> const &items, F get_key) {
  using K = typename Index::K;
  using item_map = typename Index::V;
  using Item = X; // typename item_map::E;
  using KVp = pair<K,Item*>;
  using Entry = typename item_map::Entry;
  size_t n = items.size();
  auto f = [&] (size_t i) -> KVp {return KVp(get_key(items[i]), &items[i]);};
  pbbs::sequence<KVp> x(n, f);
  //item_map::reserve(n);

  auto make_item_map = [&] (pbbs::sequence<Item*> const &L) {
    auto less = [&] (Item* a, Item* b) {
      return Entry::comp(*a, *b);};
    pbbs::sequence<Item*> r = pbbs::sample_sort(L, less);
    pbbs::sequence<Item> s(r.size(),[&] (size_t i) {return *r[i];});
    return item_map::from_sorted(s);
  };
  Index sm = Index::multi_insert_reduce(Index(), x, make_item_map);
  return sm;
}

// Makes an index of items based on key extracted from each item by get_key
// The index is nested, first by key, and then by items
template <class Index, class T, class X, class F>
Index make_map_index(pbbs::sequence<pair<T,X>> items, F get_key) {
  using K = typename Index::K;
  using item_map = typename Index::V;
  using Item = pair<T,X>; // typename item_map::E;
  using KVp = pair<K,Item*>;
  using Entry = typename item_map::Entry;
  size_t n = items.size();
  auto f = [&] (size_t i) -> KVp {return KVp(get_key(items[i]), &items[i]);};
  pbbs::sequence<KVp> x(n, f);
  //item_map::reserve(n);

  auto make_item_map = [&] (pbbs::sequence<Item*> L) {
    auto less = [&] (Item* a, Item* b) {
      return Entry::comp((*a).first, (*b).first);};
    pbbs::sequence<Item*> r = pbbs::sample_sort(L, less);
    pbbs::sequence<Item> s(r.size(),[&] (size_t i) {return *r[i];});
    return item_map::from_sorted(s);
  };
  Index sm = Index::multi_insert_reduce(Index(), x, make_item_map);
  return sm;
}

// Makes an index of items based on key extracted from each item by get_key
// The index is nested, first by key, and then by items
template <class Index, class Map, class F>
Index make_index(Map m, F get_key) {
  using K = typename Index::K;
  using Item = typename Map::E;
  using KV = pair<K,Item>;
  auto f = [&] (Item& e) -> KV {return KV(get_key(e), e);};
  pbbs::sequence<KV> x = Map::template to_seq<KV>(m, f);
  auto make_item_map = [&] (pbbs::sequence<Item> const &L) -> Map {
    return Map(L);};
  return Index::multi_insert_reduce(Index(), x, make_item_map);
}

template <class Index, class X, class F, class G>
Index make_paired_index(pbbs::sequence<X> const &items, F get_key, G get_val) {
  using Key = typename Index::Entry::key_t;
  using Item = X; // typename Index::Entry::val_t::first_type;
  using Val = typename Index::V::second_type;
  using KV = pair<Key, pair<Item,Val>>;
  pbbs::sequence<KV> S(items.size(), [&] (size_t i) {
      Item x = items[i];
      auto y = pair<Item,Val>(x,get_val(x));
      return KV(get_key(x), y);
    });
  return Index(S);
}

struct priority_map {
  int a[6];
  priority_map() {
	  for (int i = 0; i < 6; i++) a[i] = 0;
  }
};

auto add_pri_map = [] (priority_map a, priority_map b) -> priority_map {
	priority_map ret;
	for (int i = 1; i < 6; i++) ret.a[i] = a.a[i]+b.a[i];
	return ret;
};

void histogram(int* a, int start, int end, priority_map& x) {
	if (end-start < 500) {
		for (int i = start; i < end; i++) x.a[a[i]]++;
		return;
	}
	priority_map x1, x2;
	int mid = 0; mid = (start+end)/2;
	par_do([&] () {histogram(a, start, mid, x1);},
	       [&] () {histogram(a, mid, end, x2);});
	for (int i = 1; i < 6; i++) x.a[i] = x1.a[i]+x2.a[i];
}


int random_hash(int seed, int a, int rangeL, int rangeR) {
  if (rangeL == rangeR) return rangeL;
  a = (a+seed) + (seed<<7);
  a = (a+0x7ed55d16) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = ((a+seed)>>5) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  a = (a^0xb55a4f09) ^ (a>>16);
  a = a % (rangeR-rangeL);
  if (a<0) a+= (rangeR-rangeL);
  a+=rangeL;
  return a;
}

const string today_string = "2000-01-01";
Date today = Date(today_string);
int rday=78989;

Date get_today() {
	if (random_hash('d'+'a'+'y', rday, 0, 100) == 0) {
		Date new_day = today;
		new_day.add_one_day();
		today = new_day;
	}
	rday ++;
	return today;
}
