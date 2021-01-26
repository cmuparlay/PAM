// nation, year, sum of profit
using Q9_elt = tuple<char*, int, double>;
using Q9_rtype = sequence<Q9_elt>;

Q9_rtype Q9(maps mx, const char* color) {
  maps m = mx;
  using ps_map = part_to_supp_map;
  using s_map = part_supp_and_item_map;
  order_map& om = m.om;
  o_order_map& odm = m.oom;
  timer t;
  t.start();

  const uchar num_nations = 25;
  const uchar num_years = 20;
  size_t min_year = (*(odm.select(0))).first.year();
  size_t max_year = (*(odm.select(odm.size()-1))).first.year();
  if (max_year - min_year + 1 > num_years) {
    cout << "year out of range in Q9" << endl;
    return Q9_rtype();
  }

  // write the order year out into a sequence indexed by order key
  size_t max_order_num = (*(om.select(om.size() - 1))).first;
  sequence<uchar> ord_year(max_order_num+1);
  auto order_f = [&] (order_map::E& pe, size_t i) -> void {
    ord_year[pe.first] = (uchar) (pe.second.first.orderdate.year()-min_year);
  };
  order_map::map_index(om, order_f);

  using nation_a = array<double,num_nations>;
  using nat_years = array<nation_a,num_years>;

  auto part_f = [&] (nat_years& a, ps_map::E& pe) -> void {
    Part& p = pe.second.first;
    s_map& sm = pe.second.second;
    dkey_t part_key = pe.first;
    if (strstr(p.name(), color) != NULL) {
      auto supp_f = [&] (s_map::E& se, size_t i) -> void {
	li_map& li = se.second.second;
	dkey_t supp_key = se.first.second;

	double supp_cost = se.second.first.supplycost;

	int natkey = static_data.all_supp[se.first.second].nationkey;
	auto li_f = [&] (li_map::E& l) -> void {
	  double profit = ((double) l.e_price * (1.0 - l.discount.val())
			   - supp_cost * l.quantity());
	  int year = ord_year[l.orderkey];
	  a[year][natkey] += profit;
	};
	li_map::foreach_seq(li, li_f);
      };
      //s_map::foreach_seq(sm, supp_f);
	  s_map::foreach_index(sm, supp_f, 100);
    }
  };

  part_to_supp_map& psm = m.psm2;
  using Add = Add_Nested_Array<nat_years>;
  nat_years a = ps_map::semi_map_reduce(psm, part_f, Add(), 4000);
  
  auto gen_results = [&] (size_t i) -> Q9_elt {
    int nationkey = i/num_years;
    int year = (num_years - 1 - i%num_years);
    char* nat_name = nations[nationkey].name;
    double profit = a[year][nationkey];
    Q9_elt r(nat_name, year+min_year, profit);
    return r;
  };

  auto rr = parlay::tabulate(num_years * num_nations, gen_results);
  auto non_zero = [&] (Q9_elt a) {return get<2>(a) != 0.0;};
  return parlay::filter(rr, non_zero);
}

double Q9time(maps m, bool verbose) {
  timer t;
  t.start();
  const char color[] = "green";

  Q9_rtype result = Q9(m, color);
   
  double ret_tm = t.stop();
  if (query_out) cout << "Q9 : " << ret_tm << endl;
 
  if (verbose) {
    for (int i=0; i<1; i++) {
    Q9_elt r = result[i];
    cout << "Q9:" << endl
	 << get<0>(r) << ", "
	 << get<1>(r) << ", "
	 << get<2>(r) << endl;
    }
  }

  return ret_tm;
}
