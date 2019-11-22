int CUSTOMER_NUM, PARTSUPP_NUM, CURR_ORDER, ORDER_NUM, NATION_NUM, REGION_NUM, SUPPLIER_NUM, PART_NUM, LINEITEM_NUM;

auto sep = [] (char c) -> bool { return c == '|'; };
int total_nation = 25;

template <class S>
sequence<S> read_and_parse(string filename, bool verbose) {
  csv_data d = readCSV(filename, sep, verbose);
  auto parseItem = [&] (size_t i) { return S(d.lines[i]);};
  auto x = sequence<S>(d.lines.size(), parseItem);
  return x;
}

struct li_entry {
  using key_t = Lineitem;
  static bool comp(const key_t& a, const key_t& b) {
    return ((a.orderkey < b.orderkey) ||
	    (a.orderkey == b.orderkey && a.linenumber < b.linenumber));
  }
};

using li_map = pam_set<li_entry>;
using receipt_map = date_map<li_map>;
using ship_map = date_map<li_map>;
using order_map = keyed_map<pair<Orders,li_map>>;
using customer_map = keyed_map<pair<Customer,order_map>>;
using o_order_map = date_map<order_map>;
using just_order_map = keyed_map<Orders>;

using part_supp_map = paired_key_map<Part_supp>;
using part_supp_and_item_map = paired_key_map<pair<Part_supp, li_map>>;
using part_to_supp_map = keyed_map<pair<Part, part_supp_and_item_map>>;
using supp_to_part_map = keyed_map<pair<Supplier, part_supp_and_item_map>>;

struct maps {
  order_map om;
  ship_map sm;
  receipt_map rm;
  customer_map cm;
  o_order_map oom;
  part_to_supp_map psm2;
  supp_to_part_map spm2;
  int version = 0;
  maps() {}
  maps(const maps& m) {
	om = m.om;
	sm = m.sm;
	cm = m.cm;
	oom = m.oom;
	rm = m.rm;
	psm2 = m.psm2;
	spm2 = m.spm2;
	version = m.version+1;
  }
  maps& operator = (const maps& m) {
    if (this != &m) { 
	  om = m.om;
	  sm = m.sm;
	  cm = m.cm;
	  oom = m.oom;
	  rm = m.rm;
	  psm2 = m.psm2;
	  spm2 = m.spm2;
	  version = m.version+1;
	}
    return *this;
  }

  void clear() {
	  om.clear();
	  sm.clear();
	  cm.clear();
	  oom.clear();
	  psm2.clear();
	  spm2.clear();
	  rm.clear();
  }
};

void memory_stats() {
  cout << "lineitem map:" << endl;
  li_map::GC::print_stats();
  cout << "order map:" << endl;
  order_map::GC::print_stats();
  cout << "customer map:" << endl;
  customer_map::GC::print_stats();
  cout << "orderdate map:" << endl;
  o_order_map::GC::print_stats();
  cout << "shipdate map:" << endl;
  ship_map::GC::print_stats();
  cout << "receiptdate map:" << endl;
  receipt_map::GC::print_stats();
  cout << "partsupp and item map:" << endl;
  part_supp_and_item_map::GC::print_stats();
  cout << "part map:" << endl;
  part_to_supp_map::GC::print_stats();
  cout << "supplier map:" << endl;
  supp_to_part_map::GC::print_stats();
}

struct arrays_and_temps {  
  Supplier* all_supp;
  Part* all_part;
  part_supp_map psm;
} static_data;

