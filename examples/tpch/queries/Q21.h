using Q21kip = pair<dkey_t, int>;
  
sequence<Q21kip> Q21(maps m, bool verbose, dkey_t q_nation) {
  supp_to_part_map spm = m.spm2;
  size_t max_order_key = (*(m.om.last())).first;
  //size_t max_order_key = CURR_ORDER;
  //cout << "max_order_key: " << max_order_key << endl;

  sequence<li_map*> olines(max_order_key+1, (li_map*) NULL);
  auto order_f = [&] (order_map::E& oe, size_t i) {
    if (oe.second.first.status == 'F')
      olines[oe.first] = &oe.second.second;
  };
  order_map::map_index(m.om, order_f);
  
  sequence<pair<dkey_t, int>> res(SUPPLIER_NUM+1);
  auto o_map_f = [&] (supp_to_part_map::E& e, size_t i) {
    if (e.second.first.nationkey != q_nation) {
      res[i] = make_pair(e.second.first.suppkey, 0);
      return;
    }
    part_supp_and_item_map& med_map = e.second.second;
    auto m_map_f = [&] (part_supp_and_item_map::E& e) -> int {
      li_map& inner_map = e.second.second;
      auto i_map_f = [&] (li_map::E& l1) -> int {
	if (!Date::less(l1.c_date, l1.r_date)) return 0;
	dkey_t suppkey = l1.suppkey;

	li_map* a = olines[l1.orderkey];
	if (a == NULL) return 0;
	li_map& y = *a; 
	auto cond1 = [&] (li_map::E l2) {
	  return (l2.suppkey != suppkey);
	};
	auto cond2 = [&] (li_map::E l3) {
	  return (l3.suppkey != suppkey && Date::less(l3.c_date, l3.r_date));
	};
	return (li_map::if_exist(y, cond1) && !(li_map::if_exist(y, cond2)));
      };
      int v = li_map::map_reduce(inner_map, i_map_f, Add<int>());
      return v;
    };
    int v = part_supp_and_item_map::map_reduce(med_map, m_map_f, Add<int>());
    res[i] = Q21kip(e.second.first.suppkey, v);
  };  
  supp_to_part_map::map_index(spm, o_map_f);

  auto less = [] (Q21kip a, Q21kip b) { return a.second>b.second; };
  sequence<Q21kip> res2 = parlay::sort(res, less);
  
  return res2;
}

double Q21time(maps m, bool verbose) {
  timer t;
  t.start();
  dkey_t q_nation = 20;
  
  sequence<Q21kip> res2 = Q21(m, verbose, q_nation);
  
  if (verbose) {
    Q21kip r = res2[0];
    cout << "Q21:" << endl
	 << static_data.all_supp[r.first].name() << " " << r.second << endl;
  }  
  
  double ret_tm = t.stop();
  if (query_out) cout << "Q21: " << ret_tm << endl;
  return ret_tm;
}
