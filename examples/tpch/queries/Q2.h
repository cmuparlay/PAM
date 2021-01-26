// s_acctbal, s_name, n_name, p_partkey, p_mfgr, s_address, s_phone, s_comment
using Q2_elt = tuple<float, char*, char*, dkey_t, char*, char*, char*, char*>;
using Q2_rtype = sequence<Q2_elt>;

bool equal(float a, float b) {
  if (abs(a-b) < 0.001) return true; else return false;
}

Q2_rtype Q2(maps m, unsigned int rid, int size, char* type) {

  int typelen = strlen(type);
  using ps_map = part_to_supp_map;
  using s_map = part_supp_and_item_map;

  auto part_f = [&] (ps_map::E& pe) -> optional<ps_map::V> {
    Part& p = pe.second.first;
    s_map& sim = pe.second.second;
    char* t = p.type();
    // if correct size and type
    if (p.size == size && strcmp(t + strlen(t)- typelen, type) == 0) {
      // for each supplier in correct region return ps cost
      auto supp_cost = [&] (s_map::E& se) -> float {
	//Supplier& s = se.second.first.second;
	dkey_t suppkey = se.second.first.suppkey;
	Supplier& s = static_data.all_supp[suppkey];
	if (nations[s.nationkey].regionkey == rid) {
	  //return get_ps_cost(key_pair(p.partkey, s.suppkey));
	  return se.second.first.supplycost;
	}
	else return 1e10;
      };
      // find the cheapest
      float min_cost = s_map::map_reduce(sim, supp_cost, Min<float>());

      // for each supplier in correct region check if cheapest
      auto supp_f = [&] (s_map::E& se) -> bool {
	//Supplier& s = se.second.first.second;
	dkey_t suppkey = se.second.first.suppkey;
	Supplier& s = static_data.all_supp[suppkey];
	return (nations[s.nationkey].regionkey == rid &&
		se.second.first.supplycost == min_cost);
      };

      // filter those in correct region that equal cheapest
      s_map top = s_map::filter(sim, supp_f);
      if (top.size() > 0) return optional<ps_map::V>(make_pair(p,top));
      else return optional<ps_map::V>();
    } else return optional<ps_map::V>();
  };
  
  ps_map x = ps_map::map_filter(m.psm2, part_f, 10);
  using elt = Q2_elt;

  // selected values
  auto ps_f = [&] (ps_map::E pe, s_map::E se) -> elt {
    Part p = pe.second.first;
    dkey_t suppkey = se.second.first.suppkey;
    Supplier s = static_data.all_supp[suppkey];
    char* n_name = nations[s.nationkey].name;
    //s.acctbal
    return elt(s.acctbal, s.name(), n_name, p.partkey, p.mfgr(),
	       s.address(), s.phone(), s.comment());
  };
  sequence<elt> elts = flatten_pair<elt>(x, ps_f);
  

  // sort by output ordering: s_acctbal (desc), n_name, s_name, p_partkey
  auto less = [] (elt a, elt b) {
    //return (get<0>(a) > get<0>(b));
    if (get<0>(a) > get<0>(b)) return true;
    if (get<0>(a) < get<0>(b)) return false;
    int cmp_n_name = strcmp(get<2>(a), get<2>(b));
    if (cmp_n_name < 0) return true;
    if (cmp_n_name > 0) return false;
    int cmp_s_name = strcmp(get<1>(a), get<1>(b));
    if (cmp_s_name < 0) return true;
    if (cmp_s_name > 0) return false;
    return get<3>(a) < get<3>(b);
  };
  return parlay::sort(elts, less);
}

double Q2time(maps m, bool verbose) {
  timer t;
  t.start();
  unsigned int rid = 3;
  int size = 15;
  char type[] = "BRASS";

  Q2_rtype result = Q2(m, rid, size, type);
  //cout << result.size() << endl;
  
  double ret_tm = t.stop();
  if (query_out) cout << "Q2 : " << ret_tm << endl;  

  if (verbose) {
    Q2_elt r = result[0];
    cout << "Q2: " << endl
	 << get<0>(r) << ", " << get<1>(r) << ", "
	 << get<2>(r) << ", " << get<3>(r) << ", "
	 << get<4>(r) << ", " << get<5>(r) << ", "
	 << get<6>(r) << ", " << get<7>(r) << endl;
  }
  return ret_tm;
}

