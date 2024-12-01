#include <iostream>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <pam/get_time.h>
#include <pam/parse_command_line.h>
#include <pam/pam.h>

#include "seg_utils_general.h"
#include "seg_sweep_g.h"



using namespace std;

int str_to_int(char* str) {
  return strtol(str, nullptr, 10);
}


void run_all(parlay::sequence<seg_type>& segs,
	 size_t iteration, int query_num) {
  std::string benchmark_name = "Query-All";
  reset_timers();

  const size_t threads = parlay::num_workers();
  const size_t num_points = segs.size();
  SegmentQuery r(segs);
  r.print_allocation_stats();

  parlay::sequence<query_type> queries = generate_queries(query_num);
  
  sequence<size_t> counts(query_num);

  timer t_query_total;
  t_query_total.start();

  parallel_for (0, query_num, [&] (size_t i) {
    parlay::sequence<seg_type> a = r.query_points(queries[i]);
    counts[i] = a.size();
    });
  t_query_total.stop();

  size_t total = parlay::reduce(counts);

  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "SegSweep"
       << "\tname=" << benchmark_name
       << "\tn=" << num_points
       << "\tq=" << query_num
       << "\tp=" << threads
       << "\twin-mean=" << win
       << "\titeration=" << iteration
       << "\tbuild-time=" << total_tm.get_total()
       << "\treserve-time=" << reserve_tm.get_total()
       << "\tquery-time=" << t_query_total.get_total()
       << "\ttotal=" << total
       << endl;

  SegmentQuery::finish();
}

void run_sum(parlay::sequence<seg_type>& segs,
   size_t iteration, int query_num) {
  std::string benchmark_name = "Query-Sum";
  reset_timers();

  const size_t threads = parlay::num_workers();
  const size_t num_points = segs.size();
  SegmentQuery r(segs);

  parlay::sequence<query_type> queries = generate_queries(query_num);
  
  sequence<size_t> counts(query_num);

  timer t_query_total;
  t_query_total.start();
  parallel_for (0, query_num, [&] (size_t i) {
      counts[i] = r.query_sum(queries[i]);
    });

  t_query_total.stop();

  size_t total = parlay::reduce(counts);
  
    cout << "RESULT" << fixed << setprecision(3)
	 << "\talgo=" << "SegSweep"
	 << "\tname=" << benchmark_name
	 << "\tn=" << num_points
	 << "\tq=" << query_num
	 << "\tp=" << threads
	 << "\twin-mean=" << win
	 << "\titeration=" << iteration
	 << "\tbuild-time=" << total_tm.get_total()
	 << "\treserve-time=" << reserve_tm.get_total()
	 << "\tquery-time=" << t_query_total.get_total()
	 << "\ttotal=" << total
	 << endl;
  
    SegmentQuery::finish();
}


int main(int argc, char** argv) {
  // n: input size
  // run in r rounds and q queries
  // d (dist) = 0  means random query windows
  // d (dist) != 0 means average query window edge length of w/2
  // t (query_type): 0 for report-all, 1 for report-sum, 2 for input segs with controlled length (the projection on x-axis is of length x/2 on average)
  if (argc == 1) {
	  cout << "./rt_test [-n size] [-r rounds] [-q queries] [-d dist] [-w window] [-t querytype] [-x inputseglength]" << endl;
	  cout << "n: input size" << endl;
	  cout << "run in r rounds and q queries" << endl;
	  cout << "d = 0  means random query windows" << endl;
	  cout << "d != 0 means average query window edge length of w/2" << endl;
	  cout << "t: 0 for report-all, 1 for report-sum, 2 input segments with controlled length (the projection on x-axis is of length x on average)" << endl;
	  //cout << "b:number of blocks" << endl;
	  exit(1);
  }
	srand(2017);
	
	commandLine P(argc, argv,
		"[-n size] [-r rounds] [-q queries] [-d dist] [-w window] [-t querytype] [-x inputseglength]");

    size_t n = P.getOptionLongValue("-n", 100000000);
	dist = P.getOptionIntValue("-d", 0);
	win = P.getOptionIntValue("-w", 1000);
	size_t query_num  = P.getOptionLongValue("-q", 1000);
    int type = P.getOptionIntValue("-t", 0);
	int win_x = P.getOptionIntValue("-x", 10);
    size_t iterations = P.getOptionIntValue("-r", 3);
	//size_t num_blocks = P.getOptionIntValue("-b", 0);

  for (size_t i = 0; i < iterations; ++i) {
	    if (type < 2) {
			parlay::sequence<seg_type> segs = generate_segs(n);
			if (type == 0) run_all(segs, i, query_num);
			if (type == 1) run_sum(segs, i, query_num);
		}			
		if (type == 2) {
			parlay::sequence<seg_type> segs = generate_segs_small(n, win_x);
			run_all(segs, i, query_num);
		}
  }

  return 0;
}

