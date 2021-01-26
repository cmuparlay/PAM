using pfloat = pair<double,double>;
using Q7_rtype = pair<pfloat, pfloat>;

Q7_rtype Q7(maps m, size_t n1id, size_t n2id) {
  supp_to_part_map spm = m.spm2;
  int max_supplier = (*(spm.previous(1000000000))).first;
  sequence<uchar> supplier_nation(max_supplier + 1);

  auto supplier_f = [&] (supp_to_part_map::E& se, size_t i) -> void {
    Supplier& s = se.second.first;
    supplier_nation[s.suppkey] = s.nationkey;
  };

  supp_to_part_map::map_index(spm, supplier_f);

  using AddD = Add<double>;
  using AddP = Add_Pair<AddD,AddD>; // adds pairs of doubles
  using AddPP = Add_Pair<AddP,AddP>;  // adds pairs of pairs of doubles
  pfloat id = AddP::identity();
      
  auto customer_f = [&] (customer_map::E& ce) -> Q7_rtype {
    unsigned int customer_nation = ce.second.first.nationkey;
    order_map& omap = ce.second.second;
    auto check_nation_pair = [&] (size_t nation_1, size_t nation_2) -> pfloat {
      if (customer_nation == nation_2) {
	auto order_f = [&] (order_map::E& oe) -> pfloat {
	  li_map& li = oe.second.second;
	  auto li_f = [&] (li_map::E& l) -> pfloat {
	    Date ship_date = l.s_date;
	    double rev = l.e_price*(1-l.discount.val());
	    int year = ship_date.year();
	    if ((year == 1995 || year == 1996)
		&& supplier_nation[l.suppkey] == nation_1) {
	      if (year == 1995) return pfloat(rev, 0.0);
	      if (year == 1996) return pfloat(0.0, rev); }
	    return id;
	  };
	  return li_map::map_reduce(li, li_f, AddP());
	};
	return order_map::map_reduce(omap, order_f, AddP());
      }
      return id;
    };
    return Q7_rtype(check_nation_pair(n1id, n2id),
		    check_nation_pair(n2id, n1id));
  };

  return customer_map::map_reduce(m.cm, customer_f, AddPP());
}

double Q7time(maps m, bool verbose) {
  timer t;
  t.start();
  //const char nation1[] = "FRANCE";
  //const char nation2[] = "GERMANY";
  unsigned int n1id = 6;
  unsigned int n2id = 7;

  Q7_rtype result = Q7(m, n1id, n2id);

  double ret_tm = t.stop();
  if (query_out) cout << "Q7 : " << ret_tm << endl;
  
  if (verbose) {
    cout << "Q7:" << endl
	 << nations[n1id].name << ", "
	 << nations[n2id].name << ", "
	 << "1995, "
	 << result.first.first << endl;
  }
  return ret_tm;
}
