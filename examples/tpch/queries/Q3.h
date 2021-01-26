// order_key, ship_priority, order_date, revenue
using Q3_relt = tuple<dkey_t, int, Date, float>;
using Q3_rtype = sequence<Q3_relt>;

Q3_rtype Q3(maps m, char* seg, Date date) {

  struct set_entry {
    using key_t = Q3_relt;
    static bool comp(const key_t& a, const key_t& b) {
      return (get<3>(a) > get<3>(b)); }
  };
  using result_list = pam_set<set_entry>;

  customer_map& cm = m.cm;
  struct Union {
    using T = result_list;
    static T identity() {return result_list();}
    static T add(T a, T b) {
      return T::map_union(std::move(a), std::move(b)).take(10);}
  };

  auto customer_f = [&] (customer_map::E& c) {
    char* mkseg = c.second.first.mktsegment();
    if (strcmp(mkseg, seg) == 0) {
      auto order_f = [&] (order_map::E& o) {
	Orders& ord = o.second.first;
	if (Date::less(ord.orderdate, date)) {
	  auto lineitem_f = [&] (li_map::E& l) {
	    if (Date::less(date, l.s_date))
	      return l.e_price * (1.0 - l.discount.val());
	    else return 0.0; };
	  li_map& lm = o.second.second;
	  float revenue = li_map::map_reduce(lm, lineitem_f, Add<float>());
	  if (revenue > 0.0)
	    return result_list({Q3_relt(ord.orderkey,
					ord.shippriority,
					ord.orderdate,
					revenue)});
	}
	return result_list();
      };
      order_map& od = c.second.second;
      return order_map::map_reduce(od, order_f, Union());
    } else return result_list();
  };

  result_list r = customer_map::map_reduce(cm, customer_f, Union());

  return result_list::entries(r.take(10));
}

double Q3time(maps m, bool verbose) {
  timer t;
  t.start();
  char seg[] = "BUILDING";  // one of 5
  char sdate[] = "1995-03-15";
  Date date = Date(sdate);
  Q3_rtype r =  Q3(m, seg, date);
  double ret_tm = t.stop();

  Q3_relt x = r[0];
  if (verbose) {
    cout << "Q3:" << endl;
    cout << get<0>(x) << ", "  // ord_key
	 << get<3>(x) << ", "  // revenue
	 << get<2>(x).to_str() << ", " // ord_date
	 << get<1>(x) << endl;  // ship_priority
  }

  if (query_out) cout << "Q3 : " << ret_tm << endl;
  return ret_tm;  
}
