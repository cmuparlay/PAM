int max_x, max_y;


using data_type = int;

typedef pair<data_type, data_type> point_type;
typedef pair<point_type, point_type> rec_type;
struct query_type {
	int x, y;
};

inline int x1(rec_type& s) {return s.first.first;}
inline int y1(rec_type& s) {return s.first.second;}
inline int x2(rec_type& s) {return s.second.first;}
inline int y2(rec_type& s) {return s.second.second;}

void print_out(rec_type e) {
	cout << "[" << x1(e) << ", " << y1(e) << "; " << x2(e) << ", " << y2(e) << "]";
}

template<typename T>
void output_content(T t) {
	using ent = typename T::E;
	parlay::sequence<ent> out(t.size());
	cout << t.size() << ": ";
	T::entries(t, out.begin());
	for (int i = 0; i < out.size(); i++) {
		print_out(out[i]);
		cout << " ";
	}
	cout << endl;
}


inline bool inside(data_type a, data_type b, data_type q) {
	return ((a<=q) && (q<=b));
}

inline bool in_rec(rec_type r, query_type p) {
	return ((inside(x1(r), x2(r), p.x)) && (inside(y1(r), y2(r), p.y)));
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

parlay::sequence<rec_type> generate_recs(size_t n, int a, int b) {
  parlay::sequence<int> p1(2*n+1);
  parlay::sequence<int> p2(2*n+1);
  p1[0] = 0; p2[0] = 0;
  for (int i = 1; i < 2*n+1; ++i) {
    p1[i] = p1[i-1] + random_hash('x', i, 1, 30);
    p2[i] = p2[i-1] + random_hash('y', i, 1, 30);
  }
  max_x = p1[2*n]; max_y = p2[2*n];
//  cout << max_x << " " << max_y << endl;

  //srand(unsigned(time(NULL)));
  for (size_t i = 0; i < 2*n+1; ++i) {
    size_t j = random_hash('s', i, 0, 2*n+1-i);
    swap(p1[i], p1[i+j]);
    j = random_hash('s'*'s'+920516, i, 0, 2*n+1-i);
    swap(p2[i], p2[i+j]);
  }
  parlay::sequence<rec_type> ret(n);
  parlay::parallel_for (0, n, [&] (size_t i) {
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
  auto x = parlay::sequence<pp>::uninitialized(n*2);
  auto y = parlay::sequence<pp>::uninitialized(n*2);
  bool dup = true;
  while (dup) {
    parlay::parallel_for (0, n, [&] (size_t i) {
      x[i*2] = make_pair(x1(ret[i]), i);
      x[i*2+1] = make_pair(x2(ret[i]), -i);
      y[i*2] = make_pair(y1(ret[i]), i);
      y[i*2+1] = make_pair(y2(ret[i]), -i);
    });
    auto less = [&](pp a, pp b) {return a.first<b.first;};
    x = parlay::sort(x, less);
    y = parlay::sort(y, less);
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

parlay::sequence<query_type> generate_queries(size_t q, int a, int b) {
  parlay::sequence<query_type> ret(q);
  parlay::parallel_for (0, q, [&] (size_t i) {
    ret[i].x = random_hash('q'*'x', i, 0, max_x);
    if (ret[i].x%2 == 0) ret[i].x++;
    ret[i].y = random_hash('y'+1, i, 0, max_y);
    if (ret[i].y%2 == 0) ret[i].y++;
  });
  return ret;
}