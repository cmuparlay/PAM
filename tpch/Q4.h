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


/*
Q4_rtype Q4x(maps m, const char* start, const char* end) {
  o_order_map order_range = o_order_map::range(m.oom, Date(start), Date(end));

  auto get_priority = [] (order_map::E& o) -> int {
    auto check_date = [&] (Lineitem& e) -> int {
      return Date::less(e.c_date, e.r_date);};
    int n = li_map::map_reducef(o.second.second, check_date, plus_int, 0);
    if (n > 0) return o.second.first.orderpriority;
    else return 0;
  };
  
  sequence<int> priorities = flatten<int>(order_range, get_priority);
  return pbbs::histogram<int>(priorities, 6);
}

double Q4x(maps m, bool verbose) {
  timer t;
  t.start();
  const string start = "1993-07-01";
  const string end = "1993-10-01";
  o_order_map m_range = o_order_map::range(m.oom, Date(start), Date(end));
  size_t s = m_range.size();

  sequence<priority_map> x(s);
  
  auto f = [&] (o_order_map::E& e, size_t idx) {
	order_map m0 = e.second;
	size_t s = m0.size();
	sequence<int> a(s);
	auto f2 = [&] (order_map::E& e, size_t idx) {
		priority_map ret;
		auto f3 = [&] (li_map::E& e) -> bool {
			return Date::less(e.c_date, e.r_date);
		};
		int p = e.second.first.orderpriority;

		if (li_map::if_exist(e.second.second,f3)) {
			a[idx] = p; //b[idx] = true;
		} else {
			a[idx] = 0; //b[idx] = false;
		}
	};
	order_map::map_index(m0,f2,10);
	histogram(a.as_array(), 0, s, x[idx]);
  };

  o_order_map::map_index(m_range, f,1);
  pbbs::reduce(x,add_pri_map);
  
  double ret_tm = t.stop();
  cout << "Q4x : " << ret_tm << endl;
  return ret_tm;
}
*/
