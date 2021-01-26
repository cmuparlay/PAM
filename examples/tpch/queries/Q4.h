using Q4_rtype = array<int,6>;

Q4_rtype Q4(maps m, const char* start, const char* end) {
  o_order_map oom = m.oom;
  o_order_map order_range = o_order_map::range(oom, Date(start), Date(end));

  using T = Q4_rtype;
  using H = Add_Array<T>;

  auto date_f = [] (o_order_map::E& d) -> T {
    auto order_f = [] (T& a, order_map::E& o) -> void {
      auto line_f = [] (Lineitem& e) -> int {
	return Date::less(e.c_date, e.r_date);};
      int n = li_map::map_reduce(o.second.second, line_f, Add<int>());
      if (n > 0) a[o.second.first.orderpriority]++;
    };
    return order_map::semi_map_reduce(d.second, order_f, H());
  };
  return o_order_map::map_reduce(order_range, date_f, H(), 1);
}

double Q4time(maps m, bool verbose) {
  timer t;
  t.start();
  const char start[] = "1993-07-01";
  const char end[] = "1993-09-30";

  Q4_rtype result = Q4(m, start, end);

  double ret_tm = t.stop();
  if (query_out) cout << "Q4 : " << ret_tm << endl;

  if (verbose) {
    int count = get<1>(result);
    cout << "Q4:" << endl << "1, " << count << endl;
  }
  return ret_tm;
}


