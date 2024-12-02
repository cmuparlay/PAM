// mkt_share for two years: 1995 and 1996
using Q8_rtype = pair<double, double>;

Q8_rtype Q8(maps m, const size_t regionk, const size_t nationk,
	    const char* type) {
  using double_pair = pair<double,double>;
  using sums = pair<double_pair, double_pair>;
  using AddD = Add<double>;
  using AddP = Add_Pair<AddD,AddD>; // adds pairs of doubles
  using AddPP = Add_Pair<AddP,AddP>;  // adds pairs of pairs of doubles
  sums identity = AddPP::identity();
  size_t max_order_key = (*(m.line_items_for_order.last())).first;

  // for each order, if it is one of the two years and its customer is in
  // the right region, write the year into order_year (indexed by order_key).
  sequence<uchar> order_year(max_order_key+1, (uchar) 255);
  auto cust_f = [&] (customer_map::E& ce, size_t i) {
    order_map& om = ce.second.second;
    Customer& c = ce.second.first;

    if (nations[c.nationkey].regionkey == regionk) {
      auto order_f = [&] (order_map::E& oe, size_t j) {
	Orders& o = oe.second.first;
	Date d = o.order_date;
	int ok = o.order_key;
	int year = d.year();
	if (year == 1995 || year == 1996)
	  order_year[ok] = year - d.start_year;
      };
      order_map::map_index(om, order_f);
    }
  };
  customer_map::map_index(m.orders_for_customer, cust_f);

  // for each part 
  auto part_f = [&] (part_to_supp_map::E& pe) -> sums {
    part_supp_and_item_map& supps = pe.second.second;
    char* x = pe.second.first.type();

    // check that part is correct type
    // about 1/150 will pass this test
    if (strcmp(type, x) == 0) {

      auto supp_f = [&] (part_supp_and_item_map::E& se) -> sums {
	line_item_set& items = se.second.second;
	dkey_t suppkey = se.second.first.supplier_key;
	Supplier& s = static_data.all_supp[suppkey];

	// for each lineitem of the part/supplier
	auto item_f = [&] (sums& pp, line_item_set::E& l) {
	  int oy = order_year[l.order_key] + Date::start_year;
	  if (oy == 1995 || oy == 1996) {
	    double rev = l.e_price * (1.0 - l.discount.val());
	    double_pair p = ((s.nationkey == nationk) ? double_pair(rev,rev)
			    : double_pair(0.0,rev));
	    if (oy == 1995) pp.first = AddP::add(pp.first, p);
	    else pp.second = AddP::add(pp.second, p);
	  }
	};
	return line_item_set::semi_map_reduce(items, item_f, AddPP());
      };
      return part_supp_and_item_map::map_reduce(supps, supp_f, AddPP());
    } else return identity;
  };

  sums r = part_to_supp_map::map_reduce(m.suppliers_for_part, part_f, AddPP());
  return make_pair(r.first.first/r.first.second,
		   r.second.first/r.second.second);
}

double Q8time(maps m, bool verbose) {
  timer t;
  t.start();
  //const char region[] = "AMERICA";
  const int regionk = 1;
  //const char nation[] = "BRAZIL";
  const int nationk = 2;
  const char type[] = "ECONOMY ANODIZED STEEL";

  Q8_rtype result = Q8(m, regionk, nationk, type);

  double ret_tm = t.stop();
  if (QUERY_OUT) cout << "Q8 : " << ret_tm << endl;
  
  if (verbose) {
    cout << "Q8:" << endl << "1995, " << result.first << endl;
  }
  return ret_tm;
}

