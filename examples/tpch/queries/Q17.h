using Q17_rtype = double;

Q17_rtype Q17(maps m, const char* brand, const char* container) {
  using int_pair = pair<int, int>;
  using AddI = Add<int>;
  using AddD = Add<double>;
  using AddP = Add_Pair<AddI,AddI>;
 
  auto part_f = [&] (part_to_supp_map::E& e) -> double {
    Part& p = e.second.first;

    // if correct part brand and container
    if (strcmp(p.brand(), brand) == 0 &&
	strcmp(p.container(), container) == 0) {

      // calculate average quantity
      auto supp_f1 = [&] (part_supp_and_item_map::E& e) -> int_pair {
	line_item_set& lm = e.second.second;
	auto li_f1 = [&] (line_item_set::E& l) -> int {
	  return l.quantity();};
	int v = line_item_set::map_reduce(lm, li_f1, AddI());
	return int_pair(v, lm.size());
      };
      part_supp_and_item_map& simap = e.second.second;
      int_pair x = part_supp_and_item_map::map_reduce(simap, supp_f1, AddP());
      double threshold = .2 * ((double) x.first) / (double) x.second;

      // calculate total extended price if < 20% of average quantity
      auto supp_f2 = [&] (part_supp_and_item_map::E& e) {
	line_item_set& lm = e.second.second;
	auto li_f2 = [=] (line_item_set::E& l) -> double {
	  return (((double) l.quantity()) < threshold) ? l.e_price : 0.0;};
	return line_item_set::map_reduce(lm, li_f2, AddD());
      };
      return part_supp_and_item_map::map_reduce(simap, supp_f2, AddD());
    } else return 0.0;
  };
  
  double total = part_to_supp_map::map_reduce(m.suppliers_for_part, part_f, AddD());
  return total/7.0;
}

double Q17time (maps m, bool verbose) {
  timer t;
  t.start();
  const char brand[] = "Brand#23";
  const char container[] = "MED BOX";

  Q17_rtype result = Q17(m, brand, container);

  double ret_tm = t.stop();
  if (QUERY_OUT) cout << "Q17 : " << ret_tm << endl;
  
  if (verbose) {
    cout << "Q17:" << endl << result << endl;
  }
  
  return ret_tm;
}
