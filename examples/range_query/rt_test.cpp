/*
	Based on papers:
	PAM: Parallel Augmented maps
	Yihan Sun, Daniel Ferizovic and Guy Blelloch
	PPoPP 2018
	
	Parallel Range, Segment and Rectangle Queries with Augmented Maps
	Yihan Sun and Guy Blelloch
	ALENEX 2019
*/

#include <algorithm>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include <pam/pam.h>
#include <pam/get_time.h>
#include <pam/parse_command_line.h>
#include "range_utils.h"
#include "range_tree.h"

using RQ = RangeQuery<data_type, data_type>;

void reset_timers() {
  reserve_tm.reset();
  init_tm.reset(); sort_tm.reset(); build_tm.reset(); total_tm.reset(); 	
}

void run_all(sequence<point_type>& points, size_t iteration,
	     data_type min_val, data_type max_val, size_t query_num) {
  {
    string benchmark_name = "Query-All";
    RQ r(points);

    sequence<Query_data> queries = generate_queries(query_num, min_val, max_val);

    r.print_status();
  
    sequence<size_t> counts(query_num);
    timer t_query_total;
    t_query_total.start();
    parallel_for(0, query_num, [&] (size_t i) {
      sequence<pair<int,int>> out = r.query_range(queries[i].x1, queries[i].y1,
						  queries[i].x2, queries[i].y2);
      counts[i] = out.size();
    });

    t_query_total.stop();
    size_t total = reduce(counts);

    cout << "RESULT" << fixed << setprecision(3)
	 << "\talgo=" << "RageTree"
	 << "\tname=" << benchmark_name
	 << "\tn=" << points.size()
	 << "\tq=" << query_num
	 << "\tp=" << num_workers()
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
  reset_timers();
  RQ::finish();
}


void run_sum(sequence<point_type>& points, size_t iteration, data_type min_val, data_type max_val, size_t query_num) {
  string benchmark_name = "Query-Sum";
  {
  RQ r(points);
  r.print_status();
  sequence<Query_data> queries = generate_queries(query_num, min_val, max_val);

  sequence<size_t> counts(query_num);

  timer t_query_total;
  t_query_total.start();
  parallel_for (0, query_num, [&] (size_t i) {
    counts[i] = r.query_sum(queries[i].x1, queries[i].y1,
			     queries[i].x2, queries[i].y2);
    });

  t_query_total.stop();
  size_t total = reduce(counts);

  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "RageTree"
       << "\tname=" << benchmark_name
       << "\tn=" << points.size()
       << "\tq=" << query_num
       << "\tp=" << num_workers()
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
  reset_timers();
  RQ::finish();
}

void run_insert(sequence<point_type>& points, size_t iteration, data_type min_val, data_type max_val, size_t query_num) {
  string benchmark_name = "Query-Insert";
  {
    RQ r(points);

  sequence<point_type> new_points = generate_points(query_num, min_val, insert_range, 'i'+'n'+'s'+'e'+'r'+'t');
  for (size_t i = 0; i < query_num; i++) new_points[i].y = random_hash(2991,i,0,1000000000)+new_points[i].y;
  cout << "constructed" << endl;
  r.print_status();
  timer t_query_total;
  t_query_total.start();
  for (size_t i = 0; i < query_num; i++) {
    r.insert_point(new_points[i]);
  }
  cout << "query end" << endl;
  t_query_total.stop();
  // r.print_status();
  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "RageTree"
       << "\tname=" << benchmark_name
       << "\tn=" << points.size()
       << "\tq=" << query_num
       << "\tp=" << num_workers()
       << "\tmin-val=" << min_val
       << "\tmax-val=" << max_val
       << "\twin-mean=" << win
       << "\titeration=" << iteration
       << "\tbuild-time=" << total_tm.get_total()
       << "\treserve-time=" << reserve_tm.get_total()
       << "\tquery-time=" << t_query_total.get_total()
       << endl;

  }
  reset_timers();
  RQ::finish();
}

void run_insert_lazy(sequence<point_type>& points, size_t iteration, data_type min_val, data_type max_val, size_t query_num) {
  string benchmark_name = "Query-Insert";
  {
    RQ r(points);

  sequence<point_type> new_points = generate_points(query_num, min_val, insert_range, 'i'+'n'+'s'+'e'+'r'+'t');
  for (size_t i = 0; i < query_num; i++) new_points[i].y = random_hash(2991,i,0,1000000000)+new_points[i].y;
  r.print_status();
  timer t_query_total;
  t_query_total.start();
  for (size_t i = 0; i < query_num; i++) {
    r.insert_point_lazy(new_points[i]);
  }
  cout << endl;
  cout << "query end" << endl;
  t_query_total.stop();
  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "RageTree"
       << "\tname=" << benchmark_name
       << "\tn=" << points.size()
       << "\tq=" << query_num
       << "\tp=" << num_workers()
       << "\tmin-val=" << min_val
       << "\tmax-val=" << max_val
       << "\twin-mean=" << win
       << "\titeration=" << iteration
       << "\tbuild-time=" << total_tm.get_total()
       << "\treserve-time=" << reserve_tm.get_total()
       << "\tquery-time=" << t_query_total.get_total()
       << endl;

  }
  reset_timers();
  RQ::finish();
}

void test_loop(size_t n, int min_val, int max_val,
	       size_t iterations, size_t query_num, int type) {
  for (size_t i = 0; i < iterations; ++i) {
    sequence<point_type> points = generate_points(n, min_val, max_val);
    if (type == 0) run_all(points, i, min_val, max_val, query_num);
    else {
      if (type == 1) run_sum(points, i, min_val, max_val, query_num);
      else {
		if (type == 2) run_insert(points, i, min_val, max_val, query_num);
		else {
		  run_insert_lazy(points, i, min_val, max_val, query_num);
		}
      }
    }
  }
}

int main(int argc, char** argv) {

  // n: input size
  // coordinates in range [l, h]
  // run in r rounds and q queries
  // dist = 0  means random query windows
  // dist != 0 means average query window edge length of w/2
  // query_type: 0 for report-all, 1 for report-sum, 2 for insert, 3 for lazy insert
  // insert_range: the coordinate range of insertions
  if (argc == 1) {
	  cout << "./rt_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-d dist] [-w window] [-t query_type] [-e insert_range]" << endl;
	  cout << "n: input size" << endl;
	  cout << "coordinates in range [l, h]" << endl;
	  cout << "run in r rounds and q queries" << endl;
	  cout << "dist = 0  means random query windows" << endl;
	  cout << "dist != 0 means average query window edge length of w/2" << endl;
	  cout << "query_type: 0 for report-all, 1 for report-sum, 2 for insert, 3 for lazy insert" << endl;
	  cout << "insert_range: the coordinate range of insertions" << endl;
  }
	  
  commandLine P(argc, argv,
		"./rt_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-d dist] [-w window] [-t query_type] [-e insert_range]");
  size_t n = P.getOptionLongValue("-n", 100000000);
  int min_val = P.getOptionIntValue("-l", 0);
  int max_val = P.getOptionIntValue("-h", 1000000000);
  size_t iterations = P.getOptionIntValue("-r", 3);
  dist = P.getOptionIntValue("-d", 0);
  win = P.getOptionIntValue("-w", 1000000);
  size_t query_num = P.getOptionLongValue("-q", 1000);
  int type = P.getOptionIntValue("-t", 0);
  insert_range = P.getOptionIntValue("-e", max_val);

  srand(2017);

  test_loop(n, min_val, max_val, iterations, query_num, type);
  
  return 0;
}

