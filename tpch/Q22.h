using Q22ifpair = pair<int,double>;
pbbs::sequence<Q22ifpair> Q22(maps m, bool verbose, uchar* countrycode) {
  auto belong_country = [&] (uchar x) -> bool {
    for (int i = 0; i < 7; i++)
      if (x == countrycode[i]) return true;
    return false;
  };
  
  auto get_countrycode = [&] (char* phone) -> uchar {
	if (!phone) return 0;
    return (phone[0]-48)*10+phone[1]-48;
  };

  // c_acctbal, countrycode, size = 0
  using a_type = tuple<float, uchar, bool>;
  pbbs::sequence<a_type> res(m.cm.size());

  // write info to a sequence
  auto map_res = [&] (customer_map::E& e, size_t i) -> void {
    Customer& c = e.second.first;
    uchar cc = get_countrycode(c.phone());
    res[i] = a_type(c.acctbal, cc, e.second.second.size() == 0);
  };
  customer_map::map_index(m.cm, map_res);

  // filter to keep those who belong to the right country
  auto is_country = [&] (a_type e) -> bool {
    return belong_country(get<1>(e)); };
  pbbs::sequence<a_type> a = pbbs::filter(res, is_country);

  // find average non-zero balance
  using fip = pair<float, int>;
  fip empty(0.0, 0);
  auto map_bal = [&] (size_t i) -> fip {
    float balance = get<0>(a[i]);
    return (balance > epsilon) ? fip(balance, 1) : fip(0.0, 0);
  };
  auto vals = pbbs::delayed_seq<fip>(a.size(), map_bal);
  fip ave = pbbs::reduce(vals, pbbs::pair_monoid(addm<float>(), addm<int>()));
  float avebal = ave.first / ((float) ave.second);

  // only keep if balance above average, and no orders
  auto keep = [=] (a_type x) -> bool {
    return (get<0>(x) > avebal && get<2>(x)); };
  pbbs::sequence<a_type> r = pbbs::filter(a, keep);

  // collect by phone prefix
  auto key = [&] (a_type v) -> int {if (get<1>(v) > 31) abort(); return get<1>(v);};
  auto val = [&] (a_type v) -> Q22ifpair {return Q22ifpair(1, get<0>(v));};
  pbbs::sequence<Q22ifpair> sums =
    pbbs::collect_reduce(r, key, val,
			 pbbs::pair_monoid(pbbs::addm<int>(),
					   pbbs::addm<double>()),
			 32);

  return sums;
}

double Q22time(maps m, bool verbose) {
  timer t;
  t.start();
  
  uchar countrycode[7] = {13,31,23,29,30,18,17};
  
  pbbs::sequence<Q22ifpair> sums = Q22(m, verbose, countrycode);
  if (verbose) {
    Q22ifpair x = sums[13];
    cout << "Q22:" << endl
  	 << "13, "
  	 << x.first << ", "
  	 << x.second << endl;
   }
  double ret_tm = t.stop();
  if (query_out) cout << "Q22 : " << ret_tm << endl;
  return ret_tm;
}
