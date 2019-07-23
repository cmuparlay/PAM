//#define GLIBCXX_PARALLEL

#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include "seg_sweep.h"
#include "pbbslib/get_time.h"

using namespace std;

int str_to_int(char* str) {
  return strtol(str, NULL, 10);
}

int random_hash(int seed, int a, int rangeL, int rangeR) {
  if (rangeL == rangeR) return rangeL;
  a = (a+seed) + (seed<<7);
  a = (a+0x7ed55d16) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = (a+seed>>5) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  a = (a^0xb55a4f09) ^ (a>>16);
  a = a % (rangeR-rangeL);
  if (a < 0) a += (rangeR-rangeL);
  a += rangeL;
  return a;
}

int win;
int dist;

double gaussrand()
{
  static double V1, V2, S;
  static int phase = 0;
  double X;
     
  if ( phase == 0 ) {
    do {
      double U1 = (double)rand() / RAND_MAX;
      double U2 = (double)rand() / RAND_MAX;
             
      V1 = 2 * U1 - 1;
      V2 = 2 * U2 - 1;
      S = V1 * V1 + V2 * V2;
    } while(S >= 1 || S == 0);
         
    X = V1 * sqrt(-2 * log(S) / S);
  } else
    X = V2 * sqrt(-2 * log(S) / S);
         
  phase = 1 - phase;
 
  return X;
}

vector<segment_t> generate_segs(size_t n, int a, int b) {
  vector<segment_t> ret(n);
  parallel_for (0, n, [&] (size_t i) {
    segment_t s;
    s.w = 1;
    s.y = random_hash('y', i, a, b);
    s.x1 = random_hash('l', i, a, b);
    s.x2 = random_hash('r', i, a, b);
    if (s.x1 > s.x2) std::swap(s.x1, s.x2);
    ret[i] = s;
    });
  return ret;
}

vector<query_t> generate_queries(size_t q, int a, int b) {
  vector<query_t> ret(q);
  parallel_for (0, q, [&] (size_t i) {
    query_t q;
    q.x = random_hash('q'*'x', i, a, b);
    q.y1 = random_hash('y'+1, i, a, b);
    if (dist == 0) {
      q.y2 = random_hash('y'*'y'+2, i, a, b);
    } else {
      //double dy_g = gaussrand();
      //int dy = floor(dy_g*win/2 + win);
      int dy = random_hash('q'*'y'+2, i, 0, win);
      q.y2 = q.y1 + dy;
    }
    if (q.y1 > q.y2) std::swap(q.y1,q.y2);
    ret[i] = q;
    });
  return ret;
}


void run_all(vector<segment_t>& segs,
	 size_t iteration, int min_val, int max_val, int query_num) {
  std::string benchmark_name = "Query-All";
  reset_timers();

  const size_t threads = num_workers();
  const size_t num_points = segs.size();
  SegmentQuery r(segs);
  r.print_allocation_stats();
  r.set_min_max(min_val, max_val);

  vector<query_t> queries = generate_queries(query_num, min_val, max_val);
  
  size_t counts[query_num];

  timer t_query_total;
  t_query_total.start();

  parallel_for (0, query_num, [&] (size_t i) {
    pbbs::sequence<mkey_t> a = r.query_points(queries[i]);
    counts[i] = a.size();
    });
  t_query_total.stop();

  size_t total = pbbs::reduce(pbbs::sequence<size_t>(counts,query_num),
			      pbbs::addm<size_t>());
  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "SegSweep"
       << "\tname=" << benchmark_name
       << "\tn=" << num_points
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


 // / timer deconst_tm;
//  deconst_tm.start();
  SegmentQuery::finish();
//  deconst_tm.stop();
}

/*
void run_sum(vector<segment_t> segs,
	 size_t iteration, int min_val, int max_val, int query_num) {
  std::string benchmark_name = "Query-Sum";
  reset_timers();

  const size_t threads = __cilkrts_get_nworkers();
  const size_t num_points = segs.size();
  SegmentQuery r(segs);
  r.set_min_max(min_val, max_val);
  
  vector<query_t> queries = generate_queries(query_num, min_val, max_val);
  
  size_t counts[query_num];

  timer t_query_total;
  t_query_total.start();
  cilk_for (int i = 0; i < query_num; i++) {
    counts[i] = r.query_sum(queries[i]);
  }

  t_query_total.stop();
  
  size_t total = pbbs::reduce_add(pbbs::sequence<size_t>(counts,query_num));
  
  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "SegSweep"
       << "\tname=" << benchmark_name
       << "\tn=" << num_points
       << "\tq=" << query_num
       << "\tp=" << __cilkrts_get_nworkers()
       << "\tmin-val=" << min_val
       << "\tmax-val=" << max_val
       << "\twin-mean=" << win
       << "\titeration=" << iteration
       << "\tbuild-time=" << total_tm.get_total()
       << "\treserve-time=" << reserve_tm.get_total()
       << "\tquery-time=" << t_query_total.get_total()
       << "\ttotal=" << total
       << endl;


  SegmentQuery::finish();
}*/


void run_sum(vector<segment_t>& segs,
   size_t iteration, int min_val, int max_val, int query_num) {
  std::string benchmark_name = "Query-Sum";
  reset_timers();

  const size_t threads = num_workers();
  const size_t num_points = segs.size();
  SegmentQuery r(segs);
  //r.print_allocation_stats();
  r.set_min_max(min_val, max_val);

  vector<query_t> queries = generate_queries(query_num, min_val, max_val);
  
  size_t counts[query_num];

  timer t_query_total;
  t_query_total.start();
  parallel_for (0, query_num, [&] (size_t i) {
    counts[i] = r.query_sum(queries[i]);
    });

  t_query_total.stop();

  size_t total = pbbs::reduce(pbbs::sequence<size_t>(counts,query_num),
			      pbbs::addm<size_t>());
    cout << "RESULT" << fixed << setprecision(3)
	 << "\talgo=" << "SegSweep"
	 << "\tname=" << benchmark_name
	 << "\tn=" << num_points
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
  
  SegmentQuery::finish();
}


int main(int argc, char** argv) {
	
  if (argc < 9) {
      cout << argv[0] << " <n> <min> <max> <rounds> <num_queries> <dist> <win> <qtype> [num_blocks]"<< std::endl;
      exit(1);
  }
  srand(2017);

  size_t n = str_to_int(argv[1]);
  int min_val  = str_to_int(argv[2]);
  int max_val  = str_to_int(argv[3]);
  dist = str_to_int(argv[6]);
  win = str_to_int(argv[7]);
  int query_num  = str_to_int(argv[5]);

  int type = str_to_int(argv[8]);

  num_blocks = 0; // default, sets to number of threads
  if (argc > 9) num_blocks = str_to_int(argv[9]);

  size_t iterations = str_to_int(argv[4]);

  for (size_t i = 0; i < iterations; ++i) {
    vector<segment_t> segs = generate_segs(n, min_val, max_val);
    if (type == 0) run_all(segs, i, min_val, max_val, query_num);
    else run_sum(segs, i, min_val, max_val, query_num);
  }

  return 0;
}

