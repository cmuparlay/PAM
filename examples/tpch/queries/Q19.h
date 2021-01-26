using Q19_rtype = double;

Q19_rtype Q19(maps m, bool verbose, const int* quantity, const char** brand) {
  using AddD = Add<double>;

  auto streq = [] (const char* s1, const char* s2) -> bool {
    return strcmp(s1,s2) == 0;};

  auto is_in = [&] (const char* s1, const char** list, int n) -> bool {
    for (int i=0; i < n; i++) 
      if (streq(s1,list[i])) return true;
    return false;
  };
  
  const char* container[3][4] =
    {{"SM CASE", "SM BOX", "SM PACK", "SM PKG"},
     {"MED BAG", "MED BOX", "MED PKG", "MED PACK"},
     {"LG CASE", "LG BOX", "LG PACK", "LG PKG"}};
  const int size[3] = {5, 10, 15};

  auto f = [&](part_to_supp_map::E& e) -> double {
    Part p = e.second.first;
    part_supp_and_item_map& si = e.second.second;

    auto block = [&] (int high_size, const char** containers,
		      const char* brand, int quantity) -> double {
      if ((p.size >= 1 && p.size <= high_size)
	  && streq(p.brand(), brand)
	  && (is_in(p.container(), containers, 4))) {
	auto g = [&] (part_supp_and_item_map::E& s) -> double {
	  li_map& li = s.second.second;
	  auto h = [&] (Lineitem& l) -> double {
	    if ((l.quantity() >= quantity) &&
		(l.quantity() <= (quantity + 10)) &&
		(l.shipinstruct() == 'D') &&
		((l.shipmode() == 'A'))) // || l.shipmode() == 'G')))
	      return  l.e_price*(1 - l.discount.val());
	    else return 0.0;
	  };
	  return li_map::map_reduce(li, h, AddD());
	};
	return part_supp_and_item_map::map_reduce(si, g, AddD(), 10);
      } else return 0.0;
    };

    double total = 0.0;
    for (int i = 0; i < 3; i++)
      total += block(size[i], container[i], brand[i], quantity[i]);
    return total;
  };
  
  double result = part_to_supp_map::map_reduce(m.psm2, f, AddD(), 1);
  return result;
}

double Q19time(maps m, bool verbose) {
  timer t_q19; t_q19.start();
  const int quantity[3] = {1, 10, 20};
  const char* brand[3] = {"Brand#12", "Brand#23", "Brand#34"};
  
  double result = Q19(m, verbose, quantity, brand);
  
  if (verbose) {
    cout << "Q19:" << endl
	 << result << endl;
  }
  double ret_tm = t_q19.stop();
  if (query_out) cout << "Q19 : " << ret_tm << endl;
  return ret_tm;
}  
  
