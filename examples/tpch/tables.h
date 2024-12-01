int CUSTOMER_NUM, PARTSUPP_NUM, CURR_ORDER, ORDER_NUM, NATION_NUM, REGION_NUM, SUPPLIER_NUM, PART_NUM, LINEITEM_NUM;

template <class S>
parlay::sequence<S> read_and_parse(string filename, bool verbose)
{
  // cannot detect end_of_file. So always losing a line at the end.
  auto is_line = [](char c) -> bool
  { return c == '\r' || c == '\n' || c == 0; };
  auto is_item = [](char c) -> bool
  { return c == '|'; };
  auto str = parlay::file_map(filename);
  if (verbose)
  {
    string x = "read ";
    nextTime(x.append(filename));
  }

  auto process_line = [&](auto line)
  { return S(parlay::tokens(line, is_item)); };

  auto entries = parlay::map_tokens(str, process_line, is_line);
  cout << "total lines: " << entries.size() << endl;
  return entries;
}

struct LineItemEntry
{
  using key_t = LineItem;
  static bool comp(const key_t &a, const key_t &b)
  {
    return ((a.order_key < b.order_key) ||
            (a.order_key == b.order_key && a.linenumber < b.linenumber));
  }
};

using line_item_set = pam_set<LineItemEntry>;
using receipt_map = date_map<line_item_set>;
using ship_map = date_map<line_item_set>;
using order_map = keyed_map<pair<Orders, line_item_set>>;
using customer_map = keyed_map<pair<Customer, order_map>>;
using o_order_map = date_map<order_map>;
using just_order_map = keyed_map<Orders>;

using part_supp_map = paired_key_map<part_supp>;
using part_supp_and_item_map = paired_key_map<pair<part_supp, line_item_set>>;
using part_to_supp_map = keyed_map<pair<Part, part_supp_and_item_map>>;
using supp_to_part_map = keyed_map<pair<Supplier, part_supp_and_item_map>>;

struct maps
{
  /* Maps every order to its line items */
  order_map line_items_for_order;
  /* Maps every date to shipments made on that date */
  ship_map shipments_for_date;
  /* Maps every date to line items sold on that date */
  receipt_map receipts_for_date;
  /* Maps every customer to orders placed by that customer */
  customer_map orders_for_customer;
  /* Maps every date to orders placed on that date */
  o_order_map orders_for_date;
  part_to_supp_map suppliers_for_part;
  supp_to_part_map parts_for_supplier;
  int version = 0;
  maps() {}
  maps(const maps &m)
  {
    line_items_for_order = m.line_items_for_order;
    shipments_for_date = m.shipments_for_date;
    orders_for_customer = m.orders_for_customer;
    orders_for_date = m.orders_for_date;
    receipts_for_date = m.receipts_for_date;
    suppliers_for_part = m.suppliers_for_part;
    parts_for_supplier = m.parts_for_supplier;
    version = m.version + 1;
  }
  maps &operator=(const maps &m)
  {
    if (this != &m)
    {
      line_items_for_order = m.line_items_for_order;
      shipments_for_date = m.shipments_for_date;
      orders_for_customer = m.orders_for_customer;
      orders_for_date = m.orders_for_date;
      receipts_for_date = m.receipts_for_date;
      suppliers_for_part = m.suppliers_for_part;
      parts_for_supplier = m.parts_for_supplier;
      version = m.version + 1;
    }
    return *this;
  }

  void clear()
  {
    line_items_for_order.clear();
    shipments_for_date.clear();
    orders_for_customer.clear();
    orders_for_date.clear();
    receipts_for_date.clear();
    suppliers_for_part.clear();
    parts_for_supplier.clear();
  }
};

