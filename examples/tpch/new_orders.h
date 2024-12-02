struct transaction
{
	enum class type
	{
		Shipment,
		Payment,
		NewOrder
	};
	transaction(type type, int ind) : type(type), ind(ind) {}
	type type;
	int ind;
};

struct new_order_entry
{
	Orders orders;
	unsigned int num_items;
	parlay::sequence<LineItem> items;
	new_order_entry(int idx, const maps &m) : num_items(random_hash('n', idx, 1, 8))
	{
		int customer_id = random_hash('c', idx, 0, CUSTOMER_NUM) + 1;
		orders.order_key = CURR_ORDER + idx;
		orders.customer_key = customer_id;
		orders.order_date = get_today();
		orders.shipment_priority = 0;
		orders.order_priority = 0;
		orders.status = 'A';
		orders.strings = new char[100];
		items.reserve(num_items);
		double total;
		for (size_t i = 0; i < num_items; i++)
		{
			LineItem item;
			int item_id = random_hash('i' + 't' + 'e' + 'm' + idx, i, 0, PARTSUPP_NUM);
			int quantity = random_hash('q' + idx, i, 0, 20) + 1;
			auto ps = *(static_data.psm.select(item_id));
			item.part_key = ps.first.first;
			item.supplier_key = ps.first.second;
			// Part_supp ps = p.second;
			auto part = m.suppliers_for_part.find(items[i].part_key)->first;
			item.order_key = orders.order_key;
			item.c_date = today;
			item.r_date = today;
			item.e_price = part.retailprice * quantity;
			item.linenumber = i + 1;
			int dis = random_hash('d' + idx, i, 0, 10);
			int tax = random_hash('t' + idx, i, 0, 10);
			item.discount = Percent(dis);
			item.tax = Percent(tax);
			total += (items[i].e_price * (100.0 + tax) / 100.0 * (100.0 - dis) / 100.0);
			items.push_back(std::move(item));
		}
		orders.total_price = total;
	}
};

parlay::sequence<new_order_entry> generate_new_orders(int n, const maps &m)
{

	return parlay::tabulate(n, [&](int i) -> new_order_entry
													{ return {i, m}; });
}

struct payment_entry
{
	dkey_t customer_key;
	double amount;
	payment_entry(int idx) : customer_key(random_hash('c', idx, 0, CUSTOMER_NUM) + 1), amount((random_hash('i' + 'n' + 't', idx, 1, 5000) + 0.0) + (random_hash('f' + 'r' + 'a' + 'c', idx, 0, 100) + 0.0) / 100.0)
	{
	}
};

parlay::sequence<payment_entry> generate_payments(int n)
{
	return parlay::tabulate(n, [](int i) -> payment_entry
													{ return i; });
}

struct shipment_entry
{
	Date date;
	int num;
	shipment_entry(int idx, const new_order_entry &order)
			: num(random_hash('s' + 'h' + 'i' + 'p', idx, 1, 10)),
				date(order.orders.order_date)
	{
	}
};

parlay::sequence<shipment_entry> generate_shipments(const parlay::sequence<new_order_entry> &orders)
{
	return parlay::tabulate(orders.size(), [&](int i) -> shipment_entry
													{ return {i, orders[i]}; });
}

// struct txn_info
// {
// 	parlay::sequence<std::pair<dkey_t, dkey_t>> new_order_queue;
// 	unsigned int start;
// 	int new_order_num;
// 	int payment_num;
// 	parlay::sequence<new_order_entry> new_orders;
// 	parlay::sequence<payment_entry> payments;
// 	parlay::sequence<shipment_entry> shipments;
// 	txn_info(const maps &indices, size_t num_txns) : start(0),
// 																									 new_order_num(0),
// 																									 payment_num(0),
// 																									 new_orders(generate_new_orders(num_txns, indices)),
// 																									 payments(generate_payments(num_txns)),
// 																									 shipments(generate_shipments(new_orders))
// 	{
// 		new_order_queue.reserve(num_txns);
// 	}
// };

class Tables
{
public:
	Tables(maps indices, size_t num_txns) : last(0),
																					new_line_items(0),
																					shipped_line_items(0),
																					start(0),
																					new_order_num(0),
																					payment_num(0),
																					new_orders(generate_new_orders(num_txns, indices)),
																					payments(generate_payments(num_txns)),
																					shipments(generate_shipments(new_orders)),
																					history{std::move(indices)}
	{
		new_order_queue.reserve(num_txns);
	}

