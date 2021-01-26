
void Q5old(maps m, bool verbose) {
  timer t_q5; t_q5.start();
  const string start = "1994-01-01";
  const string end = "1995-01-01"; 
  Date ostart = Date(start), oend = Date(end);
  int qregion = 2;
  auto f_region = [&](supp_map::E& e) {
    return (nations[e.second.nationkey].regionkey == qregion);
  };
  global_suppm = supp_map::filter(static_data.suppm, f_region);
  int s = m.cm.size();
  pair<dkey_t,float>* ans = new pair<dkey_t,float>[s];

  auto f = [&](customer_map::E& e, size_t i) { // added &
    dkey_t nationid = e.second.first.nationkey;
    if (nations[nationid].regionkey == qregion) {
      order_map om = e.second.second;

      auto from_order = [&] (order_map::E& e) -> float{
	Orders ord = e.second.first;
	if (Date::less(ord.orderdate, ostart) ||
	    Date::less(oend, ord.orderdate))
	  return 0.0;
	else {
	  li_map& li = e.second.second; // added &

	  auto from_li = [&] (li_map::E& e) -> float { 
	    dkey_t suppid = e.suppkey;
	    maybe<Supplier> suppV = global_suppm.find(suppid);
	    if (!suppV || (*suppV).nationkey != nationid) return 0.0;
	    else return e.e_price * e.discount.val();
	  };

	  return li_map::map_reducef(li, from_li, plus_float, 0.0);
	}
      };

      float res = order_map::map_reducef(om, from_order, plus_float, 0.0);
      ans[i] = make_pair(nationid,res);
    } else ans[i] = make_pair(-1, -1);
  };

  customer_map::map_index(m.cm, f, 1);
  //should also be a pack here.
  cout << "Q5 : " << t_q5.stop() << endl;
  delete[] ans;
}

void Q4old(maps m, bool verbose) {
  timer t;
  t.start();
  
  Date q_date1 = Date("1993-07-01"), q_date2 = Date("1993-10-01");
  o_order_map m_range = o_order_map::range(m.oom, q_date1, q_date2);
  size_t s = m_range.size();
  sequence<priority_map> x(s);
  
  auto f = [&] (o_order_map::E e, size_t idx) -> priority_map {
	order_map m0 = e.second;
	size_t s = m0.size();
	sequence<priority_map> a(s);
	auto f2 = [&] (order_map::E e, size_t idx) {
		priority_map ret;
		auto f3 = [&] (li_map::E e) -> bool {
			return Date::less(e.c_date, e.r_date);
		};
		int p = e.second.first.orderpriority;
		if (li_map::if_exist(e.second.second,f3)) {
			a[idx].a[p] = 1;
		}
	};
	order_map::map_index(m0,f2);
	x[idx] = pbbs::reduce(a, add_pri_map);
  };
  o_order_map::map_index(m_range, f);
  
  priority_map ret = pbbs::reduce(x,add_pri_map);
  
  cout << "Q4old: " << t.stop() << endl;
}

void Q14old(maps m, bool verbose) {
  timer t;
  t.start(); 
  Date q_date1 = Date("1995-09-01"), q_date2 = Date("1995-10-01");
  ship_map smap = m.sm;
  ship_map s_range = ship_map::range(smap, q_date1, q_date2);
  auto f_map = [&] (ship_map::E e) -> float {
	  return li_map::map_reducef(e.second, revenue, plus_float, 0.0, 100);
  };
  float total_revenue = ship_map::map_reducef(s_range, f_map, plus_float, 0.0, 1);
  
  auto p_type_revenue = [&] (li_map::E& l) -> float {
	dkey_t t = l.partkey;
	maybe<pair<Part, li_map>> st = m.pm.find(t);
	if (!st) return 0.0;
	char* x = (*st).first.type();
	//cout << x << endl;
	if (x[0] == 'P' && x[1] == 'R' && x[2] == 'O' && x[3] == 'M' && x[4] == 'O') return l.e_price * (1.0 - l.discount.val());
	return 0.0;
  };
  
  auto f_map2 = [&] (ship_map::E& e) -> float {
	  return li_map::map_reducef(e.second, p_type_revenue, plus_float, 0.0, 100);
  };
  
  float part_revenue = ship_map::map_reducef(s_range, f_map2, plus_float, 0.0, 1);

  if (verbose) cout << part_revenue << endl;
  if (verbose) cout << part_revenue/total_revenue*100 << endl;
  cout << "Q14old: " << t.stop() << endl;
}
