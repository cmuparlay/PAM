using Q5_elt = pair<dkey_t, double>;
using Q5_rtype = sequence<Q5_elt>;

Q5_rtype Q5(maps m, char* start_date, char* end_date, uint qregion) {
  Date ostart = Date(start_date), oend = Date(end_date);
  customer_map cm = m.orders_for_customer;

  constexpr int num_nations = 25;
  using T = array<double, num_nations>;

  auto customer_f = [&] (T& a, customer_map::E& ce) -> void { 
    dkey_t nationid = ce.second.first.nationkey;
    if (nations[nationid].regionkey == qregion) {
      auto order_f = [&] (order_map::E& oe) -> double {
	Orders& ord = oe.second.first;
	if (Date::less(ord.order_date, ostart) ||
	    !Date::less(ord.order_date, oend)) return 0.0;
	auto li_f = [&] (line_item_set::E& l) -> double { 
	  dkey_t suppid = l.supplier_key;
	  Supplier& suppV = static_data.all_supp[suppid];
	  if (suppV.nationkey != nationid) return 0.0;
	  else return l.e_price * (1.0 - l.discount.val());
	};
	line_item_set li = oe.second.second; 
	return line_item_set::map_reduce(li, li_f, Add<double>());
      };
      order_map om = ce.second.second;
      a[nationid] += order_map::map_reduce(om, order_f, Add<double>());
    }
  };
      
  T nat_sums =
    customer_map::semi_map_reduce(cm, customer_f, Add_Array<T>());

  // convert from array to sequence, and sort by revenue
  auto r = tabulate(num_nations, [&] (size_t i) {
      return Q5_elt(i,nat_sums[i]);});
  auto less = [] (Q5_elt a, Q5_elt b) -> bool {return a.second > b.second;};
  return parlay::sort(r, less);
  return Q5_rtype();
}


double Q5time(maps m, bool verbose) {
  timer t_q5;
  t_q5.start();
  char start[] = "1994-01-01";
  char end[] = "1995-01-01"; 
  unsigned int qregion = 2;

  Q5_rtype result = Q5(m, start, end, qregion);

  double ret_tm = t_q5.stop();
  if (QUERY_OUT) cout << "Q5 : " << ret_tm << endl;

  if (verbose) {
    Q5_elt r = result[0];
    cout << "Q5:" << endl
	 << nations[r.first].name << ", " << r.second << endl;
  }	      
  return ret_tm;
}
