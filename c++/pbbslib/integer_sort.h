// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2010-2017 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once
#include <math.h>
#include <stdio.h>
#include <cstdint>
#include "utilities.h"
#include "counting_sort.h"
#include "quicksort.h"

namespace pbbs {

  constexpr size_t radix = 8;
  constexpr size_t max_buckets = 1 << radix;

  template <class T, class F>
  void radix_step(T* A, T* B, size_t* counts, size_t n, size_t m, F extract) {
    for (size_t i = 0; i < m; i++)  counts[i] = 0;
    for (size_t j = 0; j < n; j++) {
      size_t k = extract(A[j]);
      counts[k]++;
    }

    size_t s = 0;
    for (size_t i = 0; i < m; i++) {
      s += counts[i];
      counts[i] = s;
    }
    
    for (long j = n-1; j >= 0; j--) {
      long x = --counts[extract(A[j])];
      B[x] = A[j];
    }
  }

  template <class T, class GetKey>
  void seq_radix_sort(sequence<T> In, sequence<T> Out, GetKey g,
		      size_t bits, bool inplace=true, size_t bit_offset=0) {
    size_t counts[max_buckets+1];
    size_t n = In.size();
    T* InA = In.as_array();
    T* OutA = Out.as_array();
    bool swapped = false;
    while (bits > 0) {
      size_t round_bits = std::min(radix, bits);
      size_t num_buckets = (1 << round_bits);
      size_t mask = num_buckets-1;
      auto get_key = [&] (T k) -> size_t {
	return (g(k) >> bit_offset) & mask;};
      radix_step(InA, OutA, counts, n, num_buckets, get_key);
      std::swap(InA,OutA);
      bits = bits - round_bits;
      bit_offset = bit_offset + round_bits;
      swapped = !swapped;
    }
    if ((inplace && swapped) || (!inplace && !swapped)) {
      for (size_t i=0; i < n; i++) 
	move_uninitialized(In[i], Out[i]);
    }
  }
  
  // a top down recursive radix sort
  // g extracts the integer keys from In
  // key_bits specifies how many bits there are left
  // num_sets is an indication of parallalelism to be used 
  template <typename T, typename Get_Key>
  void integer_sort_r(sequence<T> In,  sequence<T> Out, Get_Key& g, 
		      size_t key_bits, size_t num_sets,
		      bool inplace) {
    size_t n = In.size();
    timer t;

    // ran out of bucket sets, use sequential radix sort
    if (num_sets <= 1) {
      seq_radix_sort<T>(In, Out, g, key_bits, inplace);

    // few bits, just do a single parallel count sort
    } else if (key_bits <= radix) {
      //t.start();
      size_t num_buckets = (1 << key_bits);
      size_t mask = num_buckets - 1;
      auto f = [&] (size_t i) {return g(In[i]) & mask;};
      auto get_bits = make_sequence<size_t>(n, f); 
      count_sort(In, Out, get_bits, num_buckets, inplace, num_sets);
      //t.next("sort");

    // recursive case  
    } else {
      size_t bits = 8;
      size_t shift_bits = key_bits - bits;
      size_t buckets = (1 << bits);
      size_t mask = buckets - 1;
      auto f = [&] (size_t i) {return (g(In[i]) >> shift_bits) & mask;};
      auto get_bits = make_sequence<size_t>(n, f);
      sequence<size_t> bucket_offsets =
	count_sort(In, Out, get_bits, buckets, false, num_sets);
      auto recf = [&] (size_t i) {
	size_t start = bucket_offsets[i];
	size_t end = bucket_offsets[i+1];
	size_t nsets = floor((((double) num_sets) * (end-start)) / n);
	integer_sort_r(Out.slice(start, end), In.slice(start, end),
		       g, shift_bits, nsets, !inplace);
      };
      parallel_for(0, buckets, recf, 1);
    }
  }

  // a top down recursive radix sort
  // g extracts the integer keys from In
  // result will be placed in out, 
  //    but if inplace is true, then result will be put back into In
  // val_bits specifies how many bits there are in the key
  //    if set to 0, then a max is taken over the keys to determine
  template <typename T, typename Get_Key>
  void integer_sort_(sequence<T> In, sequence<T> Out, Get_Key& g, 
		    size_t key_bits=0, bool inplace=false) {
    if (In.start() == Out.start()) {
      cout << "in integer_sort : input and output must be different locations" << endl;
      abort();}
    if (key_bits == 0) {
      auto max = [&] (size_t a, size_t b) {return std::max(a,b);};
      auto get_key = [&] (size_t i) {return g(In[i]);};
      auto keys = make_sequence<size_t>(In.size(), get_key);
      size_t max_val = reduce(keys, max);
      key_bits = log2_up(max_val);
    }
    size_t num_sets = In.size() / (max_buckets * 16);
    integer_sort_r(In, Out, g, key_bits, num_sets, inplace);
  }

  template <typename T, typename Get_Key>
  void integer_sort(sequence<T> In, Get_Key& g, size_t key_bits=0) {
    sequence<T> Tmp = sequence<T>::alloc_no_init(In.size());
    integer_sort_(In, Tmp, g, key_bits, true);
  }
}
