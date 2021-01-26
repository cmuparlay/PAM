// shipmode1 (high, low), shipmode2 (high low)
using Q12_rtype = pair<pair<int, int>, pair<int, int>>;

Q12_rtype Q12(maps m, char* mode1, char* mode2,
	      char* start_date, char* end_date) {
  const Date start = Date(start_date);
  const Date end = Date(end_date);
  char m1 = mode1[0];
  char m2 = mode2[0];
  
  receipt_map& rm = m.rm;
  receipt_map rm_range = receipt_map::range(rm, start, end);

  using ret_t = Q12_rtype;
  using AddI = Add<int>;
  using AddP = Add_Pair<AddI,AddI>; // adds pairs of ints
  using AddPP = Add_Pair<AddP,AddP>;  // adds pairs of pairs of ints
  ret_t id = AddPP::identity();

  auto date_f = [&] (receipt_map::E& e) -> ret_t {
    auto li_f = [&] (li_map::E& e) -> ret_t {
      if (Date::less(e.c_date, e.r_date) && Date::less(e.s_date, e.c_date)
	  && !Date::less(e.r_date, start) && Date::less(e.r_date, end)) {
	if (e.shipmode() != m1 && e.shipmode() != m2) return id;
	int high = 0, low = 0;
	uchar o_priority = (*(m.om.find(e.orderkey))).first.orderpriority;
	if (o_priority == 1 || o_priority == 2) high = 1; else low = 1;
	if (e.shipmode() == m1)
	  return make_pair(make_pair(high, low), make_pair(0,0));
	if (e.shipmode() == m2)
	  return make_pair(make_pair(0,0), make_pair(high, low));
      }
      return id;
    };
    li_map& lim = e.second;
    return li_map::map_reduce(lim, li_f, AddPP());
  };
  return receipt_map::map_reduce(rm_range, date_f, AddPP(), 1);
}

double Q12time(maps m, bool verbose) {
  timer t;
  t.start();
  char start_date[] = "1994-01-01";
  char end_date[] = "1995-01-01";
  char ship_mode1[] = "MAIL";
  char ship_mode2[] = "SHIP";

  Q12_rtype result = Q12(m, ship_mode1, ship_mode2, start_date, end_date);
  
  double ret_tm = t.stop();
  if (query_out) cout << "Q12 : " << ret_tm << endl;

  if (verbose) {
    cout << "Q12:" << endl
	 << ship_mode1 << ", "
	 << result.first.first << ", "
	 << result.first.second << endl;
  }
  
  return ret_tm;
}

