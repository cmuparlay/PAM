#include "augmented_map.h"
#include "pbbs-include/get_time.h"

timer sweep_1_tm, sweep_2_tm, sweep_3_tm;

template<class Tp, class T, class Ti, class F, class G, class H>
Ti* sweep(T* A, size_t n, Ti Init, F f, G g, H h, size_t num_blocks=0) {

  if (num_blocks == 0) {
    const size_t factor = 1;
    const size_t threads = __cilkrts_get_nworkers();
    num_blocks = threads*factor;
  }
  size_t block_size = ((n-1)/num_blocks);
  Tp* Sums = pbbs::new_array<Tp>(num_blocks);
  Ti* R = pbbs::new_array<Ti>(n+1);

  //cout << "1" << endl;
  // generate partial sums for each block
  parallel_for (size_t i = 0; i < num_blocks-1; ++i) {
    size_t l = i * block_size;
    size_t r = l + block_size;
    Sums[i] = g(A + l, A + r);
  }
  
  //cout << "2" << endl;

  // Compute the prefix sums across blocks
  R[0] = Init;
  for (size_t i = 1; i < num_blocks; ++i) 
    R[i*block_size] = h(R[(i-1)*block_size], std::move(Sums[i-1]));
  pbbs::delete_array(Sums, num_blocks);

  //cout << "3" << endl;
  
  // Fill in final results within each block
  parallel_for (size_t i = 0; i < num_blocks; ++i) {
    size_t l = i * block_size;
    size_t r = (i == num_blocks - 1) ? (n+1) : l + block_size;
    for (size_t j = l+1; j < r; ++j) 
      R[j] = f(R[j-1], A[j-1]);
  }
  return R;
}