	double place_new_order(size_t idx)
	{
		const auto &new_order = new_orders.at(idx);
		timer t;
		t.start();
		new_line_items += new_order.num_items;
		auto orders_for_customer = indices().orders_for_customer;
		line_item_set line_items{new_order.items.begin(), new_order.items.end()};
		auto orders_with_line_items = std::make_pair(new_order.orders, std::move(line_items));
		auto keyed_orders_with_line_items = std::make_pair(new_order.orders.order_key, std::move(orders_with_line_items));
		auto orders_for_date = indices().orders_for_date;
		auto suppliers_for_part = indices().suppliers_for_part;
		auto line_items_for_order = indices().line_items_for_order;
		auto parts_for_supplier = indices().parts_for_supplier;

		orders_for_customer.update(new_order.orders.customer_key, [&](customer_map::E &e) {
			order_map o = e.second.second;
			o.insert(keyed_orders_with_line_items);
			return std::make_pair(e.second.first, std::move(o));
		});

		// update om
		line_items_for_order.insert(std::move(keyed_orders_with_line_items));

		// update supp->part->lineitem
		for (int i = 0; i < new_order.num_items; i++)
		{
			auto supplier_key = new_order.items[i].supplier_key;
			auto part_key = new_order.items[i].part_key;
			int qty = new_order.items[i].quantity();
			parts_for_supplier.update(supplier_key, [&](supp_to_part_map::E &e) {
				auto t1 = part_supp_and_item_map::update(e.second.second, {part_key, supplier_key}, [&](part_supp_and_item_map::E &e2) {
					auto x = e2.second.first;
					x.available_quantity -= qty;
					auto line_items = line_item_set::insert(e2.second.second, new_order.items[i]);
					return std::make_pair(std::move(x), std::move(line_items));
				});
				return std::make_pair(e.second.first, std::move(t1)); 
			});
		}

		// update part->supp->lineitem
		for (int i = 0; i < new_order.num_items; i++)
		{
			auto supplier_key = new_order.items[i].supplier_key;
			auto part_key = new_order.items[i].part_key;
			suppliers_for_part.update(part_key, [&](part_to_supp_map::E &e) {
				auto t1 = part_supp_and_item_map::update(e.second.second, {part_key, supplier_key}, [&](part_supp_and_item_map::E &e2) {
					auto line_items = line_item_set::insert(e2.second.second, new_order.items[i]);
					return std::make_pair(e2.second.first, std::move(line_items));
				});
				return std::make_pair(e.second.first, std::move(t1));
			});
		}

		// add to new_order queue
		new_order_queue.emplace_back(new_order.orders.customer_key, new_order.orders.order_key);

		auto updated_indices = indices();
		updated_indices.orders_for_customer = std::move(orders_for_customer);
		updated_indices.parts_for_supplier = std::move(parts_for_supplier);
		updated_indices.orders_for_date = std::move(orders_for_date);
		updated_indices.line_items_for_order = std::move(line_items_for_order);
		updated_indices.suppliers_for_part = std::move(suppliers_for_part);
		updated_indices.version = history.size();

		history.push_back(std::move(updated_indices));
		return t.stop();
	}

	double make_payment(size_t idx)
	{
		const auto &payment = payments.at(idx);
		timer t;
		t.start();
		auto orders_for_customer = indices().orders_for_customer;
		orders_for_customer.update(payment.customer_key, [&](customer_map::E &e) {
			e.second.first.acctbal -= payment.amount;
			return e.second;
		});
		auto updated_indices = indices();
		updated_indices.orders_for_customer = std::move(orders_for_customer);
		updated_indices.version = history.size();
		history.push_back(std::move(updated_indices));
		return t.stop();
	}

