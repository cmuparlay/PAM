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
#include <parlay/primitives.h>
#include "seg_utils_horizontal.h"
#include "segment.h"

using namespace std;

int str_to_int(char* str) {
  return strtol(str, NULL, 10);
}

void reset_timers() {
  reserve_tm.reset();
  init_tm.reset(); sort_tm.reset(); build_tm.reset(); total_tm.reset(); 
}

void run_all(parlay::sequence<seg_type_2d>& segs, size_t iteration,
	     int min_val, int max_val, int query_num) {
  //const size_t threads = parlay::num_workers();
  std::string benchmark_name = "Query-All";

  {
    //timer t;
    //t.start();
    const size_t num_points = segs.size();
    SegmentQuery<int> r(segs);
    //r->print_tree();
    //double tm = t.stop();

    parlay::sequence<query_type> queries = generate_queries(query_num, min_val, max_val);
    //cout << endl;
    parlay::sequence<size_t> counts(query_num);

    timer t_query_total;
    t_query_total.start();
    //int tot = 0;
    parlay::parallel_for (0, query_num, [&] (size_t i) {
	int cnt = r.query_count(queries[i], 0);
	parlay::sequence<seg_type_2d> out(cnt*2);
	r.query_points(queries[i], out.begin());
	counts[i] = out.size();
      });

    t_query_total.stop();
    size_t total = parlay::reduce(counts);

    cout << "RESULT" << fixed << setprecision(3)
	 << "\talgo=" << "SegTree"
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
  reset_timers();
  SegmentQuery<int>::finish();
  //SegmentQuery<int>::print_stats();
}

void run_sum(parlay::sequence<seg_type_2d>& segs, size_t iteration,
	     int min_val, int max_val, size_t query_num) {
  //const size_t threads = parlay::num_workers();
  std::string benchmark_name = "Query-Sum";
  //timer t;
  //t.start();
  const size_t num_points = segs.size();
  SegmentQuery<int> r(segs);
  //r.print_stats();
  //r->print_tree();
  //double tm = t.stop();

  parlay::sequence<query_type> queries = generate_queries(query_num, min_val, max_val);
  //cout << endl;
  parlay::sequence<size_t> counts(query_num);

  timer t_query_total;
  t_query_total.start();

  cout << "start query!" << endl;
  parlay::parallel_for (0, query_num, [&] (size_t i) {
      counts[i] = r.query_count(queries[i], 0);
    });

  t_query_total.stop();
  size_t total = parlay::reduce(counts);
	
  cout << "RESULT" << fixed << setprecision(5)
       << "\talgo=" << "SegTree"
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

  reset_timers();
  SegmentQuery<int>::finish();

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

