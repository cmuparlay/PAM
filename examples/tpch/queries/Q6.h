double Q6(maps m, const char* start_date, const char* end_date,
	  float discount, int quantity) {
  ship_map sm = m.sm;
  ship_map ship_range = ship_map::range(sm, Date(start_date), Date(end_date));

  auto sh_f = [&] (ship_map::E& e) -> double {
    auto li_f = [=] (li_map::E& l) -> double {
      double dis = l.discount.val();
      int quant = l.quantity();
      if ((dis > (discount - .01-epsilon)) &&
	  (dis < (discount + .01+epsilon)) &&
	  (quant < quantity))
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

