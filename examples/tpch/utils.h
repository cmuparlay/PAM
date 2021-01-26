const double epsilon = 0.00001;

// ****************************
// Some index types (keyed_map, paired_key_map, and date_map)
// ****************************

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

// ****************************
// Constructing indices (primary_index, secondary_index, paired_index)
// ****************************

template <class Index, class X, class F>
Index primary_index(parlay::sequence<X> const &items, F get_key) {
  using K = typename Index::K;
  parlay::sequence<pair<K,X>> entries = parlay::tabulate(items.size(),[&] (size_t i) {
      return make_pair(get_key(items[i]), items[i]);
    });
  return Index(entries);
}

template <class Index, class X, class F>
Index secondary_index(parlay::sequence<X> const &items, F secondary_key) {
  using outer_key = typename Index::K;
  using inner_map = typename Index::V;
  using inner_item = X; // same as inner_map::E;
  using P = pair<outer_key,inner_item>;
  parlay::sequence<P> entries = parlay::tabulate(items.size(), [&] (size_t i) {
      return make_pair(secondary_key(items[i]), items[i]);
    });
  //return Index::multi_insert_reduce(Index(), entries,
	//			    [&] (parlay::sequence<X> a) {
	//			      return inner_map(a);});
  return Index::multi_insert_reduce(Index(), entries,
				    [&] (parlay::slice<X*, X*> a) {
				      return inner_map(parlay::sequence<X>(a.begin(), a.end()));});
}

// a slightly faster version since it sorts pointers to structures
// instead of the structures themselves.   Otherwise identical in
// behavior to above.
template <class Index, class X, class F>
Index secondary_index_o(parlay::sequence<X>  &items, F get_key) {
  using outer_key = typename Index::K;
  using inner_map = typename Index::V;
  using item = X; // same as inner_map::E;
  using P = pair<outer_key, item*>;
  size_t n = items.size();
  parlay::sequence<P> x = parlay::tabulate(n, [&] (size_t i) {
	  Lineitem* cur = &(items[i]);
      return P(get_key(items[i]), cur);});

  //auto make_item_map = [&] (parlay::sequence<item*> &L) {
  auto make_item_map = [&] (parlay::slice<item**, item**> L) {
    auto less = [&] (item* a, item* b) {
      return inner_map::Entry::comp(*a, *b);};
    parlay::sequence<item*> r = parlay::sort(L, less);
    parlay::sequence<item> s = parlay::tabulate(r.size(),[&] (size_t i) {return *r[i];});
    return inner_map::from_sorted(s);
  };
  Index sm = Index::multi_insert_reduce(Index(), x, make_item_map);
  return sm;
}

// Makes an index of items based on key extracted from each item by get_key
// The index is nested, first by secondary_key, and then by items
template <class Index, class Map, class F>
Index secondary_index(Map m, F secondary_key) {
  using outer_key = typename Index::K;
  using inner_item = typename Map::E;
  using P = pair<outer_key,inner_item>;
  parlay::sequence<P> entries =
    Map::template to_seq<P>(m, [&] (inner_item& e) {
	return P(secondary_key(e), e);});
  m = Map(); // to free memory
  //return Index::multi_insert_reduce(Index(), entries,
	//			    [&] (parlay::sequence<inner_item> a) {
	//			      return Map(a);});
  return Index::multi_insert_reduce(Index(), entries,
				    [&] (parlay::slice<inner_item*, inner_item*> a) {
				      return Map(parlay::sequence<inner_item>(a.begin(), a.end()));});
}


template <class Index, class I1, class I2>
Index paired_index(I1 const &index1, I2 const &index2) {
  using v1 = typename I1::V;
  using v2 = typename I2::V;
  return Index::map_intersect(index1, index2, [&] (v1 a, v2 b) {
      return make_pair(a,b);});
}

// a slightly different interface
template <class Index, class X, class F, class G>
Index paired_index(parlay::sequence<X> const &items, F get_key, G get_val) {
  using Key = typename Index::Entry::key_t;
  using Item = X; // typename Index::Entry::val_t::first_type;
  using Val = typename Index::V::second_type;
  using KV = pair<Key, pair<Item,Val>>;
  parlay::sequence<KV> S = parlay::tabulate(items.size(), [&] (size_t i) {
      Item x = items[i];
      auto y = pair<Item,Val>(x,get_val(x));
      return KV(get_key(x), y);
    });
  return Index(S);
}

// ****************************
// Flattenning nested indices (flatten and flatten_pair)
// ****************************

// Takes a nested map a and flattens it into a sequence
// Each value of the outer map is another map
// The function g is applied to each element of the inner map
// Returns a sequence of values returned by g
// OT is the type of the result
template <class OT,  class M, class G>
parlay::sequence<OT> flatten(M a, G g) {
  using outer_elt_type = typename M::E;
  using inner_tree_type = typename M::V::Tree;
  using inner_elt_type = typename M::V::E;
  // uses node to avoid incrementing reference count
  using inner_node_p = typename M::V::node*;

  size_t n = a.size();

  parlay::sequence<inner_node_p> inner_maps(n);
  M::foreach_index(a, [&] (const outer_elt_type& e, size_t i) -> void {
      inner_maps[i] = e.second.root;});

  parlay::sequence<size_t> sizes = parlay::tabulate(n, [&] (size_t i) -> size_t {
      return inner_tree_type::size(inner_maps[i]);});
  //size_t total = parlay::scan_inplace(sizes.cut(), parlay::addm<size_t>());
  size_t total = parlay::scan_inplace(sizes, parlay::addm<size_t>());

  parlay::sequence<OT> result = parlay::sequence<OT>::uninitialized(total);

  parlay::parallel_for(0, n, [&] (size_t i) {
    size_t start = sizes[i];
    int granularity = 100;
    auto h = [&] (inner_elt_type& e, size_t j) -> void {
      parlay::assign_uninitialized(result[start+j], g(e));};
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
parlay::sequence<OT> flatten_pair(M& a, const G& g, size_t granu = 40) {
  timer tt;
  using OE = typename M::E;
  using inner_map = typename M::V::second_type;
  using IE = typename inner_map::E;
  // get sizes of inner map
  parlay::sequence<uint> sizes(a.size());
  auto set_size = [&] (OE& e, size_t i) -> void {
    sizes[i] = e.second.second.size();};
  M::foreach_index(a, set_size);

  // generate offsets, and allocate result
  //size_t total = parlay::scan_inplace(sizes.slice(), parlay::addm<uint>());
  size_t total = parlay::scan_inplace(sizes, parlay::addm<uint>());
  parlay::sequence<OT> result = parlay::sequence<OT>::uninitialized(total);
  
  auto outer_f = [&] (OE& oe, size_t i) -> void {
    auto inner_f = [&] (IE& ie, size_t j) -> void {
      parlay::assign_uninitialized(result[j], g(oe, ie));};
    // go over inner maps
    inner_map::map_index(oe.second.second, inner_f, granu, sizes[i]);
  };
  size_t granularity = 1 + 100 / (1 + total/a.size());

  // go over outer map
  M::foreach_index(a, outer_f, 0, granularity);
  return result;
}

// ****************************
// Other Utilities
// ****************************

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
