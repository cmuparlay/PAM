//#define GLIBCXX_PARALLEL

#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include "rec_sweep.h"
#include "pbbslib/get_time.h"

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

vector<rec_type> generate_recs(size_t n, int a, int b) {
    vector<int> p1(2*n+1);
	vector<int> p2(2*n+1);
//	int* p2;
//	p2 = pbbs::new_array<int>(2*n+1);
	p1[0] = 0; p2[0] = 0;
    for (int i = 1; i < 2*n+1; ++i) {
		p1[i] = p1[i-1] + random_hash('x', i, 1, 30);
		//if (p1[i]%2) p1[i]++;
		p2[i] = p2[i-1] + random_hash('y', i, 1, 30);
		//if (p2[i]%2) p2[i]++;
    }
	max_x = p1[2*n]; max_y = p2[2*n];
	cout << max_x << " " << max_y << endl;

	//srand(unsigned(time(NULL)));
	for (size_t i = 0; i < 2*n+1; ++i) {
		size_t j = random_hash('s', i, 0, 2*n+1-i);
		swap(p1[i], p1[i+j]);
		j = random_hash('s'*'s'+920516, i, 0, 2*n+1-i);
		swap(p2[i], p2[i+j]);
	}
	vector<rec_type> ret(n);
	parallel_for (0, n, [&] (size_t i) {
		if (p1[i*2+1]>p1[i*2+2]) swap(p1[i*2+1], p1[i*2+2]);
		if (p2[i*2+1]>p2[i*2+2]) swap(p2[i*2+1], p2[i*2+2]);
		ret[i].first.first = p1[i*2+1];
		ret[i].first.second = p2[i*2+1];
		ret[i].second.first = p1[i*2+2];
		ret[i].second.second = p2[i*2+2];
		if (dist != 0) {
			int deltax = random_hash('d'*'x', i, 1, win);
			int deltay = random_hash('d'*'y', i, 1, win);
			ret[i].second.first = p1[i*2+1]+deltax;
			ret[i].second.second = p2[i*2+1]+deltay;
		}
	  });
	using pp = pair<int, int>;
	//pp* x = pbbs::new_array<pp>(n*2);
	//pp* y = pbbs::new_array<pp>(n*2);
	pbbs::sequence<pp> x(n*2);
	pbbs::sequence<pp> y(n*2);
	bool dup = true;
	while (dup) {
	  	parallel_for (0, n, [&] (size_t i) {
			x[i*2] = make_pair(x1(ret[i]), i);
			x[i*2+1] = make_pair(x2(ret[i]), -i);
			y[i*2] = make_pair(y1(ret[i]), i);
			y[i*2+1] = make_pair(y2(ret[i]), -i);
		  });
		auto less = [&](pp a, pp b) {return a.first<b.first;};
		x = pbbs::sample_sort(x, less);
		y = pbbs::sample_sort(y, less);
		dup = false;
		for (int i = 1; i < 2*n; i++) {
			if (x[i].first <= x[i-1].first) {
				dup = true;
				int index = x[i].second;
				//cout << i << " x: " << x[i].first << " " << x[i-1].first << " index: " << index << endl;
				int coor = x[i-1].first + 1;
				x[i].first = coor;
				if (index > 0) {
					ret[index].first.first = coor;
				} else {
					ret[-index].second.first = coor;
				}
			}
			if (y[i].first <= y[i-1].first) {
				dup = true;
				int index = y[i].second;
				//cout << "y:" << y[i].first << " " << y[i-1].first << " index: " << index << endl;
				int coor = y[i-1].first + 1;
				y[i].first = coor;
				if (index > 0) {
					ret[index].first.second = coor;
				} else {
					ret[-index].second.second = coor;
				}
			}
		}
		if (dup) cout << "dup!" << endl;
	}
	
	for (size_t i = 0; i < n; ++i) {
		if (ret[i].first.first > ret[i].second.first) swap(ret[i].first.first, ret[i].second.first);
		if (ret[i].first.second > ret[i].second.second) swap(ret[i].first.second, ret[i].second.second);
		size_t j = random_hash('S', i, 0, n-i);
		swap(ret[i], ret[i+j]);
	}
    return ret;
}

vector<query_type> generate_queries(size_t q, int a, int b) {
    vector<query_type> ret(q);
    parallel_for (0, q, [&] (size_t i) {
        ret[i].x = random_hash('q'*'x', i, 0, max_x);
		if (ret[i].x%2 == 0) ret[i].x++;
		ret[i].y = random_hash('y'+1, i, 0, max_y);
		if (ret[i].y%2 == 0) ret[i].y++;
      });
    return ret;
}

void run_all(vector<rec_type>& recs,
  size_t iteration, int min_val, int max_val, int query_num) {
  std::string benchmark_name = "Query-All";
  reset_timers();

  const size_t threads = num_workers();
  const size_t num_points = recs.size();
  RectangleQuery r(recs);
  r.print_allocation_stats();

  vector<query_type> queries = generate_queries(query_num, min_val, max_val);
  
  size_t counts[query_num];

  timer t_query_total;
  t_query_total.start();

  parallel_for (0, query_num, [&] (size_t i) {
    pbbs::sequence<rec_type> a = r.query_points(queries[i]);
    counts[i] = a.size();
    });
  t_query_total.stop();

  size_t total = pbbs::reduce(pbbs::sequence<size_t>(counts,query_num),
			      pbbs::addm<size_t>());

  cout << "RESULT" << fixed << setprecision(3)
       << "\talgo=" << "RecSweep"
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
  RectangleQuery::finish();
//  deconst_tm.stop();
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

    vector<rec_type> recs = generate_recs(n, min_val, max_val);
  	/*for (int i = 0; i < n; i++) {
		print_out(recs[i]); cout << endl;
	}*/
	
  for (size_t i = 0; i < iterations; ++i) {
	run_all(recs, i, min_val, max_val, query_num);
    //if (type == 0) run_all(segs, i, min_val, max_val, query_num);
    //else run_sum(segs, i, min_val, max_val, query_num);
  }

  return 0;
}

