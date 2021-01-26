// brand, type, size, supplier_count
using Q16_elt = tuple<char*,char*,int,int>;
using Q16_rtype = sequence<Q16_elt>;

Q16_rtype Q16(maps m,
	      const char* brand,
	      const char* type,
	      const int* sizes) {
  const int type_len = strlen(type);
  const char customer[] = "Customer";
  const char complain[] = "Complaints";

  // part number and part count
  using rtype = pair<dkey_t, int>;
  part_to_supp_map& psim = m.psm2;

  // hash table for parts kept by brand, type and size,
  // and with integer count
  struct hash_part {
    using eType = rtype;
    using kType = dkey_t;
    Part* prts;
    hash_part(Part* prts) : prts(prts) {}
    eType empty() {return rtype((uint) -1,0.0);}
    kType getKey(eType v) {return v.first;}
    size_t hash_string(char* s) {
      size_t hash = 0;
      while (*s) hash = *s++ + (hash << 6) + (hash << 16) - hash;
      return hash;
    }
    // hash on brand, type, size
    size_t hash(kType v) {
      Part& p = prts[v];
      return parlay::hash64(p.size
			  + 1000*hash_string(p.type())
			  + 1000000*hash_string(p.brand()));
    }
    // compare on brand, then type, then size
    int cmp(kType a, kType b) {
      Part& pa = prts[a];
      Part& pb = prts[b];
      int brand_cmp = strcmp(pa.brand(), pb.brand());
      if (brand_cmp != 0) return brand_cmp;
      int type_cmp = strcmp(pa.type(), pb.type());
      if (type_cmp != 0) return type_cmp;
      if (pa.size < pb.size) return -1;
      if (pa.size > pb.size) return 1;
      return 0;
    }
    bool replaceQ(eType v, eType b) {return 1;}
    eType update(eType v, eType b) {return rtype(v.first, v.second+b.second);}
    bool cas(eType* p, eType& o, eType& n) {
      return utils::atomic_compare_and_swap(p, o, n);
    }
  };

  auto htable = hash_part(static_data.all_part);
  parlay::hashtable<hash_part> T(psim.size()/6, htable, 2);

  auto map_part = [&] (part_to_supp_map::E& e, size_t i) {
    Part& p = e.second.first;
    int part_key = p.partkey;

    // exclude if not in list of sizes
    bool sizein = false;
    int sz = p.size;
    for (int i = 0; i < 8; i++) {
      if (sz == sizes[i]) {
		sizein = true;
		break;
      }
    }
    if (!sizein) return;

    // exclude if brand matches
    if (strcmp(p.brand(), brand) == 0) return;

    // exclude if type matches
    if (strncmp(p.type(), type, type_len) == 0) return;

    // count suppliers with no customer complaints
    auto map_supp = [&] (part_supp_and_item_map::E& e) -> int {
      //Supplier& s = e.second.first;
  	  dkey_t suppkey = e.second.first.suppkey;
	  Supplier& s = static_data.all_supp[suppkey];
      char* p = strstr(s.comment(), customer);
      if (p == NULL) return 1;
      if (strstr(p, complain) == NULL) return 1;
      else return 0;
    };
    int count = part_supp_and_item_map::map_reduce(e.second.second, map_supp, Add<int>());

    // update hash table by adding count
    T.update(rtype(part_key, count));
  };

  // go over all parts adding to hash table
  part_to_supp_map::map_index(psim, map_part);

  // extract entries from hash table
  sequence<rtype> res = T.entries();

  // sort by, count (desc), brand, type, size
  Part* parts = static_data.all_part;
  auto less = [&] (rtype a, rtype b) {
    if (a.second > b.second) return true;
    else if (a.second == b.second)
      return (htable.cmp(a.first, b.first) == -1);
    else return false;
  };
  sequence<rtype> sorted_res = parlay::sort(res, less);

  // generate result sequence
  auto to_res = [&] (size_t i) -> Q16_elt {
    Part& p = parts[sorted_res[i].first];
    int count = sorted_res[i].second;
    return Q16_elt(p.brand(), p.type(), p.size, count);
  };
  return parlay::tabulate(sorted_res.size(), to_res);
}

double Q16time(maps m, bool verbose) {
  const char brand[] = "Brand#45";
  const char type[] = "MEDIUM POLISHED";
  const int sizes[8] = {49, 14, 23, 45, 19, 3, 36, 9};
  timer t;
  t.start();

  Q16_rtype result = Q16(m, brand, type, sizes);

  double ret_tm = t.stop();
  if (query_out) cout << "Q16 : " << ret_tm << endl;
  
  if (verbose) {
    Q16_elt r = result[0];
    cout << "Q16:" << endl
	 << get<0>(r) << ", " << get<1>(r) << ", "
	 << get<2>(r) << ", " << get<3>(r) << endl;
  }
  return ret_tm;
}
