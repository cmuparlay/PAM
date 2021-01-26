using Q20rtype = tuple<char*,char*>;

sequence<Q20rtype> Q20(maps m, bool verbose,
		       const char* start_date, const char* end_date,
		       dkey_t q_nation, const char* color) {
  const Date start = Date(start_date);
  const Date end = Date(end_date);
  supp_to_part_map& spm = m.spm2;
  
  auto supp_filter = [&] (supp_to_part_map::E& e) {
    Supplier& s = e.second.first;
    if (s.nationkey != q_nation) return false;
    part_supp_and_item_map& inner_map = e.second.second;
    auto exists = [&] (part_supp_and_item_map::E& e) {
      Part_supp& ps = e.second.first;
      dkey_t partkey = ps.partkey;
      Part& p = static_data.all_part[partkey];
      li_map& lm = e.second.second;
      auto map_lineitem = [&] (li_map::E& l) -> int {
	if (Date::less(l.s_date, start)) return 0;
	if (Date::less(l.s_date, end)) return l.quantity(); else return 0;
      };
      int th = li_map::map_reduce(lm, map_lineitem, Add<int>());
      if ((strstr(p.name(), color) == p.name()) && ((ps.availqty*2) > th)) {
	return true;
      }
      else return false;
    };
    return part_supp_and_item_map::if_exist(inner_map, exists);
  };
  supp_to_part_map spm_nation = supp_to_part_map::filter(spm, supp_filter);
  
  sequence<Q20rtype> res(spm_nation.size());
  auto map_supp = [&] (supp_to_part_map::E& e, size_t i) {
    Supplier& s = e.second.first;
    res[i] = Q20rtype(s.name(), s.address());
  };
  supp_to_part_map::map_index(spm_nation, map_supp);
  
  return res;
}

double Q20time(maps m, bool verbose) {
  timer t;
  t.start();
  
  char start_date[] = "1994-01-01";
  char end_date[] = "1994-12-31";
  const char color[] = "forest";
  dkey_t q_nation = 3;
  
  sequence<Q20rtype> res = Q20(m, verbose, start_date, end_date,
			       q_nation, color);
  
  if (verbose) {
    Q20rtype r = res[0];
    cout << "Q20:" << endl
	 << get<0>(r) << ", "
	 << get<1>(r) << endl;
  }
  
  double ret_tm = t.stop();
  if (query_out) cout << "Q20 : " << ret_tm << endl;
  return ret_tm;
}
