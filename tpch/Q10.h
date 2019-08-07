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
  sequence<kf_pair> r = pbbs::collect_reduce_sparse(elts, pbbs::addm<float>());

  // sort by revenue
  auto less = [] (kf_pair a, kf_pair b) {return a.second > b.second;};
  sequence<kf_pair> res = pbbs::sample_sort(r, less);

  // generate results
  auto get_result = [&] (size_t i) {
    dkey_t custkey = res[i].first;
    float revenue = res[i].second;
    Customer c = (*(m.cm.find(custkey))).first;
    char* n_name = nations[c.nationkey].name;
    return Q10_relt(custkey, c.name(), revenue, c.acctbal,
		    n_name, c.address(), c.phone(), c.comment());};
  sequence<Q10_relt> rr(20, get_result);      
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

/*
double Q10old(maps m, bool verbose) {
  timer t;
  t.start();
  {
    const char start[] = "1993-10-01";
    const char end[] = "1993-12-31";

    o_order_map order_range = o_order_map::range(m.oom, Date(start), Date(end));

    auto get_order = [] (o_order_map::E& o) -> order_map {
      return o.second;
    };

    order_map omap =
      o_order_map::map_reducef(order_range, get_order, order_map::join2,
			       order_map());

    using elt = pair<float,dkey_t>;
    auto get_elt = [] (order_map::E& oe, Lineitem& l) -> elt {
      Orders& o = oe.second.first;
      float v = l.e_price*(1 - l.discount.val());
      if (l.flags.returnflag() == 'R') return elt(v, o.custkey);
      else return elt(0.0, o.custkey);
    };
	
	
    sequence<elt> elts = flatten_pair<elt>(omap, get_elt);
	
    auto x = [&] (elt& a) -> bool {return (a.first != 0.0);};
    sequence<elt> elts_f = pbbs::filter(elts, x);
    auto f = [&] (size_t i) {return elts_f[i].second;};
    auto cust_ids = make_sequence<dkey_t>(elts_f.size(), f);
    size_t max_customer_id = pbbs::reduce(cust_ids, [&] (dkey_t a, dkey_t b) {
	return std::max(a,b);});
	
	
    auto get_index = [] (elt& a) {return a.second;};
    auto get_val = [] (elt& a) {return a.first;};
  
    sequence<float> customer_sums =
      pbbs::collect_reduce<float>(elts_f, max_customer_id,
				  get_index, get_val,
				  0.0, plus_float);
					    
    if (verbose) cout << elts_f.size() << endl;
  }
  double ret_tm = t.stop();
  if (query_out) cout << "Q10 : " << ret_tm << endl;
  return ret_tm;
}


double Q10n(maps m, bool verbose) {
  timer t;
  t.start();
  const char start[] = "1993-10-01";
  const char end[] = "1993-12-31";

  struct cr {
    using key_t = dkey_t;  // customer id
    using val_t = float;  // revenue
    static bool comp(key_t a, key_t b) {return a < b;}
  };

  using cr_map = pam_map<cr>;
  cr_map::GC::init();

  auto unionf = [] (cr_map a, cr_map b) {
    return cr_map::map_union(std::move(a),std::move(b), [](float a, float b) {return a+b;});
    //return cr_map::join2(std::move(a),std::move(b));
  };

  o_order_map order_range = o_order_map::range(m.oom, Date(start), Date(end));

  auto order_date_f = [&] (o_order_map::E& oo) {
    auto order_f = [&] (order_map::E& o) {
      auto line_f = [&] (li_map::E& l) -> float {
	if (l.flags.returnflag() == 'R') return l.e_price*(1 - l.discount.val());
	else return 0.0;
      };
      li_map& m = o.second.second;
      float t = li_map::map_reducef(m, line_f, plus_float, 0.0);
      //return t;
      if (t > 0.0) return cr_map(true,make_pair(o.second.first.custkey,t));
      else return cr_map(true);
    };
    return order_map::map_reducef(oo.second, order_f, unionf, cr_map(),10);
    //return order_map::map_reducef(oo.second, order_f, plus_float, 0.0);
  };

  cr_map r = o_order_map::map_reducef(order_range, order_date_f, unionf, cr_map(),1);
  //float r = o_order_map::map_reducef(order_range, order_date_f, plus_float, 0.0, 1);

  //cout << r << endl;
  if (verbose) cout << r.size() << endl;
  double ret_tm = t.stop();
  if (query_out) cout << "Q10n : " << ret_tm << endl;
  return ret_tm;
}

*/
