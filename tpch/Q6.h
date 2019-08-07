double Q6(maps m, const char* start, const char* end, float discount, int quantity) {
  ship_map sm = m.sm;
  ship_map ship_range = ship_map::range(sm, Date(start), Date(end));

  auto sh_f = [&] (ship_map::E& e) -> double {
    auto li_f = [=] (li_map::E& l) -> double {
      double dis = l.discount.val();
      int quant = l.quantity();
      if ((dis > (discount - .01-epsilon)) && (dis < (discount + .01+epsilon))
	  && (quant < quantity))
	return l.e_price * dis;
      else return 0.0;
    };
    return li_map::map_reduce(e.second, li_f, Add<double>());
  };

  return ship_map::map_reduce(ship_range, sh_f, Add<double>(), 1);
}

double Q6time(maps m, bool verbose) {
  timer t;
  t.start();
  const char start[] = "1994-01-01";
  const char end[] = "1994-12-31";
  const float discount = .06;
  const int quantity = 24;

  double result = Q6(m, start, end, discount, quantity);
  double ret_tm = t.stop();
  if (query_out) cout << "Q6 : " << ret_tm << endl;

  if (verbose) cout << "Q6:" << endl << result << endl;
  return ret_tm;
}

/*double Q6old(maps m, bool verbose) {
  timer t;
  t.start();
  const string start = "1996-01-01";
  const string end = "1997-01-01";
  const float discount = .06;
  const int quantity = 24;

  ship_map ship_range = ship_map::range(m.sm, Date(start), Date(end));

  struct map_reduce {
    using t = double;
    float discount;
    int quantity;
    map_reduce(float d, int q) : discount(d), quantity(q) {}
    static t identity() { return 0.0;}
    static t combine(t a, t b) { return a + b;}
    t from_entry(Lineitem l) {
      float dis = l.discount.val();
      int quant = l.quantity();
      if ((dis > (discount - .01)) && (dis < (discount + .01))
	  && (quant < quantity))
	return l.e_price * dis;
      else return 0;
    };
  };

  double result = map_reduce_nested(ship_range,
				   map_reduce(discount,quantity));
  
  double ret_tm = t.stop();
  cout << "Q6 old : " << ret_tm << endl;
  if (verbose) cout << "result = " << result << endl;
  return ret_tm;
}
*/
