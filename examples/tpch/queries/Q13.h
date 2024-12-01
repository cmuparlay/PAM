using Q13_elt = pair<int,int>;
using Q13_rtype = sequence<Q13_elt>;

Q13_rtype Q13(maps m, const char* word1, const char* word2) {
  constexpr int num_buckets = 128;
  using T = array<double, num_buckets>;

  auto customer_f = [&] (T& a, customer_map::E& ce) {
    auto order_f = [&] (order_map::E& oe) {
      char* comment = oe.second.first.comment();
      char* s1 = strstr(comment, word1);
      return (s1 == nullptr || strstr(s1, word2) == nullptr);
    };
    order_map& omap = ce.second.second;
    int t = order_map::map_reduce(omap, order_f, Add<int>());
    a[t]++;
  };

  T bucket_sums =
    customer_map::semi_map_reduce(m.orders_for_customer, customer_f, Add_Array<T>(), 20);
  
  using rt = Q13_elt;
  auto val = [&] (size_t i) {return rt(i+1, bucket_sums[i+1]);};
  auto x = parlay::delayed_seq<rt>(num_buckets-1, val);
  auto less = [] (rt a, rt b) {return a.second > b.second;};
  return parlay::sort(x, less);
}

double Q13time(maps m, bool verbose) {
  timer t;
  t.start();
  const char word1[] = "special";
  const char word2[] = "requests";

  Q13_rtype result = Q13(m, word1, word2);
    
  double ret_tm = t.stop();
  if (QUERY_OUT) cout << "Q13 : " << ret_tm << endl;

  if (verbose) {
    Q13_elt r = result[0];
    cout << "Q13:" << endl
	 << r.first << ", "
	 << r.second << endl;
  }
  return ret_tm;
}
