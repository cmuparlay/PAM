/*
	Based on papers:
	PAM: Parallel Augmented maps
	Yihan Sun, Daniel Ferizovic and Guy Blelloch
	PPoPP 2018
	
	Parallel Range, Segment and Rectangle Queries with Augmented Maps
	Yihan Sun and Guy Blelloch
	arXiv:1803.08621
*/

#include <algorithm>
#include <iostream>
#include <vector>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <parlay/primitives.h>
#include <pam/get_time.h>
#include <pam/parse_command_line.h>
#include "range_utils.h"
#include "range_sweep.h"

int str_to_int(char* str) {
  return strtol(str, NULL, 10);
}

void reset_timers() {
     reserve_tm.reset();
     deconst_tm.reset();
     sort_tm.reset();
     run_tm.reset();
}

void run(parlay::sequence<point_type>& points, size_t iteration, 
	 data_type min_val, data_type max_val, size_t query_num) {
  string benchmark_name = "Query-Sum";
  size_t total = 0;
  double tm_query = 0.0;
  {
    RangeQuery r(points);
  
    r.print_allocation_stats();

    //size_t query_num = points.size(); //1000000;
    parlay::sequence<Query_data> queries = generate_queries(query_num, min_val, max_val);
    parlay::sequence<size_t> counts(query_num);
  
    timer t_query;
    t_query.start();
    parlay::parallel_for (0, query_num, [&] (size_t i) {
	counts[i] = r.query(queries[i].x1, queries[i].y1,
						  queries[i].x2, queries[i].y2);
      });
    tm_query = t_query.stop();
    total = parlay::reduce(counts);
  }

  RangeQuery::finish(); // free memory
    
  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "RageSweepSum"
       << "\tname=" << benchmark_name
       << "\tn=" << points.size()
       << "\tq=" << query_num
       << "\tp=" << parlay::num_workers()
       << "\tmin-val=" << min_val
       << "\tmax-val=" << max_val
       << "\twin-mean=" << win
       << "\titeration=" << iteration
       << "\tbuild-time=" << run_tm.get_total()
       << "\treserve-time=" << reserve_tm.get_total()
       << "\tquery-time=" << tm_query
       << "\ttotal=" << total
       << endl;

     reset_timers();
}

void test_loop(size_t n, int min_val, int max_val,
	       size_t iterations, size_t query_num) {
  for (size_t i = 0; i < iterations; ++i) {
    parlay::sequence<point_type> points = generate_points(n, min_val, max_val);
    run(points, i, min_val, max_val, query_num);
    reset_timers();
  }
}

int main(int argc, char** argv) {

  srand(2017);
  
  commandLine P(argc, argv,
		"./rs_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-d dist] [-w window]");
		
  if (argc == 1) {
	  cout << "./rs_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-d dist] [-w window]" << endl;
	  cout << "n: input size" << endl;
	  cout << "coordinates in range [l, h]" << endl;
	  cout << "run in r rounds and q queries" << endl;
	  cout << "dist = 0  means random query windows" << endl;
	  cout << "dist != 0 means average query window edge length of w/2" << endl;
  }
  size_t n = P.getOptionLongValue("-n", 100000000);
  int min_val = P.getOptionIntValue("-l", 0);
  int max_val = P.getOptionIntValue("-h", 1000000000);
  size_t iterations = P.getOptionIntValue("-r", 3);
  dist = P.getOptionIntValue("-d", 0);
  win = P.getOptionIntValue("-w", 1000000);
  size_t query_num = P.getOptionLongValue("-q", 1000);

  test_loop(n, min_val, max_val, iterations, query_num);
  
  return 0;
}
