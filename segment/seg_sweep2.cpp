#include <algorithm>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include "seg_sweep2.h"
#include "pbbslib/get_time.h"
#include "pbbslib/random.h"
#include "pbbslib/utilities.h"

using namespace std;


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
	a = (a+seed>>5) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	  a = a % (rangeR-rangeL);
	  if (a < 0) a += (rangeR-rangeL);
	  a += rangeL;
	return a;
}


segment_2d* generate_segs(size_t n, int a, int b) {
    segment_2d *v = new segment_2d[n];
    //#pragma omp parallel for
    parallel_for (0, n, [&] (size_t i) {
		int y = random_hash('y', i, a, b);
        int xl = random_hash('l', i, a, b);
        int xr = random_hash('r', i, a, b);
		if (xl > xr) {
			int t = xl; xl = xr; xr = t;
		}
		v[i] = segment_2d(y,segment_1d(xl,xr));
		//cout << y << " " << xl << " " << xr << endl;
      });
   return v;
}

segment_2d* generate_queries(size_t q, int a, int b) {
    segment_2d *v = new segment_2d[q];
    parallel_for (0, q, [&] (size_t i) {
        int x = random_hash('q'*'x', i, a, b);
	    int yl = random_hash('y'+1, i, a, b);
		int yr = random_hash('y'*'y'+2, i, a, b);
       if (yl > yr) {
  	  		int t = yl; yl = yr; yr = t;
  	  	}
		 
       v[i] = segment_2d(x,segment_1d(yl,yr));
      });
    
    return v;
}

void reset_timers() {
    reserve_tm.reset(); build_tm.reset(); query_tm.reset();
}

int main(int argc, char** argv) {
    if (argc != 9) {
      cout << argv[0] << " <n> <min> <max> <rounds> <num_queries> <dist> <win> <qtype> [num_blocks]"<< std::endl;
      exit(1);
    }
    srand(2017);
  
    size_t n = str_to_int(argv[1]);
    int min_val  = str_to_int(argv[2]);
    int max_val  = str_to_int(argv[3]);
    size_t iterations = str_to_int(argv[4]);
    dist = str_to_int(argv[6]);
    win = str_to_int(argv[7]);
    int num_queries  = str_to_int(argv[5]);

    int type = str_to_int(argv[8]);
    if (type == 0) return 0;

    size_t threads = num_workers();

    size_t r[num_queries];	
	cout << "start!" << endl;
  segment_2d* segs = generate_segs(n, min_val, max_val);
  cout << "generated segments" << endl;
  segment_2d* queries = generate_queries(num_queries, min_val, max_val);
  cout << "generated queries" << endl;
    for (size_t i = 0; i < iterations; ++i) {
      build_tm.start();
      segment_map_2d a = segment_map_2d(segs, n);
      build_tm.stop();

	  print_stats();
	  
      query_tm.start();
      parallel_for (0, num_queries, [&] (size_t i) {
	  r[i] = a.count(queries[i]);
	});
      query_tm.stop();

      size_t total = pbbs::reduce(pbbs::sequence<size_t>(r,num_queries),
				  pbbs::addm<size_t>());

      cout << "RESULT" << fixed << setprecision(3)
         << "\talgo=" << "SegCount"
         << "\tname=" << "Query-Sum"
         << "\tn=" << n
         << "\tq=" << num_queries
         << "\tp=" << threads
         << "\tmin-val=" << min_val
         << "\tmax-val=" << max_val
         << "\twin-mean=" << win
         << "\titeration=" << i
         << "\tbuild-time=" << total_tm.get_total()
         << "\treserve-time=" << reserve_tm.get_total()
         << "\tquery-time=" << query_tm.get_total()
         << "\ttotal=" << total
         << endl;


      reset_timers();
      a.finish();
      free(segs); free(queries);
    }
    return 0;
}

