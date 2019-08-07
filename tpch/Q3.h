struct Q3result {
  Q3result(int k, int s, Date o, float r)
    : ord_key(k), ship_priority(s), ord_date(o), revenue(r) {}
  Q3result() {}
  int ord_key;
  int ship_priority;
  Date ord_date;
  float revenue;
};
struct set_entry {
  using key_t = Q3result;
  static bool comp(const key_t& a, const key_t& b) {
    return (a.revenue > b.revenue);
  }
};
using rt = pam_seq<Q3result>;
using result_list = pam_set<set_entry>;

void Q3(maps m, bool verbose, char* seg, Date date) {
  
  customer_map& cm = m.cm;
  struct Union {
    using T = result_list;
    static T identity() {return result_list();}
    static T add(T a, T b) {
      return T::map_union(std::move(a), std::move(b));}
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
	    return result_list(Q3result(ord.orderkey,ord.shippriority,ord.orderdate,revenue));
	}
	return result_list();
      };
      order_map& od = c.second.second;
      return order_map::map_reduce(od, order_f, Union());
    } else return result_list();
  };

  result_list r = customer_map::map_reduce(cm, customer_f, Union());

  Q3result x = *(r.select(0));
  if (verbose) {
	  cout << "Q3:" << endl;
	  cout << x.ord_key << ", "
	       << x.revenue << ", "
	       << x.ord_date.to_str() << ", "
	       << x.ship_priority << endl;
  }
}

/*
void Q3slow(maps m, bool verbose, char* seg, Date date) {
  
  customer_map& cm = m.cm;
  order_map& om = m.om;
  
  struct Union {
    using T = result_list;
    static T identity() {return result_list();}
    static T add(T a, T b) {
      return T::map_union(std::move(a), std::move(b), [](T a, T b) {return });}
  };
  
  sequence<bool> if_seg(CUSTOMER_NUM+1);
  auto customer_segment = [&] (customer_map::E& c, size_t i) {
	  char* mkseg = c.second.first.mktsegment();
	  if (strcmp(mkseg, seg) == 0) if_seg[c.second.first.custkey] = true; else if_seg[c.second.first.custkey] = false;
  };
  customer_map::map_index(cm, customer_segment);
  
  sequence<bool> if_seg_order(ORDER_NUM+1);
  sequence<int> o_shippriority(ORDER_NUM+1);
  sequence<Date> o_orderdate(ORDER_NUM+1);
  auto order_segment = [&] (order_map::E& o, size_t i) {
	  Orders ord = o.second.first;
	  if_seg_order[ord.orderkey] = if_seg[ord.custkey];
	  if (!Date::less(ord.orderdate, date)) if_seg_order[ord.orderkey] = false;
	  o_shippriority[ord.orderkey] = ord.shippriority;
	  o_orderdate[ord.orderkey] = ord.orderdate;
  };
  order_map::map_index(om, order_segment);

    auto order_f = [&] (order_map::E& o) {
	  auto lineitem_f = [&] (li_map::E& l) {
		if (!if_seg_order[l.orderkey]) return 0.0;
	    if (Date::less(date, l.s_date))
	      return l.e_price * (1.0 - l.discount.val());
	    else return 0.0; };
	  li_map& lm = o.second.second;
	  float revenue = li_map::map_reduce(lm, lineitem_f, Add<float>());
	  if (revenue > 0.0)
	    return result_list(Q3result(l.orderkey,o_shippriority[l.orderkey],o_orderdate[l.orderkey],revenue));
	return result_list();
      };

  result_list r = order_map::map_reduce(om, order_f, Union());

  Q3result x = *(r.select(0));
  if (verbose) {
	  cout << "Q3:" << endl;
	  cout << x.ord_key << ", "
	       << x.revenue << ", "
	       << x.ord_date.to_str() << ", "
	       << x.ship_priority << endl;
  }
}*/


double Q3time(maps m, bool verbose) {
  timer t;
  t.start();
  char seg[] = "BUILDING";  // one of 5
  char sdate[] = "1995-03-15";
  Date date = Date(sdate);
  Q3(m, verbose, seg, date);
  double ret_tm = t.stop();
  if (query_out) cout << "Q3 : " << ret_tm << endl;
  return ret_tm;  
}

/*
double Q3old(maps m, bool verbose) {
  timer t;
  t.start();
  const char seg[] = "BUILDING";  // one of 5
  const char sdate[] = "1995-03-15";
  const Date date = Date(sdate);

  auto lineitem_f = [&] (li_map::E& l) {
    if (Date::less(date, l.s_date))
      return l.e_price * (1.0 - l.discount.val());
    else return 0.0;
  };

  auto order_f = [&] (order_map::E& o) {
    Orders ord = o.second.first;
    if (Date::less(ord.orderdate, date)) {
      float revenue = li_map::map_reducef(o.second.second, lineitem_f, plus_float, 0.0);
      if (revenue > 0.0)
	return rt(Q3result(ord.orderkey,ord.shippriority,ord.orderdate,revenue));
    }
    return rt();
  };

  auto customer_f = [&] (customer_map::E& c) {
    char* mkseg = c.second.first.mktsegment();
    if (strcmp(mkseg, seg) == 0)
      return order_map::map_reducef(c.second.second, order_f, rt::join2, rt());
    else return rt();
  };

  rt r = customer_map::map_reducef(m.cm, customer_f, rt::join2, rt());
  double ret_tm = t.stop();
  if (query_out) cout << "Q3 : " << ret_tm << endl;
  return ret_tm;
}
*/
