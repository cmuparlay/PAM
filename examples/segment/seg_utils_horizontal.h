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

using namespace std;

using coord_t = int;

typedef pair<coord_t, coord_t> seg_type;
typedef pair<coord_t, seg_type> seg_type_2d;
struct query_type {
  coord_t x, y1, y2;
};

int infty = 1000000000;

parlay::sequence<seg_type_2d> generate_segs(size_t n, int a, int b) {
  parlay::sequence<seg_type_2d> ret(n);
  parlay::parallel_for (0, n, [&] (size_t i) {
      ret[i].first = random_hash('y', i, a, b);
      int xl = random_hash('l', i, a, b);
      int xr = random_hash('r', i, a, b);
      if (xl > xr) {
	int t = xl; xl = xr; xr = t;
      }
      ret[i].second.first = xl; ret[i].second.second = xr;
    });

  return ret;
}

parlay::sequence<query_type> generate_queries(size_t q, int a, int b) {
  parlay::sequence<query_type> ret(q);
  parlay::parallel_for (0, q, [&] (size_t i) {
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