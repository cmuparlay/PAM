#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <parallel/algorithm>
#include "pam.h"
#include "pbbslib/get_time.h"

using namespace std;

timer init_tm, build_tm, total_tm, aug_tm, sort_tm, reserve_tm, outmost_tm, globle_tm, g_tm;
double early;

template<typename c_type, typename w_type>
struct Point {
  c_type x, y;
  w_type w;
  Point(c_type _x, c_type _y, w_type _w) : x(_x), y(_y), w(_w){}
  Point(){}
};

template<typename value_type>
inline bool inRange(value_type x, value_type l, value_type r) {
  return ((x>=l) && (x<=r));
}

template<typename c_type, typename w_type>
struct RangeQuery {
  using x_type = c_type;
  using y_type = c_type;
  using point_type = Point<c_type, w_type>;
  using point_x = pair<x_type, y_type>;
  using point_y = pair<y_type, x_type>;
  using main_entry = pair<point_x, w_type>;
  
  struct inner_map_t {
    using key_t = point_y;
    using val_t = w_type;
    static bool comp(key_t a, key_t b) { return a < b;}
    using aug_t = w_type;
    inline static aug_t from_entry(key_t k, val_t v) { return v; }
    inline static aug_t combine(aug_t a, aug_t b) { return a+b; }
    static aug_t get_empty() { return 0;}
  };

  using inner_map = aug_map<inner_map_t>;

  struct outer_map_t {
    using key_t = point_x;
    using val_t = w_type;
    static bool comp(key_t a, key_t b) { return a < b;}
    using aug_t = inner_map;
    inline static aug_t from_entry(key_t k, val_t v) {
      return aug_t(make_pair(point_y(k.second, k.first), v));
    }
    inline static aug_t combine(aug_t a, aug_t b) {
      return aug_t::map_union(a, b, [](w_type x, w_type y) {return x+y;});
    }
    static aug_t get_empty() { return aug_t();}
  };

  using outer_map = aug_map<outer_map_t>;

  RangeQuery(vector<point_type>& points) {
    construct(points);
  }

  ~RangeQuery() {
    //outer_map::finish();
    //inner_map::finish();
  }

  static void reserve(size_t n) {
    reserve_tm.start();
    outer_map::reserve(n);
    inner_map::reserve(24*n);
    reserve_tm.stop();
  }
  
  static void finish() {
    outer_map::finish();
    inner_map::finish();
  }
  
  void construct(vector<point_type>& points) {
    const size_t n = points.size();
		
    pair<point_x, w_type> *pointsEle = new pair<point_x, w_type>[n];
		
    total_tm.start();
	
    parallel_for (0, n, [&] (size_t i) {
      pointsEle[i] = make_pair(make_pair(points[i].x, points[i].y), points[i].w);
      });
    range_tree = outer_map(pointsEle, pointsEle + n);

    delete[] pointsEle;
    total_tm.stop();
  }
  
  struct count_t {
    point_y y1, y2;
    int r;
    count_t(point_y y1, point_y y2) : y1(y1), y2(y2), r(0) {}
    //void add_entry(point_x p, w_type wp) {
    void add_entry(pair<point_x,w_type> e) {
      if (e.first.second >= y1.first && e.first.second <= y2.first) r += 1;
    }
    void add_aug_val(inner_map a) {
      r += (a.rank(y2) - a.rank(y1));}
  };
      
  w_type query_count(x_type x1, y_type y1, x_type x2, y_type y2) {
    count_t qrc(make_pair(y1,x1), make_pair(y2,x2));
    range_tree.range_sum(make_pair(x1,y1), make_pair(x2,y2), qrc);
    return qrc.r;
  }

  struct sum_t {
    point_y y1, y2;
    int r;
    sum_t(point_y y1, point_y y2) : y1(y1), y2(y2), r(0) {}
    //void add_entry(point_x p, w_type wp) {
    void add_entry(pair<point_x,w_type> e) { // p, w_type wp) {
      if (e.first.second >= y1.first && e.first.second <= y2.first) r += e.second;
    }
    void add_aug_val(inner_map a) { r += a.aug_range(y1, y2); }
  };
      
  w_type query_sum(x_type x1, y_type y1, x_type x2, y_type y2) {
    sum_t qrs(make_pair(y1,x1), make_pair(y2,x2));
    range_tree.range_sum(make_pair(x1,y1), make_pair(x2,y2), qrs);
    return qrs.r;
  }

  struct range_t {
    point_y y1, y2;
    point_y* out;
    range_t(point_y y1, point_y y2, point_y* out) : y1(y1), y2(y2), out(out) {}
    void add_entry(pair<point_x,w_type> e) {
      if (e.first.second >= y1.first && e.first.second <= y2.first) {
	*out++ = point_y(e.first.second,e.first.second);}
    }
    void add_aug_val(inner_map a) {
      inner_map r = inner_map::range(a, y1, y2);
      size_t s = r.size();
      inner_map::keys(std::move(r),out);
      out += s;
    }
  };
  
  pbbs::sequence<point_y> query_range(x_type x1, y_type y1, x_type x2, y_type y2) {
    size_t n = query_count(x1,y1,x2,y2);
    pbbs::sequence<point_y> out(n);
    range_t qr(point_y(y1, x1), point_y(y2, x2), out.begin());
    range_tree.range_sum(point_x(x1, y1), point_x(x2, y2), qr);
    return out;
  }

  void insert_point(point_type p) {
    range_tree.insert(make_pair(make_pair(p.x, p.y), p.w));
  }
  
  void insert_point_lazy(point_type p) {
    range_tree = outer_map::insert_lazy(std::move(range_tree),
					make_pair(make_pair(p.x, p.y), p.w));
  }
  
  void print_status() {
    cout << "Tree size: " << range_tree.size() << endl; 
    cout << "Outer Map: ";  outer_map::GC::print_stats();
    cout << "Inner Map: ";  inner_map::GC::print_stats();
	cout << "Outer node size: " << sizeof(typename outer_map::node) << endl;
	cout << "Inner node size: " << sizeof(typename inner_map::node) << endl;
  }
  
  bool find(point_type p) {
    return range_tree.find(make_pair(p.x, p.y));
  }

  outer_map range_tree;
};
