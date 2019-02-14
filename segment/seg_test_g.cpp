//#define GLIBCXX_PARALLEL

#include <algorithm>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include "segment_g.h"
#include "pbbslib/get_time.h"
#include "pbbslib/sequence_ops.h"
#include "pbbslib/parse_command_line.h"

using namespace std;

int str_to_int(char* str) {
    return strtol(str, NULL, 10);
}

int max_x, max_y;

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

vector<seg_type> generate_segs(size_t n) {
    vector<int> p1(2*n+1);
	int* p2;
	p2 = pbbs::new_array<int>(2*n+1);
	p1[0] = 0; p2[0] = 0;
    for (int i = 1; i < 2*n+1; ++i) {
		p1[i] = p1[i-1] + random_hash('x', i, 1, 10);
		if (p1[i]%2) p1[i]++;
		p2[i] = p2[i-1] + random_hash('y', i, 1, 10);
		if (p2[i]%2) p2[i]++;
    }
	max_x = p1[2*n]; max_y = p2[2*n];

	for (size_t i = 0; i < 2*n+1; ++i) {
		size_t j = random_hash('s', i, 0, 2*n+1-i);
		swap(p1[i], p1[i+j]);
	}
	vector<seg_type> ret(n);
	parallel_for (0, n, [&] (size_t i) {
		if (p1[i*2+1]>p1[i*2+2]) swap(p1[i*2+1], p1[i*2+2]);
		ret[i].first.first = p1[i*2+1];
		ret[i].first.second = p2[i*2+1];
		ret[i].second.first = p1[i*2+2];
		ret[i].second.second = p2[i*2+2];
	  });
	for (size_t i = 0; i < n; ++i) {
		size_t j = random_hash('S', i, 0, n-i);
		swap(ret[i], ret[i+j]);
	}
    return ret;
}

vector<seg_type> generate_segs_small(size_t n, int win_x) {
    vector<int> p1(2*n+1);
	int* p2;
	p2 = pbbs::new_array<int>(2*n+1);
	p1[0] = 0; p2[0] = 0;
    for (int i = 1; i < 2*n+1; ++i) {
		p1[i] = p1[i-1] + random_hash('x', i, 1, 10);
		if (p1[i]%2) p1[i]++;
		p2[i] = p2[i-1] + random_hash('y', i, 1, 10);
		if (p2[i]%2) p2[i]++;
    }
	max_x = p1[2*n]; max_y = p2[2*n];

	for (size_t i = 0; i < 2*n+1; ++i) {
		size_t j = random_hash('s', i, 0, 2*n+1-i);
		swap(p1[i], p1[i+j]);
	}
	vector<seg_type> ret(n);
	parallel_for (0, n, [&] (size_t i) {
		if (p1[i*2+1]>p1[i*2+2]) swap(p1[i*2+1], p1[i*2+2]);
		ret[i].first.first = p1[i*2+1];
		ret[i].first.second = p2[i*2+1];
		ret[i].second.first = p1[i*2+1]+2*random_hash('d', i, 1, win_x);
		ret[i].second.second = p2[i*2+2];
	  });
	for (size_t i = 0; i < n; ++i) {
		size_t j = random_hash('S', i, 0, n-i);
		swap(ret[i], ret[i+j]);
	}
    return ret;
}

vector<query_type> generate_queries(size_t q) {
    vector<query_type> ret(q);
    parallel_for (0, q, [&] (size_t i) {
        ret[i].x = random_hash('q'*'x', i, 0, max_x);
		if (ret[i].x%2 == 0) ret[i].x++;
		int yl = random_hash('y'+1, i, 0, max_y);
		if (yl%2 == 0) yl++;
		int dy = random_hash('q'*'y'+2, i, 1, win);
		int yr = yl+dy;
        if (dist == 0) yr = random_hash('y'*'y'+2, i, 0, max_y);
		if (yr%2 == 0) yr++;
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

void run_all(vector<seg_type>& segs, size_t iteration, int query_num) {
  const size_t threads = num_workers();
     std::string benchmark_name = "Query-All";

	const size_t num_points = segs.size();
	SegmentQuery<int> r(segs);
	cout << "Constructed!" << endl;

	vector<query_type> queries = generate_queries(query_num);
	size_t counts[query_num];
	r.print_stats();

	timer t_query_total;
	t_query_total.start();
	int tot = 0;
	parallel_for (0, query_num, [&] (size_t i) {
		int cnt = r.query_count(queries[i], 0);
		sequence<seg_type> out(cnt*2);
		seg_type* outiter = out.as_array();
		counts[i] = r.query_points(queries[i], outiter);
	  });

	t_query_total.stop();
    size_t total = pbbs::reduce_add(sequence<size_t>(counts,query_num));

   cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "SegTree"
       << "\tname=" << benchmark_name
       << "\tn=" << num_points
       << "\tq=" << query_num
       << "\tp=" << num_workers()
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

void run_sum(vector<seg_type>& segs, size_t iteration, size_t query_num) {
	const size_t threads = num_workers();
    std::string benchmark_name = "Query-Sum";
	const size_t num_points = segs.size();
	SegmentQuery<int> r(segs);
	r.print_stats();

	vector<query_type> queries = generate_queries(query_num);
    size_t* counts = new size_t[query_num];

	timer t_query_total;
	t_query_total.start();

	cout << "start query!" << endl;
	parallel_for (0, query_num, [&] (size_t i) {
		counts[i] = r.query_count(queries[i], 0);
	  });

	t_query_total.stop();
    size_t total = pbbs::reduce_add(sequence<size_t>(counts,query_num));
	
    cout << "RESULT" << fixed << setprecision(5)
	 << "\talgo=" << "SegTree"
	 << "\tname=" << benchmark_name
	 << "\tn=" << num_points
	 << "\tq=" << query_num
	 << "\tp=" << num_workers()
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
	
    for (size_t i = 0; i < iterations; ++i) {
	    if (type < 2) {
			vector<seg_type> segs = generate_segs(n);
			if (type == 0) run_all(segs, i, query_num);
			if (type == 1) run_sum(segs, i, query_num);
		}			
		if (type == 2) {
			vector<seg_type> segs = generate_segs_small(n, win_x);
			run_all(segs, i, query_num);
		}
    }

    return 0;
}

