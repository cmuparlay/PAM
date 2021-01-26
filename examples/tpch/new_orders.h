struct transaction {
	char type;
	int ind;
};

struct new_order_entry {
	Orders o;
	unsigned int num_items;
	Lineitem* items;
	new_order_entry() {}
	void generate(int idx, maps& m) { 
		int customer_id = random_hash('c', idx, 0, CUSTOMER_NUM) + 1;
		num_items = random_hash('n', idx, 1, 8);
		o.orderkey = CURR_ORDER + idx;
		o.custkey = customer_id;
		o.orderdate = get_today();
		o.shippriority = 0;
		o.orderpriority = 0;
		o.status = 'A';
		o.strings = new char[100];
		items = new Lineitem[num_items];
		float tot = 0.0;
		for (size_t i = 0; i < num_items; i++) {
			int item_id = random_hash('i'+'t'+'e'+'m'+idx, i, 0, PARTSUPP_NUM);
			int quantity = random_hash('q'+idx, i, 0, 20) + 1;
			auto ps = *(static_data.psm.select(item_id));
			items[i].partkey = ps.first.first; items[i].suppkey = ps.first.second;
			//Part_supp ps = p.second;
			Part part = (*(m.psm2.find(items[i].partkey))).first;
			items[i].orderkey = o.orderkey; 
			items[i].c_date = today;
			items[i].r_date = today;
			items[i].e_price = part.retailprice * quantity;
			items[i].linenumber = i+1;
			int dis = random_hash('d'+idx, i, 0, 10);
			int tax = random_hash('t'+idx, i, 0, 10);
			items[i].discount = Percent(dis);
			items[i].tax = Percent(tax);
			tot += (items[i].e_price*(100.0+tax)/100.0*(100.0-dis)/100.0);
		}
		o.totalprice = tot;
	}
};


new_order_entry* generate_new_orders(int n, maps& m) {
	new_order_entry* ret = new new_order_entry[n];
	parlay::parallel_for (0, n, [&] (size_t i) {
		ret[i].generate(i, m);
	  });
	return ret;
}

struct payment_entry {
	dkey_t custkey;
	float amount;
	payment_entry() {}
	void generate(int idx, maps& m) { 
		custkey = random_hash('c', idx, 0, CUSTOMER_NUM) + 1;
		amount = (random_hash('i'+'n'+'t', idx, 1, 5000)+0.0) + (random_hash('f'+'r'+'a'+'c', idx, 0, 100)+0.0)/100.0;
	}
};

payment_entry* generate_payments(int n, maps& m) {
	payment_entry* ret = new payment_entry[n];
	parlay::parallel_for (0, n, [&] (size_t i) {
		ret[i].generate(i, m);
	  });
	return ret;
}

struct shipment_entry {
	Date date;
	int num;
	shipment_entry() {}
	void generate(int idx, new_order_entry* no) {
		num = random_hash('s'+'h'+'i'+'p', idx, 1, 10);
		date = no[idx].o.orderdate;
	}
};

shipment_entry* generate_shipments(int n, new_order_entry* no) {
	shipment_entry* ret = new shipment_entry[n];
	parlay::parallel_for (0, n, [&] (size_t i) {
		ret[i].generate(i, no);
	  });
	return ret;
}

struct txn_info {
	vector<pair<dkey_t, dkey_t>> new_order_q;
	unsigned int start;
	new_order_entry* new_orders;
	payment_entry* payments;
	shipment_entry* shipments;
	int new_order_num;
	int payment_num;
};