maps make_maps(string dirname, bool verbose) {
  maps m;
  string lf = dirname;
 
  timer tx; tx.start();
  cout << sizeof(Lineitem) << endl;
  cout << sizeof(Orders) << endl;
  cout << sizeof(Customer) << endl;
  cout << sizeof(Part_supp) << endl;
  cout << sizeof(Part) << endl;
  cout << sizeof(Supplier) << endl;
  
  // lineitems
  cout << "-- lineitems" << endl;

  // first keyed on shipdate
  string lineitem_fname = lf.append("lineitem.tbl");
  sequence<Lineitem> items = read_and_parse<Lineitem>(lineitem_fname, verbose);
  cout << items.size() << endl;
  if (verbose) {nextTime("parse line items");}
    
  m.sm = secondary_index<ship_map>(items, [] (Lineitem l) {
      return l.s_date;}); 
  if (verbose) nextTime("ship_date -> lineitem index");

  m.rm = secondary_index<receipt_map>(items, [] (Lineitem l) {
      return l.r_date;});
  if (verbose) nextTime("receipt_date -> lineitem index");
  
  LINEITEM_NUM = items.size();

  // orders and customers
  { 
    if (verbose) cout << endl;
    cout << "-- orders" << endl;

    string of = dirname;
    string orders_fname = of.append("orders.tbl");
    sequence<Orders> orders = read_and_parse<Orders>(orders_fname, verbose);
	ORDER_NUM = orders.size();
	CURR_ORDER = orders[ORDER_NUM-1].orderkey;
    if (verbose) nextTime("parse orders");

    using ordermap = keyed_map<Orders>;
    using co_li_map = paired_key_map<li_map>;
    {
      auto orders_primary = primary_index<ordermap>(orders, [&] (Orders o) {
	  return o.orderkey;});

      auto comap = secondary_index<co_li_map>(items, [&] (Lineitem l) {
	  return make_pair((*(orders_primary.find(l.orderkey))).custkey,
			   l.orderkey);});

      auto get_key = [&] (Orders& o) -> dkey_t {
	return o.orderkey;};
      auto get_val = [&] (Orders& o) -> li_map {
	maybe<li_map> x = comap.find(make_pair(o.custkey,o.orderkey));
	return x ? *x : li_map();
      };
      m.om = make_paired_index<order_map>(orders, get_key, get_val);
    }
    ordermap::GC::finish(); // return memory
    co_li_map::GC::finish(); // return memory 
    
    auto get_o_date = 
    m.oom = secondary_index<o_order_map>(m.om, [] (order_map::E e) -> Date {
	return e.second.first.orderdate;});
	
    if (verbose) {nextTime("orderkey -> <order,lineitem> index");}

    if (verbose) cout << endl;
    cout << "-- customers" << endl;
    
    using cust_map = keyed_map<order_map>;
    cust_map cmap = secondary_index<cust_map>(m.om, [] (order_map::E e) {
	return ((e.second).first).custkey;});
    if (verbose) nextTime("custkey -> orderkey -> <order,lineitem> index");

    string cf = dirname;
    string customers_fname = cf.append("customers.tbl");
    sequence<Customer> customers
      = read_and_parse<Customer>(customers_fname, verbose);
    CUSTOMER_NUM = customers.size();
    if (verbose) nextTime("parse customers");

    auto a = primary_index<keyed_map<Customer>>(customers, [&] (Customer c) {
	return c.custkey;});

    m.cm = paired_index<customer_map>(a, cmap);
    cmap = cust_map();
    cust_map::GC::finish();
    
    if (verbose) {nextTime("custkey -> <customer, orderkey -> <order, lineitem>> index");}

  }

  
  //part supp and part_supp
  {
    //supp and part_supp
    if (verbose) cout << endl;
    cout << "-- part_suppliers" << endl;
    string of = dirname;
    string partsupp_fname = of.append("partsupp.tbl");
    sequence<Part_supp> partsupp = read_and_parse<Part_supp>(partsupp_fname,
							     verbose);
    PARTSUPP_NUM = partsupp.size();
    if (verbose) nextTime("parse partsupp");
    if (verbose) cout << "partsup size: " << PARTSUPP_NUM << endl;
    
    static_data.psm
      = primary_index<part_supp_map>(partsupp, [&] (Part_supp ps) {
	  return make_pair(ps.partkey, ps. suppkey);});
    
    if (verbose) {nextTime("<partkey,suppkey> -> partsupp index");} 
    PARTSUPP_NUM = partsupp.size();
	
    // make map from partsupp to item	
    using ps_item_map = paired_key_map<li_map>;
    ps_item_map amap = secondary_index<ps_item_map>(items, [] (Lineitem l) {
	return make_pair(l.partkey,l.suppkey);});
    if (verbose) {nextTime("<partkey,suppkey> -> lineitem index");}

    part_supp_and_item_map sptmp =
      paired_index<part_supp_and_item_map>(static_data.psm, amap);
    if (verbose) {nextTime("<partkey,suppkey> -> (partsupp,lineitem index)");}
    
    using tmp_map = keyed_map<part_supp_and_item_map>;
	
    //parts
    {
      if (verbose) cout << endl;
      cout << "-- parts" << endl;
      of = dirname;
      string parts_fname = of.append("part.tbl");
      sequence<Part> parts = read_and_parse<Part>(parts_fname, verbose);
      PART_NUM = parts.size();
      if (verbose) nextTime("parse parts");

      static_data.all_part = new Part[PART_NUM+1];
      parallel_for (0, parts.size(), [&] (size_t i) {
	  static_data.all_part[parts[i].partkey] = parts[i];
	});
      if (verbose) {nextTime("all parts stored");}  

      cout << "partkey->(part,partsupp->lineitem)" << endl;
      using part_map = keyed_map<Part>;
      part_map part_primary
	= primary_index<part_map>(parts, [&] (Part p) {return p.partkey;});
      tmp_map part_other
	= secondary_index<tmp_map>(sptmp, [] (part_supp_and_item_map::E e) {
	    return e.second.first.partkey;});
      m.psm2 = paired_index<part_to_supp_map>(part_primary, part_other);
      if (verbose) cout << "psm2 size: " << m.psm2.size() << endl;
    }
    
    //suppliers
    {
      if (verbose) cout << endl;
      cout << "-- suppliers" << endl;
      of = dirname;
      string supp_fname = of.append("supplier.tbl");
      sequence<Supplier> supp = read_and_parse<Supplier>(supp_fname, verbose);
      if (verbose) nextTime("parse suppliers");
    
      SUPPLIER_NUM = supp.size();
      static_data.all_supp = new Supplier[SUPPLIER_NUM+1];
      parallel_for (0, supp.size(), [&] (size_t i) {
	  static_data.all_supp[supp[i].suppkey] = supp[i];
	});

      cout << "suppkey->(supplier,partsupp->lineitem)" << endl;
      using supp_map = keyed_map<Supplier>;
      supp_map supp_primary
	= primary_index<supp_map>(supp, [&] (Supplier p) {return p.suppkey;});
      tmp_map supp_other
	= secondary_index<tmp_map>(sptmp, [] (part_supp_and_item_map::E e) {
	    return e.second.first.suppkey;});
      m.spm2 = paired_index<supp_to_part_map>(supp_primary, supp_other);
      if (verbose) cout << "spm2 size: " << m.spm2.size() << endl;
    }
  }

  //nations
  if (verbose) cout << endl;
  cout << "-- nations" << endl;  string of = dirname;
  
  string nation_fname = of.append("nation.tbl");
  nations = read_and_parse<Nations>(nation_fname, verbose);
  m.version = 0;
  cout << "build all index in time: " << tx.stop() << endl;
  
  return m;
}

auto revenue = [] (li_map::E& l) -> float {
  return l.e_price * (1.0 - l.discount.val());
};

auto plus_float = [] (float a, float b) -> float {return float(a + b);};
auto plus_double = [] (double a, double b) -> double {return double(a + b);};
auto plus_size_t = [] (size_t a, size_t b) {return a + b;};
auto plus_int = [] (int a, int b) {return a + b;};
template <class P>
P plus_pair(P a, P b) {
  return make_pair(a.first+b.first, a.second+b.second);
}

template <class T>
T plus_f(T a, T b) {return a + b;}

template <class T>
T max_f(T a, T b) {return std::max(a,b);}

template <class T>
T min_f(T a, T b) {return std::min(a,b);}

auto min_float = [] (float a, float b) -> float {return std::min(a,b);};

