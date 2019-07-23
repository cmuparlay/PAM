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
#include "range_sweep.h"
#include "pbbslib/get_time.h"
#include "pbbslib/parse_command_line.h"

using namespace std;

using tuple_type = tuple<coord, coord, coord, coord>;

int str_to_int(char* str) {
  return strtol(str, NULL, 10);
}

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
  if (a < 0) a += (rangeR-rangeL);
  a += rangeL;
  return a;
}

int win;
int dist;

// generate random input points. The coordinate are uniformly distributed in [a, b]
vector<Point> generate_points(size_t n, coord a, coord b) {
  vector<Point> ret(n);

  parallel_for(0, n, [&] (size_t i) {
    ret[i].x = random_hash('x', i, a, b);
    ret[i].y = random_hash('y', i, a, b);
    ret[i].w = 1; //random_hash('w', i, a, b);
    });

  return ret;
}


// generate random query windows:
// if dist == 0: the coordinate of the query windows are uniformly random in [a, b]
// otherwise: the average size of query windows is win^2/4
vector<tuple_type> generate_queries(size_t q, coord a, coord b) {
  vector<tuple_type> ret(q);
  parallel_for (0, q, [&] (size_t i) {
    coord x1 = random_hash('q'*'x', i*2, a, b);
    coord y1 = random_hash('q'*'y', i*2, a, b);
  int dx = random_hash('d'*'x', i, 0, win);
  int dy = random_hash('d'*'y', i, 0, win);
    int x2 = x1+dx;
    int y2 = y1+dy;
  if (dist == 0) {
    x2 = random_hash('q'*'x', i*2+1, a, b);
    y2 = random_hash('q'*'y', i*2+1, a, b);
  }
    if (x1 > x2) {
      coord t = x1; x1 = x2; x2 = t;
    }
    if (y1 > y2) {
      coord t = y1; y1 = y2; y2 = t;
    }
    ret[i] = tuple_type(x1, y1, x2, y2);
    });

  return ret;
}

void reset_timers() {
     reserve_tm.reset();
     deconst_tm.reset();
     sort_tm.reset();
     //sweep_1_tm.reset();
     //sweep_2_tm.reset();
     //sweep_3_tm.reset();
     run_tm.reset();
}

void run(vector<Point>& points, size_t iteration, 
	 coord min_val, coord max_val, size_t query_num) {
  string benchmark_name = "Query-Sum";
  
  RangeQuery *r = new RangeQuery(points);
  // RangeQuery::print_allocation_stats();
  
  r->print_allocation_stats();

  //size_t query_num = points.size(); //1000000;
  vector<tuple_type> queries = generate_queries(query_num, min_val, max_val);
  size_t counts[query_num];
  
  timer t_query;
  t_query.start();
  parallel_for (0, query_num, [&] (size_t i) {
    counts[i] = r->query(std::get<0>(queries[i]), std::get<1>(queries[i]), 
	     std::get<2>(queries[i]), std::get<3>(queries[i]));
		 //cout << counts[i] << endl;
    });
  double tm_query = t_query.stop();
  size_t total = pbbs::reduce(pbbs::sequence<size_t>(counts,query_num),
			      pbbs::addm<size_t>());

  RangeQuery::finish();
    
 // deconst_tm.start();
  delete r;
// deconst_tm.stop();
	
  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "RageSweepSum"
       << "\tname=" << benchmark_name
       << "\tn=" << points.size()
       << "\tq=" << query_num
       << "\tp=" << num_workers()
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
    vector<Point> points = generate_points(n, min_val, max_val);
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
