// c_name, c_custkey, o_orderkey, o_orderdate, o_totalprice, sum(quant)
using Q18_elt = tuple<char*, dkey_t, dkey_t, Date, float, int>;
using Q18_rtype = sequence<Q18_elt>;

Q18_rtype Q18(maps m, int q_quantity) {
  using elt = Q18_elt;
  using r_seq = pam_seq<elt>;

  struct Join {
    using T = r_seq;
    static T identity() {return r_seq();}
    static T add(T a, T b) {return r_seq::join2(move(a), move(b));}
  };
  
  auto cust_f = [&] (customer_map::E& ce) -> r_seq {
    Customer& c = ce.second.first;
    order_map& omap = ce.second.second;
    auto order_f = [&] (order_map::E& oe) -> r_seq {
      li_map& lmap = oe.second.second;
      Orders& o = oe.second.first;
      auto line_f = [&] (li_map::E& e) -> int {return e.quantity();};
      int v = li_map::map_reduce(lmap, line_f, Add<int>());
      if (v > q_quantity)
	return r_seq({elt(c.name(), c.custkey, o.orderkey,
			  o.orderdate, o.totalprice, v)}); 
      else return r_seq();
    };
    return order_map::map_reduce(omap, order_f, Join());
  };

  r_seq result = customer_map::map_reduce(m.cm, cust_f, Join(), 10);
  sequence<elt> r2 = r_seq::entries(result);

  auto less = [] (elt a, elt b) {
    return (get<4>(a) > get<4>(b) ||   // decrease on totalprice
	    (get<4>(a) == get<4>(b) && // increase on orderdate
	     Date::less(get<3>(a), get<3>(b))));};
  return parlay::sort(r2, less);
}

double Q18time(maps m, bool verbose) {
  timer t;
  t.start();
  int q_quantity = 300;

  Q18_rtype result = Q18(m, q_quantity);
  double ret_tm = t.stop();

  if (verbose) {
    Q18_elt a = result[0];
    cout << "Q18:" << endl
	 << get<0>(a) << ", " << get<1>(a) << ", " << get<2>(a) << ", "
	 << get<3>(a) << ", " << get<4>(a) << ", " << get<5>(a) << endl;
  }

  if (query_out) cout << "Q18 : " << ret_tm << endl;
  return ret_tm;
}

