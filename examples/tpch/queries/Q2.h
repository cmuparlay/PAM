#include <limits>

// s_acctbal, s_name, n_name, p_partkey, p_mfgr, s_address, s_phone, s_comment
using Q2_elt = tuple<double, char *, char *, dkey_t, char *, char *, char *, char *>;
using Q2_rtype = sequence<Q2_elt>;

constexpr double EPS = 0.001;

bool equal(double a, double b)
{
  return std::abs(a - b) < EPS;
}

Q2_rtype Q2(maps m, unsigned int rid, int size, char *type)
{

  size_t typelen = strlen(type);
  using ps_map = part_to_supp_map;
  using s_map = part_supp_and_item_map;

  auto x = ps_map::map_filter(m.suppliers_for_part, [&](ps_map::E &pe) -> std::optional<ps_map::V> {
    auto &[p, sim] = pe.second;
    char *t = p.type();
    // if not correct size and type, filter out
    if (p.size != size || strcmp(t + strlen(t) - typelen, type) != 0)
      return {};
    // for each supplier in correct region return ps cost
    // find the cheapest
    double min_cost = s_map::map_reduce(sim, [&](s_map::E &se) {
      // Supplier& s = se.second.first.second;
      dkey_t suppkey = se.second.first.supplier_key;
      Supplier &s = static_data.all_supp[suppkey];
      if (nations[s.nationkey].regionkey != rid)
      {
        return std::numeric_limits<double>::infinity();
      }
      return se.second.first.supply_cost;
    }, Min<double>());

    // filter those in correct region that equal cheapest
    s_map top = s_map::filter(sim, [&](s_map::E &se)
    {
      // for each supplier in correct region check if cheapest
      // Supplier& s = se.second.first.second;
      dkey_t suppkey = se.second.first.supplier_key;
      Supplier &s = static_data.all_supp[suppkey];
      return (nations[s.nationkey].regionkey == rid &&
              se.second.first.supply_cost == min_cost);
    });

    if (top.is_empty()) 
    {
      return {};
    }
    return std::make_pair(p, top); 
  }, 10);

  using elt = Q2_elt;

  // selected values
  sequence<elt> elts = flatten_pair<elt>(x, [&](ps_map::E pe, s_map::E se) -> elt {
    Part p = pe.second.first;
    dkey_t suppkey = se.second.first.supplier_key;
    Supplier s = static_data.all_supp[suppkey];
    char *n_name = nations[s.nationkey].name;
    // s.acctbal
    return {s.acctbal, s.name(), n_name, p.partkey, p.mfgr(),
               s.address(), s.phone(), s.comment()};
  });

  // sort by output ordering: s_acctbal (desc), n_name, s_name, p_partkey
  return parlay::sort(elts, [](elt a, elt b) {
    // return (get<0>(a) > get<0>(b));
    if (get<0>(a) > get<0>(b))
      return true;
    if (get<0>(a) < get<0>(b))
      return false;
    int cmp_n_name = strcmp(get<2>(a), get<2>(b));
    if (cmp_n_name < 0)
      return true;
    if (cmp_n_name > 0)
      return false;
    int cmp_s_name = strcmp(get<1>(a), get<1>(b));
    if (cmp_s_name < 0)
      return true;
    if (cmp_s_name > 0)
      return false;
    return get<3>(a) < get<3>(b);
  });
}

double Q2time(maps m, bool verbose)
{
  timer t;
  t.start();
  unsigned int rid = 3;
  int size = 15;
  char type[] = "BRASS";

  Q2_rtype result = Q2(m, rid, size, type);
  // cout << result.size() << endl;

  double ret_tm = t.stop();
  if (QUERY_OUT)
    cout << "Q2 : " << ret_tm << endl;

  if (verbose)
  {
    const auto &r = result.front();
    cout << "Q2: " << endl
        << get<0>(r) << ", " << get<1>(r) << ", "
        << get<2>(r) << ", " << get<3>(r) << ", "
        << get<4>(r) << ", " << get<5>(r) << ", "
        << get<6>(r) << ", " << get<7>(r) << endl;
  }
  return ret_tm;
}