Q8_rtype Q8_(maps m, const size_t regionk, const size_t nationk, const char* type) {
  const char start[] = "1995-01-01";
  const char end[] = "1996-12-31";

  using double_pair = pair<double,double>;
  using sums = pair<double_pair, double_pair>;
  using AddD = Add<double>;
  using AddP = Add_Pair<AddD,AddD>; // adds pairs of doubles
  using AddPP = Add_Pair<AddP,AddP>;  // adds pairs of pairs of doubles
  sums identity = AddPP::identity();

  // for each part 
  auto cust_f = [&] (customer_map::E& ce) -> sums {
    order_map& om = ce.second.second;
	Customer& c = ce.second.first;

    if (nations[c.nationkey].regionkey == regionk) {
      auto order_f = [&] (order_map::E& oe) -> sums {
		line_item_set& items = oe.second.second;
		Orders& o = oe.second.first;
		Date d = o.order_date;
		if (Date::less(Date(start),d) && Date::less(d, Date(end))) {
			auto item_f = [&] (line_item_set::E& l) -> sums {
				double rev = 0.0;
				dkey_t suppkey = l.supplier_key;
				dkey_t partkey = l.part_key;
				Supplier& supp = static_data.all_supp[suppkey];
				Part& part = static_data.all_part[partkey];
				char* x = part.type();
				if (strcmp(type, x) == 0) {
					rev = l.e_price * (1.0 - l.discount.val());
					double_pair p = ((supp.nationkey == nationk) ? double_pair(rev,rev)
					  : double_pair(0.0,rev));
					if (d.year() == 1995) return sums(p, double_pair(0.0,0.0));
					else return sums(double_pair(0.0,0.0), p);
				} else return identity;
			};
			return line_item_set::map_reduce(items, item_f, AddPP());
		} else return identity;
	  };
	  return order_map::map_reduce(om, order_f, AddPP());
    } else return identity;
  };

  sums r = customer_map::map_reduce(m.orders_for_customer, cust_f, AddPP());
  return make_pair(r.first.first/r.first.second,
		   r.second.first/r.second.second);
}

Q8_rtype Q8__(maps m, const size_t regionk, const size_t nationk, const char* type) {
  const char start[] = "1995-01-01";
  const char end[] = "1996-12-31";
  Date sdate = Date(start);
  Date edate = Date(end);

  using double_pair = pair<double,double>;
  using sums = pair<double_pair, double_pair>;
  using AddD = Add<double>;
  using AddP = Add_Pair<AddD,AddD>; // adds pairs of doubles
  using AddPP = Add_Pair<AddP,AddP>;  // adds pairs of pairs of doubles
  sums identity = AddPP::identity();
  
  o_order_map orange = o_order_map::range(m.orders_for_date, sdate, edate);
  
  sequence<bool> if_region(CUSTOMER_NUM+1);
  auto customer_region = [&] (customer_map::E& c, size_t i) {
	  dkey_t nkey = c.second.first.nationkey;
	  if (nations[nkey].regionkey == regionk) if_region[c.second.first.custkey] = true; else if_region[c.second.first.custkey] = false;
  };
  customer_map::map_index(m.orders_for_customer, customer_region);

  // for each date
  auto odate_f = [&] (o_order_map::E& ooe) -> sums {
    order_map& om = ooe.second;
    auto order_f = [&] (order_map::E& oe) -> sums {
		line_item_set& items = oe.second.second;
		Orders& o = oe.second.first;
		Date d = o.order_date;
		if (if_region[o.customer_key]) {
			auto item_f = [&] (line_item_set::E& l) -> sums {
				double rev = 0.0;
				dkey_t suppkey = l.supplier_key;
				dkey_t partkey = l.part_key;
				Supplier& supp = static_data.all_supp[suppkey];
				Part& part = static_data.all_part[partkey];
				char* x = part.type();
				if (strcmp(type, x) == 0) {
					rev = l.e_price * (1.0 - l.discount.val());
					double_pair p = ((supp.nationkey == nationk) ? double_pair(rev,rev)
					  : double_pair(0.0,rev));
					if (d.year() == 1995) return sums(p, double_pair(0.0,0.0));
					else return sums(double_pair(0.0,0.0), p);
				} else return identity;
			};
			return line_item_set::map_reduce(items, item_f, AddPP());
		} else return identity;
	  };
	  return order_map::map_reduce(om, order_f, AddPP());
  };

  sums r = o_order_map::map_reduce(orange, odate_f, AddPP());
  return make_pair(r.first.first/r.first.second,
		   r.second.first/r.second.second);
}

