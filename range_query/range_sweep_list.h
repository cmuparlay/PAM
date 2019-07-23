#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <parallel/algorithm>
#include "pam.h"
#include "pbbslib/get_time.h"
#include "../common/sweep.h"

using namespace std;

const int max_int = 1000000000;

timer deconst_tm;
timer reserve_tm;
timer sort_tm;
timer const_tm1, const_tm2;
timer prefix_tm, run_tm;

using coord = int;
using weight = float;
using unwPoint = pair<coord,coord>;

struct Point {
  coord x, y;
  weight w;
  Point() {}
  Point(coord x, coord y, weight w) : x(x), y(y), w(w) {}
};

  coord get_max(coord a, coord b) {
	  return (a>b)?a:b;
  }  
  
struct RangeQuery {

  struct aug_max {
	using key_t = unwPoint;
	using val_t = coord;
	static bool comp(key_t a, key_t b) { return a.first < b.first;}
    using aug_t = coord;
    static aug_t from_entry(key_t k, val_t v) {return v;}
    static aug_t combine(aug_t a, aug_t b) {return get_max(a,b);}
    static aug_t get_empty() {return 0;}
  };

  using inner_elt = pair<unwPoint, coord>;
  using inner_map = aug_map<aug_max>;
  //using outer_elt = pair<coord, inner_map>;
  using inner_node = inner_map::node;
  coord* xs;
  inner_map* ts;
  int xs_size;
  //using outer_map = tree_map<coord, inner_map>;

  //outer_map M;

  RangeQuery(vector<Point>& points) {
    size_t n = points.size();
    Point* A = points.data();
	

    reserve_tm.start();
    inner_map::reserve(37 * n);
    reserve_tm.stop();
        
    auto less = [] (Point a, Point b) {return a.x < b.x;};

    run_tm.start();
   // sort_tm.start();
    pbbs::sample_sort(A,n,less);
   // sort_tm.stop();

    xs = pbbs::new_array<coord>(n);
	xs_size = n;
	inner_elt *ys = pbbs::new_array<inner_elt>(n);
    cilk_for (size_t i = 0; i < n; ++i) {
	  xs[i] = A[i].x;
      ys[i] = inner_elt(make_pair(A[i].y, A[i].x),A[i].x);
    }

	auto insert = [&] (inner_map m, inner_elt a) {
      inner_map mr = m; mr.insert(a); return mr;};

    auto build = [&] (inner_elt* s, inner_elt* e) {
      return inner_map(s,e);};

	auto fold = [&] (inner_map m1, inner_map m2) {
      return inner_map::map_union(m1, std::move(m2));};

    ts = sweep<inner_map>(ys, n, inner_map(), insert, build, fold);
	
	pbbs::delete_array(ys, n);
	run_tm.stop();

    //cout << "allocation stats:" ;  inner_map::print_allocation_stats();
	//cout << "xs_size: " << xs_size << endl;
  }

	int get_index(const coord q) {
		int l = 0, r = xs_size;
		int mid = (l+r)/2;
		if (xs[0]>q) return -1;
		while (l<r-1) {
			if (xs[mid] == q) break;
			if (xs[mid] < q) l = mid;
			else r = mid;
			mid = (l+r)/2;
			//cout << l << " " << r << " "  << mid << endl;
		}
		return mid;
	}
	
	template <class OutIter>
	void report_all(inner_node* r, int x1, OutIter out) {
		if (!r) return;
		if (r->entry.second < x1) return;
		if (r->entry.first.second > x1) {
			out=r->entry.first.first; ++out;
		}
		report_all(r->lc, x1, out);
		report_all(r->rc, x1, out);
	}

  vector<unwPoint> query(coord x1, coord y1, coord x2, coord y2) {
	vector<unwPoint> ret;
	int r = get_index(x2);
	if (r<0) return ret;
	inner_map m = ts[r];
	int temp_s = m.size();
	if (temp_s == 0) return ret;
	if (m.aug_val()<x1) return ret;
	
	inner_node* cur_root = m.root;
	while (cur_root) {
		if (cur_root->entry.first.first.first < y1) {
			cur_root = cur_root->rc; continue;
		}
		if (cur_root->entry.first.first.first > y2) {
			cur_root = cur_root->lc; continue;
		}
		break;
	}
	if (!cur_root) return ret;
	if (cur_root->entry.first.second > x1) {
		ret.push_back(cur_root->entry.first.first);
	}
	inner_node* lp = cur_root->lc;
	while (lp) {
		if (lp->entry.second<x1) break;
		if (lp->entry.first.first.first<y1) {
			lp = lp->rc; continue;
		}
		if (lp->entry.first.second > x1) {
			ret.push_back(lp->entry.first.first);
		}
		report_all(lp->rc, x1, std::back_inserter(ret));
		lp = lp->lc;
	}

	inner_node* rp = cur_root->rc;
	while (rp) {
		if (rp->entry.second<x1) break;
		if (rp->entry.first.first.first>y2) {
			rp = rp->lc; continue;
		}
		if (rp->entry.first.second > x1) {
			ret.push_back(rp->entry.first.first);
		}
		report_all(rp->lc, x1, std::back_inserter(ret));
		rp = rp->rc;
	}
	
	return ret;
  }	
  
  vector<unwPoint> query2(coord x1, coord y1, coord x2, coord y2) {
	vector<unwPoint> ret;
	int r = get_index(x2);
	if (r<0) return ret;
	inner_map m = inner_map::range(ts[r], make_pair(y1,0), make_pair(y2, max_int));
	int temp_s = m.size();
	if (temp_s == 0) return ret;
	auto f = [&] (coord a) {
		return (a>x1);
	};
	inner_map m2 = inner_map::aug_filter(m,f);
	temp_s = m2.size();
	if (temp_s == 0) return ret;
	
	ret.resize(temp_s);
	auto g = [&] (inner_elt a, size_t i) {
		ret[i] = a.first;
	};
	inner_map::map_index(m2,g);

	return ret;
  }
};
