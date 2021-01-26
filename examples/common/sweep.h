#include "parlay/primitives.h"

template<class Tp, class T, class Ti, class F, class G, class H>
parlay::sequence<Ti> sweep(parlay::sequence<T> &A, // elements to sweep over
			   Ti Init, // initial value 
			   F f, // a function for inserting f(Ti,T) -> Ti
			   G g, // a function for building g(T*,T*) -> Ti
			   H h, // a function for combining h(Ti,Ti) -> Ti
			   size_t num_blocks=0) {

  size_t n = A.size();
  if (num_blocks == 0) {
    const size_t factor = 1;
    const size_t threads = parlay::num_workers();
    num_blocks = threads*factor;
  }
  size_t block_size = ((n-1)/num_blocks);
  auto R = parlay::sequence<Ti>(n+1);

  {
    // generate partial sums for each block
    auto Sums = parlay::tabulate(num_blocks-1, [&] (size_t i) -> Tp {
	size_t l = i * block_size;
	size_t r = l + block_size;
	return g(&A[l], &A[r]);
      });

    // Compute the prefix sums across blocks
    R[0] = Init;
    for (size_t i = 1; i < num_blocks; ++i) 
      R[i*block_size] = h(R[(i-1)*block_size], std::move(Sums[i-1]));
  } // free Sums
  
  // Fill in final results within each block
  parlay::parallel_for (0, num_blocks, [&] (size_t i) {
    size_t l = i * block_size;
    size_t r = (i == num_blocks - 1) ? (n+1) : l + block_size;
    for (size_t j = l+1; j < r; ++j) 
      R[j] = f(R[j-1], A[j-1]);
    }, 1, true);
  return R;
}