Q8_rtype Q8___(maps m, const size_t regionk, const size_t nationk, const char* type) {
  const char start[] = "1995-01-01";
  const char end[] = "1996-12-31";

  using double_pair = pair<double,double>;
  using sums = pair<double_pair, double_pair>;
  using AddD = Add<double>;
  using AddP = Add_Pair<AddD,AddD>; // adds pairs of doubles
  using AddPP = Add_Pair<AddP,AddP>;  // adds pairs of pairs of doubles
  sums identity = AddPP::identity();
  
  sequence<bool> if_region(CUSTOMER_NUM+1);
  auto customer_region = [&] (customer_map::E& c, size_t i) {
    dkey_t nkey = c.second.first.nationkey;
    if (nations[nkey].regionkey == regionk) if_region[c.second.first.custkey] = true; else if_region[c.second.first.custkey] = false;
  };
  customer_map::map_index(m.orders_for_customer, customer_region);


  // sequence<int> odate(CURR_ORDER + 1);
  // sequence<dkey_t> cust(CURR_ORDER + 1);
  // auto odate_f = [&] (order_map::E& oe, size_t i) {
  // Date d = oe.second.first.orderdate;
  // odate[oe.first] = d.year();
  // cust[oe.first] = oe.second.first.custkey;
  // };
  // order_map::map_index(m.om, odate_f);

  // for each part 
  auto part_f = [&] (part_to_supp_map::E& pe) -> sums {
    part_supp_and_item_map& supps = pe.second.second;
    char* x = pe.second.first.type();

    // check that part is correct type
    // about 1/150 will pass this test
    if (strcmp(type, x) == 0) {

      auto supp_f = [&] (part_supp_and_item_map::E& se) -> sums {
	line_item_set& items = se.second.second;
	//Supplier supp = se.second.first;
	dkey_t suppkey = se.second.first.supplier_key;
	Supplier& supp = static_data.all_supp[suppkey];

	// for each lineitem of the part
	auto item_f = [&] (sums& pp, line_item_set::E& l) {
	  double rev = 0.0;

	  // find the order from lineitem 
	  Orders o = (*(m.line_items_for_order.find(l.order_key))).first;

	  // check that in correct date range
	  // about 2/7 (of the 1/150)
	  Date d = o.order_date;
	  int year = d.year();
	  //if (Date::less(Date(start),d) && Date::less(d, Date(end))) {
	  if ((year == 1995) || (year == 1996)) {

	    // find the customer from order
	    //Customer c = (*m.cm.find(o.custkey)).first;

	    // check that customer from correct region
	    //if (nations[c.nationkey].regionkey == regionk) {
	    if (if_region[o.customer_key]) {
	      rev = l.e_price * (1.0 - l.discount.val());

	      // check that supplier of lineitem from correct nation
	      double_pair p = ((supp.nationkey == nationk) ? double_pair(rev,rev)
			      : double_pair(0.0,rev));
	      if (year == 1995) pp.first = AddP::add(pp.first, p);
	      else pp.second = AddP::add(pp.second, p);
	    }
	  } 
	};
	return line_item_set::semi_map_reduce(items, item_f, AddPP());
      };
      return part_supp_and_item_map::map_reduce(supps, supp_f, AddPP());
    } else return identity;
  };

  sums r = part_to_supp_map::map_reduce(m.suppliers_for_part, part_f, AddPP());
  return make_pair(r.first.first/r.first.second,
		   r.second.first/r.second.second);
}
