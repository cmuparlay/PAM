// mkt_share for two years: 1995 and 1996
using Q8_rtype = pair<float, float>;

Q8_rtype Q8(maps m, const size_t regionk, const size_t nationk, const char* type) {
  const char start[] = "1995-01-01";
  const char end[] = "1996-12-31";

  using float_pair = pair<float,float>;
  using sums = pair<float_pair, float_pair>;
  using AddD = Add<float>;
  using AddP = Add_Pair<AddD,AddD>; // adds pairs of doubles
  using AddPP = Add_Pair<AddP,AddP>;  // adds pairs of pairs of doubles
  sums identity = AddPP::identity();

  // for each part 
  auto part_f = [&] (part_to_supp_map::E& pe) -> sums {
    part_supp_and_item_map& supps = pe.second.second;
    char* x = pe.second.first.type();

    // check that part is correct type
    // about 1/150 will pass this test
    if (strcmp(type, x) == 0) {

      auto supp_f = [&] (part_supp_and_item_map::E& se) -> sums {
		li_map& items = se.second.second;
		//Supplier supp = se.second.first;
		dkey_t suppkey = se.second.first.suppkey;
		Supplier& supp = static_data.all_supp[suppkey];

		// for each lineitem of the part
		auto item_f = [&] (li_map::E& l) -> sums {
		  float rev = 0.0;

		  // find the order from lineitem 
		  Orders o = (*(m.om.find(l.orderkey))).first;

		  // check that in correct date range
		  // about 2/7 (of the 1/150)
		  Date d = o.orderdate;
		  if (Date::less(Date(start),d) && Date::less(d, Date(end))) {

			// find the customer from order
			Customer c = (*m.cm.find(o.custkey)).first;

			// check that customer from correct region
			if (nations[c.nationkey].regionkey == regionk) {
			  rev = l.e_price * (1.0 - l.discount.val());

			  // check that supplier of lineitem from correct nation
			  float_pair p = ((supp.nationkey == nationk) ? float_pair(rev,rev)
					  : float_pair(0.0,rev));
			  if (d.year() == 1995) return sums(p, float_pair(0.0,0.0));
			  else return sums(float_pair(0.0,0.0), p);
			} else return identity;
		  } else return identity;
		};
		return li_map::map_reduce(items, item_f, AddPP());
      };
      return part_supp_and_item_map::map_reduce(supps, supp_f, AddPP());
    } else return identity;
  };

  sums r = part_to_supp_map::map_reduce(m.psm2, part_f, AddPP());
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
  if (query_out) cout << "Q8 : " << ret_tm << endl;
  
  if (verbose) {
    cout << "Q8:" << endl << "1995, " << result.first << endl;
  }
  return ret_tm;
}

