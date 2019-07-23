//#define GLIBCXX_PARALLEL

#include <algorithm>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include "segment.h"
#include "pbbslib/get_time.h"
#include "pbbslib/sequence_ops.h"

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

vector<seg_type_2d> generate_segs(size_t n, int a, int b) {
    vector<seg_type_2d> ret(n);
    parallel_for (0, n, [&] (size_t i) {
		ret[i].first = random_hash('y', i, a, b);
        int xl = random_hash('l', i, a, b);
        int xr = random_hash('r', i, a, b);
		if (xl > xr) {
			int t = xl; xl = xr; xr = t;
		}
		ret[i].second.first = xl; ret[i].second.second = xr;
		//cout << xl << " " << xr << " " << ret[i].first << endl;
      });

   return ret;
}

vector<query_type> generate_queries(size_t q, int a, int b) {
    vector<query_type> ret(q);
    //#pragma omp parallel for
    parallel_for (0, q, [&] (size_t i) {
        ret[i].x = random_hash('q'*'x', i, a, b);
		int yl = random_hash('y'+1, i, a, b);
		int dy = random_hash('q'*'y'+2, i, 0, win);
		int yr = yl+dy;
        if (dist == 0) yr = random_hash('y'*'y'+2, i, a, b);
		if (yl > yr) {
			int t = yl; yl = yr; yr = t;
		}
		ret[i].y1 = yl; ret[i].y2 = yr;
      });
    return ret;
}


void reset_timers() {
	reserve_tm.reset();
	init_tm.reset(); sort_tm.reset(); build_tm.reset(); total_tm.reset(); 
}

void run_all(vector<seg_type_2d>& segs, size_t iteration, int min_val, int max_val, int query_num) {
  const size_t threads = num_workers();
     std::string benchmark_name = "Query-All";

    //timer t;
	//t.start();
	const size_t num_points = segs.size();
	SegmentQuery<int> r(segs);
	//r->print_tree();
	//double tm = t.stop();

	vector<query_type> queries = generate_queries(query_num, min_val, max_val);
	//cout << endl;
	size_t counts[query_num];

	timer t_query_total;
	t_query_total.start();
	int tot = 0;
	parallel_for (0, query_num, [&] (size_t i) {
	    int cnt = r.query_count(queries[i], 0);
	    pbbs::sequence<seg_type_2d> out(cnt*2);
	    r.query_points(queries[i], out.begin());
	    counts[i] = out.size();
	  });

	t_query_total.stop();
	size_t total = pbbs::reduce(pbbs::sequence<size_t>(counts,query_num),
				    pbbs::addm<size_t>());

   cout << "RESULT" << fixed << setprecision(3)
	<< "\talgo=" << "SegTree"
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

   reset_timers();
   SegmentQuery<int>::finish();
}



void run_sum(vector<seg_type_2d>& segs, size_t iteration, int min_val, int max_val, size_t query_num) {
  const size_t threads = num_workers();
    std::string benchmark_name = "Query-Sum";
    //timer t;
	//t.start();
	const size_t num_points = segs.size();
	SegmentQuery<int> r(segs);
	r.print_stats();
	//r->print_tree();
	//double tm = t.stop();

	vector<query_type> queries = generate_queries(query_num, min_val, max_val);
	//cout << endl;
    size_t* counts = new size_t[query_num];

	timer t_query_total;
	t_query_total.start();

	cout << "start query!" << endl;
	parallel_for (0, query_num, [&] (size_t i) {
	    counts[i] = r.query_count(queries[i], 0);
	  });

	t_query_total.stop();
	size_t total = pbbs::reduce(pbbs::sequence<size_t>(counts,query_num),
				    pbbs::addm<size_t>());
	
    cout << "RESULT" << fixed << setprecision(5)
	 << "\talgo=" << "SegTree"
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

    reset_timers();
    SegmentQuery<int>::finish();

}


int main(int argc, char** argv) {

    if (argc != 9) {
        cout << "Invalid number of command line arguments" << std::endl;
        exit(1);
    }
	srand(2017);

    size_t n = str_to_int(argv[1]);
    int min_val  = str_to_int(argv[2]);
    int max_val  = str_to_int(argv[3]);
	dist = str_to_int(argv[6]);
	win = str_to_int(argv[7]);
	size_t query_num  = str_to_int(argv[5]);
    int type = str_to_int(argv[8]);

    size_t iterations = str_to_int(argv[4]);
	//int query_num  = str_to_int(argv[5]);
	//int query_num = 1000;
	
    for (size_t i = 0; i < iterations; ++i) {
        vector<seg_type_2d> segs = generate_segs(n, min_val, max_val);
	    if (type == 0) run_all(segs, i, min_val, max_val, query_num);
	    else run_sum(segs, i, min_val, max_val, query_num);
    }

    return 0;
}

