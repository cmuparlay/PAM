using Q14_rtype = double;

double Q14(maps m, const char* start_date, const char* end_date) {
  const char promo[] = "PROMO";
  int plen = strlen(promo);
  
  ship_map s_range = ship_map::range(m.sm, Date(start_date), Date(end_date));

  using sum = pair<float,float>;
  using Add = Add_Pair<Add<float>,Add<float>>;

  auto date_f = [&] (ship_map::E& e) -> sum {
    auto li_f = [&] (li_map::E& l) -> sum {
      float dp = l.e_price * (1.0 - l.discount.val());
	  char* x = static_data.all_part[l.partkey].type();
      if (memcmp(x, promo, plen) == 0) return sum(dp,dp);
      else return sum(dp,0.0);
    };
    return li_map::map_reduce(e.second, li_f, Add());
  };
  
  sum revenues = ship_map::map_reduce(s_range, date_f, Add(), 1);
  return revenues.second/revenues.first*100;
}

double Q14time(maps m, bool verbose) {
  timer t;
  t.start();
  const char start[] = "1995-09-01";
  const char end[] = "1995-09-31";

  Q14_rtype result = Q14(m, start, end);
  double ret_tm = t.stop();

  if (verbose) {
    cout << "Q14:" << endl << result << endl;
  }
  
  if (query_out) cout << "Q14 : " << ret_tm << endl;
  return ret_tm;
}
