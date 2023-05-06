#include "monoid.h"

// 6 possible flags, 6 values each
constexpr int num_rows = 6;
constexpr int num_vals = 6;
using Q1_row = array<double, num_vals>;
using Q1_rtype = array<Q1_row, num_rows>;

Q1_rtype Q1(maps m, const char* end_date) {
  ship_map ship_range = ship_map::upTo(m.sm, Date(end_date));

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
  const char end_date[] = "1998-09-01";

  Q1_rtype result = Q1(m, end_date);

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