	double shipment(size_t idx)
	{
		const auto &ship = shipments.at(idx);
		auto m = indices();
		timer t;
		t.start();
		auto orders_for_customer = m.orders_for_customer;
		auto shipments_for_date = m.shipments_for_date;
		Date now = ship.date;
		parlay::sequence<line_item_set> line_items_for_order_number{static_cast<size_t>(ship.num)};
		parlay::sequence<Date> order_dates{static_cast<size_t>(ship.num)};
		auto line_items_for_order = m.line_items_for_order;
		auto suppliers_for_part = m.suppliers_for_part;
		auto parts_for_supplier = m.parts_for_supplier;
		auto orders_for_date = m.orders_for_date;

		// update customers
		for (size_t i = start; i < start + ship.num && i < new_order_queue.size(); i++)
		{
			auto [customer_key, order_key] = new_order_queue[i];
			double money = 0.0;
			orders_for_customer.update(customer_key, [&](customer_map::E e) {
				order_map omf = e.second.second;
				// increase customer's balance
				e.second.first.acctbal += money;
				omf.update(order_key, [&](order_map::E e) {
					auto line_items = e.second.second;
					line_items_for_order_number[i - start] = line_items;
					money = e.second.first.total_price;
					order_dates[i - start] = e.second.first.order_date;
					e.second.second = line_item_set::map_set(line_items, [&](line_item_set::E &e) {
						// update lineitem's shipdate
						e.ship_date = now;
						e.set_linestatus();
						// insert into ship_map
						//----to be added later---
						shipments_for_date = ship_map::insert(shipments_for_date, {now, line_item_set(e)});
						return e;
					});
					return e.second;
				});
				e.second.second = omf;
				return e.second;
			});
		}

		// update parts and supps
		for (size_t i = start; i < start + ship.num && i < new_order_queue.size(); i++)
		{
			auto line_items = line_items_for_order_number[i - start];
			shipped_line_items += line_items.size();
			line_item_set::map_index(line_items, [&](line_item_set::E e, size_t i) {
				e.ship_date = now;
				e.set_linestatus();
				auto supplier_key = e.supplier_key;
				auto part_key = e.part_key;
				// update supp->part->lineitem
				parts_for_supplier.update(supplier_key, [&](supp_to_part_map::E &ee) {
					auto t1 = part_supp_and_item_map::update(ee.second.second, {part_key, supplier_key}, [&](part_supp_and_item_map::E &e2) {
						line_item_set lm = line_item_set::insert(e2.second.second, e);
						return make_pair(e2.second.first, lm);
					});
					return std::make_pair(ee.second.first, t1);
				});

				// update part->supp->lineitem
				suppliers_for_part.update(part_key, [&](part_to_supp_map::E &ee) {
					auto t1 = part_supp_and_item_map::update(ee.second.second, {part_key, supplier_key}, [&](part_supp_and_item_map::E &e2) {
						line_item_set lm = line_item_set::insert(e2.second.second, e);
						return make_pair(e2.second.first, lm);
					});
					return make_pair(ee.second.first, t1);
				});
			});
		}

		// update odate
		for (size_t i = start; i < start + ship.num && i < new_order_queue.size(); i++)
		{
			auto [_, order_key] = new_order_queue[i];
			Date dt = order_dates[i - start];
			orders_for_date.update(dt, [&](o_order_map::E e) {
				order_map om = e.second;
				om.update(order_key, [&](order_map::E e) {
					line_item_set lm = e.second.second;
					line_items_for_order_number[i - start] = lm;
					e.second.second = line_item_set::map_set(lm, [&](line_item_set::E e) {
						// update lineitem's shipdate
						e.ship_date = now;
						return e;
					});
					return e.second;
				});
				return om;
			});
		}

		start += ship.num;
		maps updated_indices = m;
		updated_indices.orders_for_customer = orders_for_customer;
		updated_indices.shipments_for_date = shipments_for_date;
		updated_indices.orders_for_date = orders_for_date;
		updated_indices.parts_for_supplier = parts_for_supplier;
		updated_indices.suppliers_for_part = suppliers_for_part;
		updated_indices.version = history.size();

		history.push_back(std::move(updated_indices));
		return t.stop();
	}

	void output_new_order(size_t i, ofstream &myfile)
	{
		const auto &t = new_orders.at(i);
		myfile
				<< "N|" << t.num_items
				<< "|" << t.orders.order_key
				<< "|" << t.orders.customer_key
				<< "|" << t.orders.order_date
				<< "|" << t.orders.shipment_priority
				<< "|" << t.orders.order_priority
				<< "|" << t.orders.status
				<< "|" << t.orders.strings
				<< std::endl;
		for (int i = 0; i < t.num_items; i++)
		{
			myfile
					<< t.items[i].part_key << "|"
					<< t.items[i].supplier_key << "|"
					<< t.items[i].c_date << "|"
					<< t.items[i].r_date << "|"
					<< t.items[i].e_price << "|"
					<< t.items[i].discount.val() << "|"
					<< t.items[i].tax.val()
					<< std::endl;
		}
	}

	void collect()
	{
		parlay::parallel_for(last, history.size() - 1, [&](size_t i) {
			history[i].clear();
		});
		last = history.size() - 1;
	}

	void output_payment(size_t i, ofstream &myfile)
	{
		const auto &t = payments.at(i);
		myfile << "P|" << t.customer_key << "|" << t.amount << std::endl;
	}

	void output_shipment(size_t i, ofstream &myfile)
	{
		const auto &t = shipments.at(i);
		myfile << "S|" << t.date << "|" << t.num << std::endl;
	}


	const maps &indices() const
	{
		return history.back();
	}

	size_t new_items() const
	{
		return new_line_items;
	}

	size_t shipped_items() const
	{
		return shipped_line_items;
	}

private:
	parlay::sequence<std::pair<dkey_t, dkey_t>> new_order_queue;
	size_t last;
	size_t new_line_items;
	size_t shipped_line_items;
	unsigned int start;
	int new_order_num;
	int payment_num;
	parlay::sequence<new_order_entry> new_orders;
	parlay::sequence<payment_entry> payments;
	parlay::sequence<shipment_entry> shipments;
	parlay::sequence<transaction> txns;
	parlay::sequence<maps> history;
};
