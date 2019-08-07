// 6 possible flags, 6 values each
constexpr int num_rows = 6;
constexpr int num_vals = 6;
using Q1_row = array<double, num_vals>;
using Q1_rtype = array<Q1_row, num_rows>;

Q1_rtype Q1(maps m, const char* end_date) {
  ship_map ship_range = ship_map::upTo(m.sm, Date(end_date));

  //cout << "Q1: m.sm ref count: " << m.sm.root->ref_cnt << endl;
  using Add = Add_Nested_Array<Q1_rtype>;

  auto date_f = [] (ship_map::E& e) -> Q1_rtype {
    auto li_f = [] (Q1_rtype& a, Lineitem& l) {
      int row =(l.flags.as_int() & 7);
      double quantity = l.quantity();
      double price = l.e_price;
      double disc = l.discount.val();
      double disc_price = price * (1 - disc);
      double disc_price_taxed = disc_price * (1 + l.tax.val());
      a[row][0] += quantity;
      a[row][1] += price;
      a[row][2] += disc_price;
      a[row][3] += disc_price_taxed;
      a[row][4] += disc;
      a[row][5] += 1.0;
    };
    return li_map::semi_map_reduce(e.second, li_f, Add(), 2000);
  };

  return ship_map::map_reduce(ship_range, date_f, Add(), 1);
}

double Q1time(maps m, bool verbose) {
  timer t;
  t.start();
  const char end[] = "1998-09-01";

  Q1_rtype result = Q1(m, end);

  double ret_tm = t.stop();
  if (query_out) cout << "Q1 : " << ret_tm << endl;

  if (verbose) {
    cout << "Q1:" << endl;
    int i = lineitem_flags('F','A','0','0').as_int();
    double quantity = result[i][0];
    double price = result[i][1];
    double disc_price = result[i][2];
    double disc_price_taxed = result[i][3];
    double disc = result[i][4];
    double count = result[i][5];
    cout << "A, F" << ", " 
	 << quantity << ", "
	 << price << ", "
	 << disc_price << ", "
	 << disc_price_taxed << ", "
	 << quantity/count << ", "
	 << price/count << ", "
	 << disc/count << ", "
	 << (size_t) count 
	 << endl;
  }
  return ret_tm;
}

// Old versions below here
/* 
#include <cilk/cilk.h>
#include <cilk/reducer.h>

// using cilkplus reducer
struct sums_view {
  static constexpr int n = 24;
  using value_type = std::array<float,n>;
  value_type s ;
  sums_view() {for (size_t i=0; i < n ; i++) s[i] = 0.0;}
  void reduce(sums_view* right) {
    for (size_t i=0; i < n; i++) s[i] += right->s[i];
  }
  void add_value(size_t i, float a, float b, float c, float d) {
    int o = i*4;
    s[o] += a; s[o+1] += b; s[o+2] += c; s[o+3] += d;}
  value_type view_get_value() const { return s; }
};

using sums_reducer = cilk::reducer<cilk::monoid_with_view<sums_view>>;

double Q1y(maps m, bool verbose) {
  timer t;
  t.start();
  char end[] = "1998-09-01";
  ship_map ship_range = ship_map::upTo(m.sm, Date(end));

  sums_reducer s;

  auto date_f = [&] (ship_map::E& e) -> int {
    auto li_f = [&] (Lineitem& l) -> int {
      float price = l.e_price;
      float quantity = l.quantity();
      float disc_price = price * (1 - l.discount.val());
      float disc_price_taxed = disc_price * (1 + l.tax.val());
      int flag = l.flags.as_int() & 7;
      s->add_value(flag, price, quantity, disc_price, disc_price_taxed);
      return 1;
    };
    return li_map::map_reducef(e.second, li_f, plus_int, 0);
  };
  ship_map::map_reducef(ship_range, date_f, plus_int, 0, 1);
  std::array<float,24> result = s.get_value();
  double ret_tm = t.stop();
  if (query_out) cout << "Q1 : " << ret_tm << endl;
  if (verbose) cout << result[0] << endl;
  return ret_tm;
}



double Q1t(maps m, bool verbose) {
  timer t;
  t.start();
  char end[] = "1998-09-01";
  ship_map ship_range = ship_map::upTo(m.sm, Date(end));

  using rtype = array<float,24>;

  auto date_f = [&] (ship_map::E& e) -> rtype {
    rtype a;
    for (size_t i = 0; i<24; i++) a[i] = 0.0;
    auto add_value = [&] (Lineitem& l) -> void {
      int o = 4 * (l.flags.as_int() & 7);
      float price = l.e_price;
      float quantity = l.quantity();
      float disc_price = price * (1 - l.discount.val());
      float disc_price_taxed = disc_price * (1 + l.tax.val());
      a[o] += price;
      a[o+1] += quantity;
      a[o+2] += disc_price;
      a[o+3] += disc_price_taxed;
    };
    li_map::foreach_seq(e.second, add_value);
    return a;
  };

  auto add_sum = [] (rtype a, rtype b) {
    for (size_t i = 0; i<24; i++) a[i] += b[i];
    return a;
  };
  rtype i;

  rtype result = ship_map::map_reducef(ship_range, date_f, add_sum, i, 1);
  double ret_tm = t.stop();
  if (query_out) cout << "Q1 : " << ret_tm << endl;
  if (verbose) cout << result[0] << endl;
  return ret_tm;
}

double Q1o(maps m, bool verbose) {
  timer t;
  t.start();
  char end[] = "1998-09-01";
  ship_map ship_range = ship_map::upTo(m.sm, Date(end));

  using elt = tuple<price_t,quantity_t,Percent,Percent, uchar>;
  auto get_elt = [] (Lineitem l) -> elt {
    return elt(l.e_price,l.quantity(),l.discount,l.tax,l.flags.as_int() & 7);};
  sequence<elt> elts = flatten<elt>(ship_range, get_elt);
  
  using sum = tuple<float,float,float,float>;

  auto value = [&] (size_t i) -> sum {
    elt x = elts[i];
    float price = get<0>(x);
    float quantity = get<1>(x);
    float disc_price = price * (1 - get<2>(x).val());
    float disc_price_taxed = disc_price * (1 + get<3>(x).val());
    return sum(quantity, price, disc_price, disc_price_taxed);
  };
  auto values = make_sequence<sum>(elts.size(), value);

  auto key = [&] (size_t i) -> uchar {return get<4>(elts[i]);};
  auto keys = make_sequence<uchar>(elts.size(), key);

  auto add = [] (sum a, sum b) -> sum {
    return sum(get<0>(a)+get<0>(b), get<1>(a)+get<1>(b),
	       get<2>(a)+get<2>(b), get<3>(a)+get<3>(b));
  };

  sum identity(0.0,0.0,0.0,0.0);

  sequence<sum> result =
    pbbs::collect_reduce<sum>(values, keys, lineitem_flags::num, identity, add);
  double ret_tm = t.stop();
  if (query_out) cout << "Q1 : " << ret_tm << endl;

  if (false && verbose) {
    for (size_t i =0; i < result.size(); i++) {
      sum a = result[i];
      lineitem_flags fl = lineitem_flags(i);
      if (get<0>(a) > 0.0) 
	cout << fl.linestatus() << ", "
	     << fl.returnflag() << ", "
	     << get<0>(a) << ", "
	     << get<1>(a) << ", "
	     << get<2>(a) << ", "
	     << get<3>(a) << endl;
    }
  }
  //cout << "out size = " << elts.size() << " buckets:" << result.size() << "\n" << endl;
  return ret_tm;
}
*/
