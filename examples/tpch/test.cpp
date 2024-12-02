using namespace std;

#include <math.h>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include "lineitem.h"
#include <pam/get_time.h>
#include <pam/parse_command_line.h>
#include <parlay/internal/collect_reduce.h>
#include <parlay/random.h>
#include <parlay/monoid.h>
#include <pam/pam.h>
#include "utils.h"
#include "tables.h"

#include "queries/queries.h"
#include "new_orders.h"

// size_t new_lineitem = 0;
// size_t shipped_lineitem = 0;
// size_t keep_versions = 1000000;
// bool if_persistent = false;
// std::atomic<int> cur_txn;

struct options
{
  bool verbose = false;
  bool query = false;
  bool update = false;
  bool persistent = false;
  bool collect = false;
  int scale = 0;
  int keep_versions = 0;
  int txns = 0;
  std::filesystem::path directory;
};

std::pair<double, double> geo_mean(const parlay::sequence<double> &timings) {
  double res = 0.0, dev = 0.0, size = timings.size();
  for (auto time : timings) {
    res += log2(time * 1000.0);
  }
  res /= size;
  res = exp2(res);
  double m = 0.0;
  for (auto time : timings) {
    double tmp = log2(time * 1000.0 / res);
    m += tmp * tmp;
  }
  dev = exp2(sqrt(m / size));
  return {res, dev};
}

struct statistics {
  double elapsed;
  size_t new_items;
  size_t shipped_items;
};

statistics exe_txns(maps indices, const parlay::sequence<transaction> &txns, const options &opts)
{
  Tables tables(std::move(indices), opts.txns);
  std::cout << "start transactions" << std::endl;
  int num_new_orders = 0, num_payments = 0, num_shipments = 0;

  size_t max_lineitem = 0;
  size_t max_order = 0;
  size_t max_customer = 0;
  size_t max_part_supp = 0;
  size_t max_part = 0;

  parlay::sequence<double> new_order_timings;
  parlay::sequence<double> payment_timings;
  parlay::sequence<double> shipment_timings;

  std::cout << "initial_lineitem: " << line_item_set::GC::used_node() << std::endl;
  std::cout << "initial_part_supp: " << part_supp_and_item_map::GC::used_node() << std::endl;
  std::cout << "initial_part: " << part_to_supp_map::GC::used_node() << std::endl;
  std::cout << "initial_order: " << order_map::GC::used_node() << std::endl;
  std::cout << "initial_customer: " << customer_map::GC::used_node() << std::endl;

  ofstream myfile;
  myfile.open("logs/tpcc.log");

  timer tm;
  tm.start();
  for (int i = 0; i < opts.txns; i++)
  {
    switch (txns[i].type)
    {
    case transaction::type::NewOrder:
    {
      double duration = tables.place_new_order(i);
      if (opts.persistent)
      {
        tables.output_new_order(i, myfile);
      }
      new_order_timings.push_back(duration);
      break;
    }
    case transaction::type::Payment:
    {
      double duration = tables.make_payment(i);
      if (opts.persistent)
      {
        tables.output_payment(i, myfile);
      }
      payment_timings.push_back(duration);
      break;
    }
    case transaction::type::Shipment:
    {
      double duration = tables.shipment(i);
      if (opts.persistent)
      {
        tables.output_shipment(i, myfile);
      }
      shipment_timings.push_back(duration);
      break;
    }
    }
    with_lock(index_mutex, [&]() {
      index_snapshot = tables.indices();
    });
    // if (if_collect && (history.size() >= keep_versions)) history[history.size()-keep_versions].clear();
    if (opts.collect && ((i + 2) % opts.keep_versions == 0))
    {
      tables.collect();
    }
    size_t t = line_item_set::GC::used_node();

    if (t > max_lineitem)
      max_lineitem = t;
    t = part_supp_and_item_map::GC::used_node();
    if (t > max_part_supp)
      max_part_supp = t;
    t = part_to_supp_map::GC::used_node();
    if (t > max_part)
      max_part = t;
    t = order_map::GC::used_node();
    if (t > max_order)
      max_order = t;
    t = customer_map::GC::used_node();
    if (t > max_customer)
      max_customer = t;
  }
  if (opts.verbose)
    std::cout << std::endl;
  finish = true;
  double elapsed = tm.stop();
  std::cout << "Elapsed duration: " << elapsed << std::endl;
  myfile.close();

  auto x1 = geo_mean(new_order_timings);
  auto x2 = geo_mean(payment_timings);
  auto x3 = geo_mean(shipment_timings);

  double new_order_time = parlay::reduce(new_order_timings);
  double payment_time = parlay::reduce(payment_timings);
  double shipment_time = parlay::reduce(shipment_timings);

  std::cout << "add up to: " << new_order_time << " " << payment_time << " " << shipment_time << std::endl;

  std::cout << "new_order: " << new_order_timings.size() << " of new orders, time: " << x1.first << " dev: " << x1.second << std::endl;
  std::cout << "payment: " << payment_timings.size() << " of payments, time: " << x2.first << " dev: " << x2.second << std::endl;
  std::cout << "shipment: " << shipment_timings.size() << " of shipments, time: " << x3.first << " dev: " << x3.second << std::endl;

  std::cout << "max_lineitem: " << max_lineitem << std::endl;
  std::cout << "max_part_supp: " << max_part_supp << std::endl;
  std::cout << "max_part: " << max_part << std::endl;
  std::cout << "max_order: " << order_map::GC::used_node() << std::endl;
  std::cout << "max_customer: " << customer_map::GC::used_node() << std::endl;

  return {
    .elapsed = elapsed, 
    .new_items = tables.new_items(),
    .shipped_items = tables.shipped_items()
  };
}

