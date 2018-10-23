#include <algorithm>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include <cilk/cilk_api.h>
#include "segment_star.h"
#include "pbbs-include/get_time.h"
#include "pbbs-include/random.h"
#include "pbbs-include/utilities.h"

using namespace std;
timer reserve_tm, build_tm, query_tm;

int str_to_int(char* str) {
  return strtol(str, NULL, 10);
}

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
    cilk_for (int i = 0; i < n; ++i) {
		int y = random_hash('y', i, a, b);
        int xl = random_hash('l', i, a, b);
        int xr = random_hash('r', i, a, b);
		if (xl > xr) {
			int t = xl; xl = xr; xr = t;
		}
		v[i] = segment_2d(y,segment_1d(xl,xr));
		//cout << y << " " << xl << " " << xr << endl;
    }
   return v;
}

segment_2d* generate_queries(size_t q, int a, int b) {
    segment_2d *v = new segment_2d[q];
    cilk_for (int i = 0; i < q; ++i) {
        int x = random_hash('q'*'x', i, a, b);
		    int yl = random_hash('y'+1, i, a, b);

		int yr = random_hash('y'*'y'+2, i, a, b);
       if (yl > yr) {
  	  		int t = yl; yl = yr; yr = t;
  	  	}
		 
       v[i] = segment_2d(x,segment_1d(yl,yr));
    }
    
    return v;
}

void reset_timers() {
    reserve_tm.reset(); build_tm.reset(); query_tm.reset();
}

int main(int argc, char** argv) {
    if (argc != 6) {
      cout << argv[0] << " <n> <min> <max> <rounds> <num_queries>"<< std::endl;
      exit(1);
    }
    srand(2017);
  
    size_t n = str_to_int(argv[1]);
    int min_val  = str_to_int(argv[2]);
    int max_val  = str_to_int(argv[3]);
    size_t iterations = str_to_int(argv[4]);
    int num_queries  = str_to_int(argv[5]);

    size_t threads = __cilkrts_get_nworkers();

    size_t r[num_queries];	
	segment_2d* segs = generate_segs(n, min_val, max_val);
	segment_2d* queries = generate_queries(num_queries, min_val, max_val);

    for (size_t i = 0; i < iterations; ++i) {
      reserve_tm.start();
      segment_map_2d::reserve(n);
      reserve_tm.stop();
      
      build_tm.start();
      segment_map_2d a = segment_map_2d(segs, n);
      build_tm.stop();

	  print_stats();
	  
      query_tm.start();
      parallel_for (size_t i = 0; i < num_queries; i++) {
  	      r[i] = a.count(queries[i]);
  	  }
      query_tm.stop();

      size_t total = pbbs::reduce_add(sequence<size_t>(r,num_queries));

      cout << "RESULT" << fixed << setprecision(3)
         << "\talgo=" << "SegCount"
         << "\tname=" << "Query-Sum"
         << "\tn=" << n
         << "\tq=" << num_queries
         << "\tp=" << threads
         << "\tmin-val=" << min_val
         << "\tmax-val=" << max_val
         << "\titeration=" << i
         << "\tbuild-time=" << build_tm.get_total()
         << "\treserve-time=" << reserve_tm.get_total()
         << "\tquery-time=" << query_tm.get_total()
         << "\ttotal=" << total
         << endl;


      reset_timers();
      a.finish();
      //free(segs); free(queries);
    }
    return 0;
}

