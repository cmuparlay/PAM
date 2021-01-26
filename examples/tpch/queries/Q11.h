using ftype = double;
using Q11_elt = pair<dkey_t, ftype>;
using Q11_rtype = parlay::sequence<Q11_elt>;

Q11_rtype Q11(maps m, uint nation_id, double fraction) {
  using kf_pair = Q11_elt;
  size_t max_part_key = (*(m.psm2.last())).first;
  parlay::sequence<ftype> sums = parlay::sequence<ftype>(max_part_key,
						     (ftype) 0.0);
    
  auto supp_f = [&] (supp_to_part_map::E& se, size_t i) -> void {
    Supplier& s = se.second.first;
    if (s.nationkey == nation_id) {
      auto part_f = [&] (part_supp_and_item_map::E& pe, size_t j) -> void {
	Part_supp ps = pe.second.first;
	utils::write_add(&sums[ps.partkey], ps.availqty* (ftype) ps.supplycost);
      };
      part_supp_and_item_map::map_index(se.second.second, part_f);
    }
  };
  supp_to_part_map::map_index(m.spm2, supp_f);

  auto rval = [&] (size_t i) {return kf_pair(i, sums[i]);};
  auto p = parlay::delayed_seq<kf_pair>(max_part_key, rval);
  auto flag = [&] (size_t i) {return sums[i] > 0.0;};
  auto fl = parlay::delayed_seq<bool>(max_part_key, flag);
  parlay::sequence<kf_pair> r = parlay::pack(p, fl);
  size_t n = r.size();

  auto second_f = [&] (size_t i) -> ftype {return r[i].second;};
  double total = parlay::reduce(parlay::delayed_seq<ftype>(n, second_f),
			      parlay::addm<ftype>());
  double cutoff = fraction*total;
      
  // only keep entries beyond a fraction of total
  // r is a sequence of <part,total_value> pairs
  auto big_enough = parlay::tabulate(n, [&] (size_t i) -> bool {
      return r[i].second > cutoff;});
  parlay::sequence<kf_pair> r2 = parlay::pack(r, big_enough);
 
  auto less = [] (kf_pair a, kf_pair b) -> bool {
    return a.second > b.second;};
  return parlay::sort(r2, less);
}

double Q11time(maps m, bool verbose) {
  timer t;
  t.start();
  // const char nation[] = "GERMANY";
  unsigned int nation_id = 7;
  double fraction = .0001;
  Q11_rtype result = Q11(m, nation_id, fraction);
  double ret_tm = t.stop();
  if (query_out) cout << "Q11 : " << ret_tm << endl;
  
  if (verbose) {
    cout << "Q11:" << endl;
    if (result.size() == 0) cout << "Empty" << endl;
    else cout << result[0].first << ", " << result[0].second << endl;
  }
  return ret_tm;
}