void memory_stats()
{
  cout << "lineitem map:" << endl;
  line_item_set::GC::print_stats();
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

struct arrays_and_temps
{
  Supplier *all_supp;
  Part *all_part;
  part_supp_map psm;
} static_data;

maps make_indices(string dirname, bool verbose)
{
  maps m;
  string lf = dirname;

  timer tx;
  tx.start();

  // ******************************
  cout << "-- lineitems" << endl;
  // ******************************
  string lineitem_fname = lf.append("lineitem.tbl");
  parlay::sequence<LineItem> items = read_and_parse<LineItem>(lineitem_fname, verbose);
  if (verbose)
  {
    nextTime("parse line items");
  }
  LINEITEM_NUM = items.size();
  if (verbose)
  {
    cout << "lineitem size: " << LINEITEM_NUM << endl;
    cout << "largest order in lineitem: " << items[LINEITEM_NUM - 1].order_key << endl;
    cout << "last line number: " << items[LINEITEM_NUM - 1].linenumber << endl;
  }

  line_item_set::reserve(LINEITEM_NUM);
  m.shipments_for_date = secondary_index_o<ship_map>(items, [](LineItem l) -> Date
                                     { return l.ship_date; });
  if (verbose)
    nextTime("shipdate -> lineitem index");

  line_item_set::reserve(LINEITEM_NUM);
  m.receipts_for_date = secondary_index_o<receipt_map>(items, [](LineItem l)
                                        { return l.r_date; });
  if (verbose)
    nextTime("receiptdate -> lineitem index");

  {
    if (verbose)
      cout << endl;

    // ******************************
    cout << "-- orders" << endl;
    // ******************************
    string of = dirname;
    string orders_fname = of.append("orders.tbl");
    parlay::sequence<Orders> orders = read_and_parse<Orders>(orders_fname, verbose);
    ORDER_NUM = orders.size();
    CURR_ORDER = orders[ORDER_NUM - 1].order_key;
    if (verbose)
    {
      cout << "ORDER_NUM: " << ORDER_NUM << endl;
      cout << "CURR_ORDER: " << CURR_ORDER << endl;
      cout << "last customer:" << orders[ORDER_NUM - 1].customer_key << endl;
    }
    if (verbose)
      nextTime("parse orders");

    using ordermap = keyed_map<Orders>;
    using co_li_map = paired_key_map<line_item_set>;
    {
      auto orders_primary = primary_index<ordermap>(orders, [&](Orders o)
                                                    { return o.order_key; });
      if (verbose)
      {
        nextTime("orderkey -> order");
      }

      line_item_set::reserve(LINEITEM_NUM);
      auto comap = secondary_index_o<co_li_map>(items, [&](LineItem l)
                                                { return make_pair((*(orders_primary.find(l.order_key))).customer_key,
                                                                   l.order_key); });
      if (verbose)
      {
        nextTime("<custkey, orderkey> -> lineitem index");
      }

      auto get_key = [&](Orders &o) -> dkey_t
      { return o.order_key; };
      auto get_paired_val = [&](Orders &o) -> line_item_set
      { return comap.find(make_pair(o.customer_key, o.order_key), line_item_set()); };
      m.line_items_for_order = paired_index<order_map>(orders, get_key, get_paired_val);
    }
    ordermap::GC::finish();  // return memory
    co_li_map::GC::finish(); // return memory
    if (verbose)
    {
      nextTime("orderkey -> <order, lineitem index>");
    }

    auto get_o_date =
        m.orders_for_date = secondary_index<o_order_map>(m.line_items_for_order, [](order_map::E e) -> Date
                                             { return e.second.first.order_date; });
    if (verbose)
    {
      nextTime("orderdate -> orderkey -> <order, lineitem index>");
    }

    using cust_map = keyed_map<order_map>;
    cust_map cmap = secondary_index<cust_map>(m.line_items_for_order, [](order_map::E e)
                                              { return ((e.second).first).customer_key; });
    if (verbose)
      nextTime("custkey -> orderkey -> <order, lineitem index>");
    if (verbose)
      cout << endl;

    // ******************************
    cout << "-- customers" << endl;
    // ******************************
    string cf = dirname;
    string customers_fname = cf.append("customers.tbl");
    parlay::sequence<Customer> customers = read_and_parse<Customer>(customers_fname, verbose);
    CUSTOMER_NUM = customers.size();
    if (verbose)
      nextTime("parse customers");

    auto get_key = [&](Customer &c) -> dkey_t
    { return c.custkey; };
    auto get_paired_val = [&](Customer &c) -> order_map
    { return cmap.find(c.custkey, order_map()); };
    m.orders_for_customer = paired_index<customer_map>(customers, get_key, get_paired_val);

    cmap = cust_map();
    cust_map::GC::finish();
    if (verbose)
    {
      nextTime("custkey -> <customer, orderkey -> <order, lineitem index>>");
    }
    if (verbose)
      cout << endl;
  }

  {
    // ******************************
    cout << "-- part_suppliers" << endl;
    // ******************************
    string of = dirname;
    string partsupp_fname = of.append("partsupp.tbl");
    parlay::sequence<part_supp> partsupp = read_and_parse<part_supp>(partsupp_fname,
                                                                     verbose);
    PARTSUPP_NUM = partsupp.size();
    if (verbose)
      nextTime("parse partsupp");

    static_data.psm = primary_index<part_supp_map>(partsupp, [&](part_supp ps) -> pair<dkey_t, dkey_t>
                                                   { return make_pair(ps.part_key, ps.supplier_key); });
    if (verbose)
    {
      nextTime("<partkey,suppkey> -> partsupp index");
    }
    PARTSUPP_NUM = partsupp.size();

    // make map from partsupp to item
    using ps_item_map = paired_key_map<line_item_set>;
    // li_map::reserve(LINEITEM_NUM);
    ps_item_map amap = secondary_index<ps_item_map>(items, [](LineItem l)
                                                    { return make_pair(l.part_key, l.supplier_key); });
    if (verbose)
    {
      nextTime("<partkey,suppkey> -> lineitem index");
    }

    // auto get_inner_key2 = [&] (Part_supp& x) -> key_pair {
    //   return make_pair(x.partkey, x.suppkey);};
    // auto get_inner_val2 = [&] (Part_supp& ps) -> li_map {
    //   return amap.find(make_pair(ps.partkey, ps.suppkey), li_map());
    // };

    // part_supp_and_item_map sptmp = paired_index<part_supp_and_item_map>(partsupp, get_inner_key2, get_inner_val2);

    part_supp_and_item_map sptmp =
        paired_index<part_supp_and_item_map>(static_data.psm, amap);
    if (verbose)
    {
      nextTime("<partkey,suppkey> -> <partsupp, lineitem index>)");
    }
    if (verbose)
      cout << endl;
    amap = ps_item_map();
    ps_item_map::GC::finish();

    using tmp_map = keyed_map<part_supp_and_item_map>;

    {
      // ******************************
      cout << "-- parts" << endl;
      // ******************************
      of = dirname;
      string parts_fname = of.append("part.tbl");
      parlay::sequence<Part> parts = read_and_parse<Part>(parts_fname, verbose);
      PART_NUM = parts.size();
      if (verbose)
        nextTime("parse parts");

      static_data.all_part = new Part[PART_NUM + 1];
      parlay::parallel_for(0, parts.size(), [&](size_t i)
                           { static_data.all_part[parts[i].partkey] = parts[i]; });
      if (verbose)
      {
        nextTime("static parts");
      }

      tmp_map by_part = secondary_index<tmp_map>(sptmp,
                                                 [](part_supp_and_item_map::E e)
                                                 { return e.second.first.part_key; });

      auto part_key = [&](Part &p)
      { return p.partkey; };
      auto part_items = [&](Part &p)
      {
        return by_part.find(p.partkey, part_supp_and_item_map());
      };
      m.suppliers_for_part = paired_index<part_to_supp_map>(parts, part_key, part_items);

      // using part_map = keyed_map<Part>;
      // part_map part_primary
      // 	= primary_index<part_map>(parts, [&] (Part p) {return p.partkey;});
      // tmp_map part_other
      // 	= secondary_index<tmp_map>(sptmp,
      // 				   [] (part_supp_and_item_map::E e) {
      // 				     return e.second.first.partkey;});
      // m.suppliers_for_part = paired_index<part_to_supp_map>(part_primary, part_other);
      if (verbose)
        nextTime("partkey -> <part, <partkey,suppkey> -> <partsupp, lineitem index>>");
      if (verbose)
        cout << endl;
    }

    {
      // ******************************
      cout << "-- suppliers" << endl;
      // ******************************
      of = dirname;
      string supp_fname = of.append("supplier.tbl");
      parlay::sequence<Supplier> supp = read_and_parse<Supplier>(supp_fname, verbose);
      SUPPLIER_NUM = supp.size();
      if (verbose)
        nextTime("parse suppliers");

      static_data.all_supp = new Supplier[SUPPLIER_NUM + 1];
      parlay::parallel_for(0, supp.size(), [&](size_t i)
                           { static_data.all_supp[supp[i].suppkey] = supp[i]; });
      if (verbose)
      {
        nextTime("static suppliers");
      }

      using supp_map = keyed_map<Supplier>;
      supp_map supp_primary = primary_index<supp_map>(supp, [&](Supplier p)
                                                      { return p.suppkey; });
      tmp_map supp_other = secondary_index<tmp_map>(std::move(sptmp),
                                                    [](part_supp_and_item_map::E e)
                                                    { return e.second.first.supplier_key; });
      m.parts_for_supplier = paired_index<supp_to_part_map>(supp_primary, supp_other);
      if (verbose)
        nextTime("suppkey -> <supplier, <partkey,suppkey> -> <partsupp, lineitem index>>");
      if (verbose)
        cout << endl;
    }
  }

  // ******************************
  cout << "-- nations" << endl;
  string of = dirname;
  // ******************************
  string nation_fname = of.append("nation.tbl");
  nations = read_and_parse<Nations>(nation_fname, verbose);
  if (verbose)
    nextTime("parse nations");

  m.version = 0;

  if (verbose)
    cout << endl;
  cout << "read, parsed, and built all indices in "
       << tx.stop() << " seconds" << endl;
  if (verbose)
    cout << endl;
  return m;
}
