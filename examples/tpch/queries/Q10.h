// c_custkey, c_name, revenue, c_acctbal, n_name, c_address, c_phone, c_comment
using Q10_relt = tuple<dkey_t, char*, float, float, char*, char*, char*, char*>;
using Q10_rtype = sequence<Q10_relt>;

Q10_rtype Q10(maps m, const char* start, const char* end) {

  // take range of order dates
  o_order_map order_range = o_order_map::range(m.oom, Date(start), Date(end));
	
  using kf_pair = pair<dkey_t, float>;

  // get (custkey,revenue) for lineitems with return flag by order
  auto date_f = [&] (order_map::E& e) -> kf_pair {
    Orders& o = e.second.first;
    auto li_f = [&] (li_map::E& l) -> float {
      if (l.returnflag() != 'R') return 0.0;
      else return l.e_price*(1 - l.discount.val());
    };
    float v = li_map::map_reduce(e.second.second, li_f, Add<float>());
    return make_pair(o.custkey, v);
  };
  sequence<kf_pair> elts = flatten<kf_pair>(order_range, date_f);

  // sum revenue by custkey
  sequence<kf_pair> r = parlay::reduce_by_key(elts, parlay::plus<float>());
  //sequence<kf_pair> r = parlay::internal::group_by_and_combine(elts, parlay::addm<float>());

  // sort by revenue
  auto less = [] (kf_pair a, kf_pair b) {return a.second > b.second;};
  sequence<kf_pair> res = parlay::sort(r, less);

  // generate results
  auto get_result = [&] (size_t i) -> Q10_relt {
    dkey_t custkey = res[i].first;
    float revenue = res[i].second;
    Customer c = (*(m.cm.find(custkey))).first;
    char* n_name = nations[c.nationkey].name;
    return Q10_relt(custkey, c.name(), revenue, c.acctbal,
		    n_name, c.address(), c.phone(), c.comment());};
  auto rr = parlay::tabulate(20, get_result);      
  return rr;
}

double Q10time(maps m, bool verbose) {
  timer t;
  t.start();
  const char start[] = "1993-10-01";
  const char end[] = "1993-12-31";

  Q10_rtype result = Q10(m, start, end);
					    
  double ret_tm = t.stop();
  if (query_out) cout << "Q10 : " << ret_tm << endl;

  if (verbose) {
    Q10_relt r1 = result[0];
    cout << "Q10:" << endl;
    cout << get<0>(r1) << ", "
	 << get<1>(r1) << ", "
	 << get<2>(r1) << ", "
	 << get<3>(r1) << ", "
	 << get<4>(r1) << ", "
	 << get<5>(r1) << ", "
	 << get<6>(r1) << ", "
	 << get<7>(r1) << endl;
  }
  return ret_tm;
}

