#pragma once
#include "utilities.h"
#include "merge.h"
#include "quicksort.h" // needed for insertion_sort

namespace pbbs {

  // if inplace is true then the output is placed in In and Out is just used
  // as temp space.
  template <class SeqA, class SeqB, class F> 
  void merge_sort(SeqA In, SeqB Out, const F& f, bool inplace=0) {
    size_t n = In.size();
    if (n < 24) {
      pbbs::insertion_sort(In.as_array(), n, f);
      if (!inplace)
	for (size_t i=0; i < n; i++) Out[i] = In[i];
      return;
    }
    size_t m = n/2;
    par_do_if(n > 64,
	   [&] () {merge_sort(In.slice(0,m), Out.slice(0,m), f, !inplace);},
	   [&] () {merge_sort(In.slice(m,n), Out.slice(m,n), f, !inplace);},
	   true);
    if (inplace)
      pbbs::merge(Out.slice(0,m), Out.slice(m,n), In.slice(0,n), f, true);
    else
      pbbs::merge(In.slice(0,m), In.slice(m,n), Out.slice(0,n), f, true);
  }
}
