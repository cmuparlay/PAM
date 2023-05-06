using Q22_relt = pair<int,double>;
using Q22_rtype = parlay::sequence<Q22_relt>;

using Q22_a_type = tuple<float, uchar, bool>;


//template<typename Monoid>
  struct Q22_helper {
        using key_type = int;
        using val_type = Q22_relt;
        //static auto mon = parlay::pair_monoid(parlay::addm<int>(), parlay::addm<double>());
        //Monoid mon;
        //Q22_helper(Monoid const &m) : mon(m) {};
        static key_type get_key(const Q22_a_type& a) {if (get<1>(a) > 31) abort(); return get<1>(a);}
        static val_type get_val(const Q22_a_type& a) {return Q22_relt(1, get<0>(a));}
        static val_type init() {return Q22_relt();}
        static void update(val_type& d, val_type d2) {
		auto mon = parlay::pair_monoid(parlay::addm<int>(), parlay::addm<double>());
		d = mon(d,d2);}
        static void combine(val_type& d, slice<Q22_a_type*,Q22_a_type*> s) {
          //auto vals = delayed_map(s, [&] (Q22_a_type &v) {return Q22_relt(1, get<0>(v));});
          auto mon = parlay::pair_monoid(parlay::addm<int>(), parlay::addm<double>());
	  d = parlay::internal::reduce(delayed_map(s, [&] (Q22_a_type &v) -> Q22_relt {return Q22_relt(1, get<0>(v));}), mon);
        }
  };


Q22_rtype Q22(maps m, bool verbose, uchar* countrycode) {
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
  parlay::sequence<a_type> res(m.cm.size());

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
  
  parlay::sequence<a_type> a = parlay::filter(res, is_country);

  // find average non-zero balance
  using fip = pair<float, int>;
  fip empty(0.0, 0);
  auto map_bal = [&] (size_t i) -> fip {
    float balance = get<0>(a[i]);
    return (balance > epsilon) ? fip(balance, 1) : fip(0.0, 0);
  };
  auto vals = parlay::delayed_seq<fip>(a.size(), map_bal);
  fip ave = parlay::reduce(vals, parlay::pair_monoid(addm<float>(), addm<int>()));
  float avebal = ave.first / ((float) ave.second);

  // only keep if balance above average, and no orders
  auto keep = [=] (a_type x) -> bool {
    return (get<0>(x) > avebal && get<2>(x)); };
  parlay::sequence<a_type> r = parlay::filter(a, keep);

  // collect by phone prefix
  auto key = [&] (a_type v) -> int {if (get<1>(v) > 31) abort(); return get<1>(v);};
  auto val = [&] (a_type v) -> Q22_relt {
    return Q22_relt(1, get<0>(v));};
  //using Monoid = parlay::pair_monoid<typename addm<int>, typename addm<double>>;
  
  auto mon = parlay::pair_monoid(parlay::addm<int>(), parlay::addm<double>());
  
  parlay::sequence<Q22_relt> sums = 
	  parlay::internal::collect_reduce(r, Q22_helper(), 32);
  //parlay::sequence<Q22_relt> sums =
    //parlay::internal::collect_reduce(r, key, val,
	//		 parlay::pair_monoid(parlay::addm<int>(),
	//				   parlay::addm<double>()),
	//		 32);
  

  return sums;
}

double Q22time(maps m, bool verbose) {
  timer t;
  t.start();
  
  uchar countrycode[7] = {13,31,23,29,30,18,17};
  
  Q22_rtype sums = Q22(m, verbose, countrycode);
  if (verbose) {
    Q22_relt x = sums[13];
    cout << "Q22:" << endl
  	 << "13, "
  	 << x.first << ", "
  	 << x.second << endl;
   }
  double ret_tm = t.stop();
  if (query_out) cout << "Q22 : " << ret_tm << endl;
  return ret_tm;
}