void post_process(const query_timings &timings)
{

  double tot = 0;
  int no[] = {22, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
  std::cout << "total rounds: " << timings[0].size() << std::endl;
  for (int i = 0; i < NUM_QUERIES; i++)
  {
    std::cout << "processing query " << no[i] << std::endl;

    auto [res, dev] = geo_mean(timings[i]);

    std::cout << "geo mean of query " << no[i] << " : " << res << ", sdev: " << dev << std::endl
              << std::endl;
    tot += res;
  }
  std::cout << "geo mean of all: " << exp2(tot / (NUM_QUERIES + 0.0)) << std::endl;
}

auto generate_txns(size_t num_txns)
{
  int wgts[] = {45, 88, 92, 96, 100};
  return parlay::tabulate(num_txns, [&](int i) -> transaction
                          {
    int rand = random_hash('q' + 't', i, 0, 92);
    if (rand < wgts[0])
    {
      return {transaction::type::NewOrder, i};
    }
    if (rand < wgts[1])
    {
      return {transaction::type::Payment, i};
    }
    if (rand < wgts[2])
    {
       return {transaction::type::Shipment, i};
    }
    std::cerr << "Cannot generate transaction\n";
    std::abort(); });
}

void test_all(const options &opts)
{

  memory_stats();
  if (opts.verbose)
    nextTime("gc for make maps");

  auto indices = make_indices(opts.directory, opts.verbose);
  index_snapshot = indices;
  auto txns = generate_txns(opts.txns);
  nextTime("generate txns");

  if (opts.verbose)
  {
    std::cout << "print memory stats:" << std::endl;
    memory_stats();
    std::cout << "Lineitem size: " << sizeof(LineItem) << std::endl;
    std::cout << "Order size: " << sizeof(Orders) << std::endl;
    std::cout << "Customer size: " << sizeof(Customer) << std::endl;
    std::cout << "Part supplier size: " << sizeof(part_supp) << std::endl;
  }
  if (opts.query && opts.update)
  {
    memory_stats();
    nextTime("ready");

    size_t new_items = 0;
    size_t shipped_items = 0;
    std::optional<query_timings> query_timings;
    par_do([&]() { 
      auto stats = exe_txns(std::move(indices), txns, opts);
      new_items = stats.new_items;
      shipped_items = stats.shipped_items;
    }, [&]() {
      query_timings.emplace(exe_query(opts.verbose));
    });

    nextTime("query and update");
    post_process(*query_timings);
    memory_stats();
    std::cout << "new: " << new_items << std::endl;
    std::cout << "shipped: " << shipped_items << std::endl;
  }
  else
  {
    if (opts.query)
    {
      std::cout << "Queries start ..." << std::endl;
      auto query_timings = exe_query(opts.verbose);
      post_process(query_timings);
    }
    if (opts.update)
    {
      exe_txns(std::move(indices), txns, opts);
    }
    nextTime("query or update");
  }
}

int main(int argc, char **argv)
{
  commandLine P(argc, argv, "./test [-v] [-q] [-u] [-c] [-s size] [-t txns] [-d directory] [-y keep_versions] [-p]");
  options opts;
  opts.verbose = P.getOption("-v");
  opts.query = P.getOption("-q");
  opts.update = P.getOption("-u");
  opts.persistent = P.getOption("-p");
  opts.keep_versions = P.getOptionIntValue("-y", 1000000);
  opts.collect = P.getOption("-c");
  std::string default_directory = "/ssd1/tpch/S10/";
  opts.scale = P.getOptionIntValue("-s", 10);
  opts.txns = P.getOptionIntValue("-t", 100000);
  if (opts.scale == 100)
    default_directory = "/ssd1/tpch/S100/";
  if (opts.scale == 1)
    default_directory = "/ssd1/tpch/S1/";

  opts.directory = P.getOptionValue("-d", default_directory);

  test_all(opts);

  // memory_stats();
  return 0;
}
