using data_type = int;

typedef pair<data_type, data_type> point_type;
typedef pair<point_type, point_type> seg_type;
struct query_type {
	int x, y1, y2;
};

using parlay::sequence;
using parlay::parallel_for;

template<typename T>
void shuffle(vector<T> v) {
    srand(unsigned(time(NULL)));
	int n = v.size();
    for (size_t i = 0; i < n; ++i) {
        size_t j = rand() % (n-i);
        swap(v[i], v[i+j]);
    }
}

inline data_type x1(seg_type& s) {
	return s.first.first;
}

inline data_type y1(seg_type& s) {
	return s.first.second;
}

inline data_type x2(seg_type& s) {
	return s.second.first;
}

inline data_type y2(seg_type& s) {
	return s.second.second;
}

double value_at(seg_type& s, int x) {
	if (x2(s) == x1(s)) return double(y1(s));
	double r1 = (double)((x-x1(s))+0.0)/(x2(s)-x1(s)+0.0)*(y1(s)+0.0);
	double r2 = ((x2(s)-x)+0.0)/(x2(s)-x1(s)+0.0)*(y2(s)+0.0);
	return r1+r2;
}

bool in_seg_x_range(seg_type& s, int p) {
	if ((x1(s) <= p) && (p <= x2(s))) return true;
	return false;
}

bool cross(seg_type& s, query_type q) {
	double cp;
	if (in_seg_x_range(s, q.x)) cp = value_at(s, q.x); else return false;
	if (q.y1 <= cp && cp <= q.y2) return true; else return false;
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

parlay::sequence<seg_type> generate_segs_small(size_t n, int win_x) {
    parlay::sequence<int> p1(2*n+1);
	parlay::sequence<int> p2(2*n+1);
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
	parlay::sequence<seg_type> ret(n);
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

parlay::sequence<seg_type> generate_segs(size_t n) {
    parlay::sequence<int> p1(2*n+1);
	parlay::sequence<int> p2(2*n+1);
	p1[0] = 0; p2[0] = 0;
    for (int i = 1; i < 2*n+1; ++i) {
		p1[i] = p1[i-1] + random_hash('x', i, 1, 10);
		if (p1[i]%2) p1[i]++;
		p2[i] = p2[i-1] + random_hash('y', i, 1, 10);
		if (p2[i]%2) p2[i]++;
    }
	max_x = p1[2*n]; max_y = p2[2*n];
	cout << max_x << " " << max_y << endl;

	for (size_t i = 0; i < 2*n+1; ++i) {
		size_t j = random_hash('s', i, 0, 2*n+1-i);
		swap(p1[i], p1[i+j]);
	}
	parlay::sequence<seg_type> ret(n);
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

parlay::sequence<query_type> generate_queries(size_t q) {
    parlay::sequence<query_type> ret(q);
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