using Q21kip = pair<dkey_t, int>;
  
sequence<Q21kip> Q21(maps m, bool verbose, dkey_t q_nation) {
  supp_to_part_map spm = m.spm2;
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
		dkey_t orderkey = l1.orderkey;
		dkey_t suppkey = l1.suppkey;
		maybe<order_map::V> maybex = m.om.find(orderkey);
		if (!maybex) return 0;
		order_map::V x = (*maybex);
		if (x.first.status != 'F') return 0;
		li_map& y = x.second;
		auto cond1 = [&] (li_map::E& l2) {
		  if (l2.suppkey != suppkey) return true; else return false;
		};
		auto cond2 = [&] (li_map::E& l3) {
		  if (l3.suppkey != suppkey && Date::less(l3.c_date, l3.r_date)) 
			return true; else return false;
		};
		if (li_map::if_exist(y, cond1) && !(li_map::if_exist(y, cond2))) 
		  return 1; else return 0;
      };
      int v = li_map::map_reduce(inner_map, i_map_f, Add<int>());
      return v;
    };
    int v = part_supp_and_item_map::map_reduce(med_map, m_map_f, Add<int>(), 1);
    res[i] = Q21kip(e.second.first.suppkey, v);
  };  
  supp_to_part_map::map_index(spm, o_map_f);

  auto less = [] (Q21kip a, Q21kip b) { return a.second>b.second; };
  sequence<Q21kip> res2 = pbbs::sample_sort(res, less);
  
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
