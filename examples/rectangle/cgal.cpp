#include <CGAL/Cartesian.h>
#include <CGAL/Segment_tree_k.h>
#include <CGAL/Range_segment_tree_traits.h>
typedef CGAL::Cartesian<int> K;
typedef CGAL::Range_segment_tree_set_traits_2<K> Traits;
typedef CGAL::Segment_tree_2<Traits > Segment_tree_2_type;

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
    //#pragma omp parallel for
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

int main()
{
  typedef Traits::Interval Interval;
  typedef Traits::Key Key;
  std::vector<Interval> InputList, OutputList;
  
  InputList.push_back(Interval(Key(1,5), Key(2,7)));
  InputList.push_back(Interval(Key(2,1), Key(5,9)));
  InputList.push_back(Interval(Key(6,5), Key(9,8)));
  InputList.push_back(Interval(Key(1,4), Key(3,8)));
  
  Segment_tree_2_type Segment_tree_2(InputList.begin(),InputList.end());
  
  
  Interval a(Key(2,3), Key(3,4));
  //Key a(3,4);
  Segment_tree_2.enclosing_query(a,std::back_inserter(OutputList));
  std::vector<Interval>::iterator j = OutputList.begin();
  //std::cout << "\n window_query (3,6,5),(7,12,8) \n";
  while(j!=OutputList.end()){
    std::cout << (*j).first.x() << "," << (*j).first.y() << ",";
    j++;
  }
}
