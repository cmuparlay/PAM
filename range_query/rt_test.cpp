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

#include "range_tree.h"
#include "pbbslib/get_time.h"
#include "pbbslib/parse_command_line.h"

using namespace std;

using data_type = int;
using point_type = Point<data_type, data_type>;
using tuple_type = tuple<data_type, data_type, data_type, data_type>;

int insert_range;

struct Query_data {
  data_type x1, x2, y1, y2;
} query_data;

int str_to_int(char* str) {
  return strtol(str, NULL, 10);
}

int win;
int dist;

int random_hash(int seed, int a, int rangeL, int rangeR) {
  if (rangeL == rangeR) return rangeL;
  a = (a+seed) + (seed<<7);
  a = (a+0x7ed55d16) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = ((a+seed)>>5) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  a = (a^0xb55a4f09) ^ (a>>16);
  a = a % (rangeR-rangeL);
  if (a<0) a+= (rangeR-rangeL);
  a+=rangeL;
  return a;
}

// generate random input points. The coordinate are uniformly distributed in [a, b]
vector<point_type> generate_points(size_t n, data_type a, data_type b, data_type offset = 0) {
  vector<point_type> ret(n);
  parallel_for(0, n, [&] (size_t i) {
    ret[i].x = random_hash('x'+offset, i, a, b);
    ret[i].y = random_hash('y'+offset, i, a, b);
    ret[i].w = 1; //random_hash('w', i, a, b);
    });

  return ret;
}

// generate random query windows:
// if dist == 0: the coordinate of the query windows are uniformly random in [a, b]
// otherwise: the average size of query windows is win^2/4
vector<Query_data> generate_queries(size_t q, data_type a, data_type b) {
  vector<Query_data> ret(q);

  parallel_for (0, q, [&] (size_t i) {
    data_type x1 = random_hash('q'*'x', i*2, a, b);
    data_type y1 = random_hash('q'*'y', i*2, a, b);
    int dx = random_hash('d'*'x', i, 0, win);
    int dy = random_hash('d'*'y', i, 0, win);
    int x2 = x1+dx;
    int y2 = y1+dy;
    if (dist==0) {
      x2 = random_hash('q'*'x', i*2+1, a, b);
      y2 = random_hash('q'*'y', i*2+1, a, b);
    }
    if (x1 > x2) {
      data_type t = x1; x1 = x2; x2 = t;
    }
    if (y1 > y2) {
      data_type t = y1; y1 = y2; y2 = t;
    }
    ret[i].x1 = x1; ret[i].x2 = x2;
    ret[i].y1 = y1; ret[i].y2 = y2;
    });

  return ret;
}

void reset_timers() {
  reserve_tm.reset();
  init_tm.reset(); sort_tm.reset(); build_tm.reset(); total_tm.reset(); 	
}

void run_all(vector<point_type>& points, size_t iteration,
	     data_type min_val, data_type max_val, size_t query_num) {
  using RQ = RangeQuery<data_type, data_type>;
  string benchmark_name = "Query-All";
  RQ::reserve(points.size());

  RQ *r = new RQ(points);

  vector<Query_data> queries = generate_queries(query_num, min_val, max_val);

  //r->print_status();
  
  size_t counts[query_num];
  timer t_query_total;
  t_query_total.start();
  parallel_for(0, query_num, [&] (size_t i) {
    pbbs::sequence<pair<int,int>> out = r->query_range(queries[i].x1, queries[i].y1,
					       queries[i].x2, queries[i].y2);
    counts[i] = out.size();
    });

  t_query_total.stop();
  size_t total = pbbs::reduce(pbbs::sequence<size_t>(counts,query_num),
			      pbbs::addm<size_t>());

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

  reset_timers();

  delete r;
  RQ::finish();
}


void run_sum(vector<point_type>& points, size_t iteration, data_type min_val, data_type max_val, size_t query_num) {
  string benchmark_name = "Query-Sum";
  RangeQuery<data_type, data_type> *r = new RangeQuery<data_type, data_type>(points);
  //r->print_status();
  vector<Query_data> queries = generate_queries(query_num, min_val, max_val);
	
  size_t counts[query_num];

  timer t_query_total;
  t_query_total.start();
  parallel_for (0, query_num, [&] (size_t i) {
    counts[i] = r->query_sum(queries[i].x1, queries[i].y1,
			     queries[i].x2, queries[i].y2);
    });

  t_query_total.stop();
  size_t total = pbbs::reduce(pbbs::sequence<size_t>(counts,query_num),
			      pbbs::addm<size_t>());

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

  reset_timers();
  delete r;
}

void run_insert(vector<point_type>& points, size_t iteration, data_type min_val, data_type max_val, size_t query_num) {
  string benchmark_name = "Query-Insert";
  RangeQuery<data_type, data_type> *r = new RangeQuery<data_type, data_type>(points);

  vector<point_type> new_points = generate_points(query_num, min_val, insert_range, 'i'+'n'+'s'+'e'+'r'+'t');
  for (size_t i = 0; i < query_num; i++) new_points[i].y = random_hash(2991,i,0,1000000000)+new_points[i].y;
  cout << "constructed" << endl;
  //r->print_status();
  timer t_query_total;
  t_query_total.start();
  for (size_t i = 0; i < query_num; i++) {
    r->insert_point(new_points[i]);
  }
  cout << "query end" << endl;
  t_query_total.stop();
  // r->print_status();
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

  reset_timers();
  delete r;
}

void run_insert_lazy(vector<point_type>& points, size_t iteration, data_type min_val, data_type max_val, size_t query_num) {
  string benchmark_name = "Query-Insert";
  RangeQuery<data_type, data_type> *r = new RangeQuery<data_type, data_type>(points);

  vector<point_type> new_points = generate_points(query_num, min_val, insert_range, 'i'+'n'+'s'+'e'+'r'+'t');
  for (size_t i = 0; i < query_num; i++) new_points[i].y = random_hash(2991,i,0,1000000000)+new_points[i].y;
  //auto less = [] (point_type a, point_type b) {return a.x < b.x;};
  //r->print_status();
  timer t_query_total;
  t_query_total.start();
  for (size_t i = 0; i < query_num; i++) {
    r->insert_point_lazy(new_points[i]);
  }
  cout << endl;
  cout << "query end" << endl;
  t_query_total.stop();
  //r->print_status();
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

  reset_timers();
  delete r;
}

void test_loop(size_t n, int min_val, int max_val,
	       size_t iterations, size_t query_num, int type) {
  for (size_t i = 0; i < iterations; ++i) {
    vector<point_type> points = generate_points(n, min_val, max_val);
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

