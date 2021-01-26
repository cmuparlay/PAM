#include <algorithm>
#include <iostream>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include <pam/pam.h>
#include <pam/get_time.h>
#include <pam/parse_command_line.h>
#include <parlay/primitives.h>

#include "rect_utils.h"
#include "rectangle.h"

using namespace std;

size_t rsv = 1000;

void reset_timers() {
  reserve_tm.reset();
  init_tm.reset(); sort_tm.reset(); build_tm.reset(); total_tm.reset(); 
}

void run_all(parlay::sequence<rec_type>& recs,
	     size_t iteration, int min_val, int max_val, int query_num) {
  std::string benchmark_name = "Query-All";
  {
    reset_timers();

    //const size_t threads = parlay::num_workers();
    const size_t num_points = recs.size();
    cout << rsv << endl;
    RectangleQuery r(recs);
    r.print_stats();
    //r.print_root();

    cout << "constructed in " << total_tm.get_total() << " seconds" << endl;
    parlay::sequence<query_type> queries = generate_queries(query_num, min_val, max_val);
  
    parlay::sequence<size_t> counts(query_num);

    timer t_query_total;
    t_query_total.start();

    parlay::parallel_for (0, query_num, [&] (size_t i) {
      auto out = parlay::sequence<rec_type>::uninitialized(rsv);
      int x = r.query_points(queries[i], out.begin());
      counts[i] = x;
    });
    t_query_total.stop();

    size_t total = parlay::reduce(counts);

    cout << "RESULT" << fixed << setprecision(3)
	 << "\talgo=" << "RecTree"
	 << "\tname=" << benchmark_name
	 << "\tn=" << num_points
	 << "\tq=" << query_num
	 << "\tp=" << parlay::num_workers()
	 << "\tmin-val=" << min_val
	 << "\tmax-val=" << max_val
	 << "\twin-mean=" << win
	 << "\titeration=" << iteration
	 << "\tbuild-time=" << total_tm.get_total()
	 << "\treserve-time=" << reserve_tm.get_total()
	 << "\tquery-time=" << t_query_total.get_total()
	 << "\ttotal=" << total
	 << endl;

  }
  RectangleQuery::finish();
}

int main(int argc, char** argv) {

  commandLine P(argc, argv,
		"./rec_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-d dist] [-w window] [-s rsv]");
  size_t n = P.getOptionLongValue("-n", 10000000);
  int min_val = P.getOptionIntValue("-l", 0);
  int max_val = P.getOptionIntValue("-h", 1000000000);
  size_t iterations = P.getOptionIntValue("-r", 3);
  dist = P.getOptionIntValue("-d", 0);
  win = P.getOptionIntValue("-w", 1000000);
  size_t query_num = P.getOptionLongValue("-q", 1000);
  rsv = P.getOptionIntValue("-s", 1000);

  srand(2017);

  parlay::sequence<rec_type> recs = generate_recs(n, min_val, max_val);
	
  for (size_t i = 0; i < iterations; ++i) {
    run_all(recs, i, min_val, max_val, query_num);
  }

  return 0;
}

