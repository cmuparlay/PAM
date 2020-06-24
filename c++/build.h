#pragma once
#include "pbbslib/sequence_ops.h"
#include "pbbslib/sample_sort.h"

template <class Entry>
struct build {
  using K = typename Entry::key_t;
  using V = typename Entry::val_t;
  using ET = typename Entry::entry_t;

  template <class Seq>
  static pbbs::sequence<ET>
  sort_remove_duplicates(Seq const &A,
			 bool seq_inplace = false, bool inplace = false) {
    auto less = [&] (ET a, ET b) {
      return Entry::comp(Entry::get_key(a), Entry::get_key(b));};
    if (A.size() == 0) return pbbs::sequence<ET>(0);
    if (seq_inplace) {
      pbbs::quicksort(A.begin(), A.size(), less);

      // remove duplicates
      size_t  j = 1;
      for (size_t i=1; i<A.size(); i++)
	if (less(A[j-1], A[i])) A[j++] = A[i];
      return A.slice(0,j);
    } else {
    
      if (!inplace) {
	
	auto B = pbbs::sample_sort(A, less);
	
	auto Fl = pbbs::dseq(B.size(), [&] (size_t i) {
	    return (i==0) || less(B[i-1], B[i]); });
	auto C = pbbs::pack(B, Fl);
	
	return C;
      } else {
	pbbs::sample_sort_inplace(A.slice(), less);
	auto g = [&] (size_t i) {return (i==0) || less(A[i-1], A[i]); };
	auto C = pbbs::pack(A, pbbs::delayed_seq<bool>(A.size(),g));
	return C;
      }
    }
  }
  
  pbbs::sequence<ET> 
  static sort_remove_duplicates(ET* A, size_t n) {
    auto lessE = [&] (ET a, ET b) {
      return Entry::comp(a.first, b.first);};

    pbbs::sample_sort(A, n, lessE);
	
    auto f = [&] (size_t i) {return A[i];};
    auto g = [&] (size_t i) {return (i==0) || lessE(A[i-1], A[i]); };
    auto B = pbbs::pack(dseq(n,f), dseq(n,g));
	  
    return B;
  }
  
  template<class Seq, class Reduce>
  static pbbs::sequence<ET>
  sort_reduce_duplicates(Seq const &A, const Reduce& reduce) {
    using E = typename Seq::value_type;
    using Vi = typename E::second_type;
    
    timer t("sort_reduce_duplicates", false);
    size_t n = A.size();
    if (n == 0) return pbbs::sequence<ET>(0);
    auto lessE = [] (E const &a, E const &b) {
      return Entry::comp(a.first, b.first);};

    auto B = pbbs::sample_sort(A, lessE);
    t.next("sort");
    
    // determines the index of start of each block of equal keys
    // and copies values into vals
    pbbs::sequence<bool> Fl(n);
    pbbs::sequence<Vi> Vals(n, [&] (size_t i) {
	Fl[i] = (i==0) || lessE(B[i-1], B[i]);
	return B[i].second;
      });
    t.next("copy, set flags");

    auto I = pbbs::pack_index<node_size_t>(Fl);
    t.next("pack index");
    
    // combines over each block of equal keys using function reduce
    pbbs::sequence<ET> a(I.size(), [&] (size_t i) {
	size_t start = I[i];
	size_t end = (i==I.size()-1) ? n : I[i+1];
	return ET(B[start].first, reduce(Vals.slice(start,end)));
      });
    t.next("reductions");
    // tabulate set over all entries of i
    return a;
  }

    template<class Seq, class Reduce>
  static pbbs::sequence<ET>
  sort_reduce_duplicates_a(Seq const &A, const Reduce& reduce) {
    using E = typename Seq::value_type;
    using Vi = typename E::second_type;

    timer t("sort_reduce_duplicates", false);
    size_t n = A.size();
    if (n == 0) return pbbs::sequence<ET>(0);
    auto lessE = [] (E const &a, E const &b) {
      return Entry::comp(a.first, b.first);};

    auto B = pbbs::sample_sort(A, lessE);
    t.next("sort");
    
    // determines the index of start of each block of equal keys
    // and copies values into vals
    pbbs::sequence<bool> Fl(n);
    pbbs::sequence<Vi> Vals(n, [&] (size_t i) {
	Fl[i] = (i==0) || lessE(B[i-1], B[i]);
	return B[i].second;
      });
    t.next("copy, set flags");

    auto I = pbbs::pack_index<node_size_t>(Fl);
    t.next("pack index");
    
    // combines over each block of equal keys using function reduce
    pbbs::sequence<ET> a(I.size(), [&] (size_t i) {
	size_t start = I[i];
	size_t end = (i==I.size()-1) ? n : I[i+1];
	return ET(B[start].first, reduce(Vals.slice(start,end)));
      });
    t.next("reductions");
    // tabulate set over all entries of i
    return a;
  }

  template<class Seq, class Bin_Op>
  static pbbs::sequence<ET>
  sort_combine_duplicates(Seq const &A, Bin_Op& f) {
    auto mon = pbbs::make_monoid(f,V());
    auto reduce_op = [&] (pbbs::sequence<V> S) {
      return pbbs::reduce(S, mon);};
    return sort_reduce_duplicates_a(A, reduce_op);
  }      

  template<class Seq, class Bin_Op>
  static pbbs::range<ET*>
  sort_combine_duplicates_inplace(Seq const &A,  Bin_Op& f) {
    auto less = [&] (ET a, ET b) {return Entry::comp(a.first, b.first);};
    pbbs::quicksort(A.begin(), A.size(), less);
    size_t j = 0;
    for (size_t i=1; i < A.size(); i++) {
      if (less(A[j], A[i])) A[++j] = A[i];
      else A[j].second = f(A[j].second,A[i].second);
    }
    return A.slice(0,j+1);
  }

};
