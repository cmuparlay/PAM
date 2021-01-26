#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include <pam/parse_command_line.h>
#include <pam/get_time.h>
#include <pam/pam.h>

#include "seg_utils_horizontal.h"
#include "seg_sweep.h"

int str_to_int(char* str) {
  return strtol(str, NULL, 10);
}

void run_all(parlay::sequence<seg_type_2d>& segs,
	 size_t iteration, int min_val, int max_val, int query_num) {
  std::string benchmark_name = "Query-All";
  reset_timers();
  {
    const size_t num_points = segs.size();
    SegmentQuery r(segs);
    r.print_allocation_stats();
    r.set_min_max(min_val, max_val);

    parlay::sequence<query_type> queries = generate_queries(query_num, min_val, max_val);
  
    parlay::sequence<size_t> counts(query_num);

    timer t_query_total;
    t_query_total.start();

    parlay::parallel_for (0, query_num, [&] (size_t i) {
      parlay::sequence<seg_type_2d> a = r.query_points(queries[i]);
      counts[i] = a.size();
    });
    t_query_total.stop();

    size_t total = parlay::reduce(counts);
    cout << "RESULT" << fixed << setprecision(3)
	 << "\talgo=" << "SegSweep"
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


    // / timer deconst_tm;
    //  deconst_tm.start();
  }
  SegmentQuery::finish();
  //  deconst_tm.stop();
}

void run_sum(parlay::sequence<seg_type_2d>& segs,
	     size_t iteration, int min_val, int max_val, int query_num) {
  std::string benchmark_name = "Query-Sum";
  reset_timers();
  {
    //const size_t threads = parlay::num_workers();
    const size_t num_points = segs.size();
    SegmentQuery r(segs);
    //r.print_allocation_stats();
    r.set_min_max(min_val, max_val);

    parlay::sequence<query_type> queries = generate_queries(query_num, min_val, max_val);
  
    parlay::sequence<size_t> counts(query_num);

    timer t_query_total;
    t_query_total.start();
    parlay::parallel_for (0, query_num, [&] (size_t i) {
      counts[i] = r.query_sum(queries[i]);
    });

    t_query_total.stop();

    size_t total = parlay::reduce(counts);
    cout << "RESULT" << fixed << setprecision(3)
	 << "\talgo=" << "SegSweep"
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
  SegmentQuery::finish();
}


int main(int argc, char** argv) {
  commandLine P(argc, argv,
		"./seg_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-d dist] [-w window] [-t query_type]");
  size_t n = P.getOptionLongValue("-n", 100000000);
  int min_val = P.getOptionIntValue("-l", 0);
  int max_val = P.getOptionIntValue("-h", 1000000000);
  size_t iterations = P.getOptionIntValue("-r", 3);
  dist = P.getOptionIntValue("-d", 0);
  win = P.getOptionIntValue("-w", 1000000);
  size_t query_num = P.getOptionLongValue("-q", 1000);
  int type = P.getOptionIntValue("-t", 0);

  srand(2017);

  for (size_t i = 0; i < iterations; ++i) {
    parlay::sequence<seg_type_2d> segs = generate_segs(n, min_val, max_val);
    if (type == 0) run_all(segs, i, min_val, max_val, query_num);
    else run_sum(segs, i, min_val, max_val, query_num);
  }

  return 0;
}

