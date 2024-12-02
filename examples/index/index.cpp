#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <cctype>
#include <fstream>
#include <set>

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>

#include <pam/get_time.h>
#include <pam/parse_command_line.h>

#include "index.h"

using namespace std;

long str_to_long(char* str) {
    return strtol(str, nullptr, 10);
}

using post_elt = inv_index::post_elt;
using post_list = inv_index::post_list;
using index_elt = inv_index::index_elt;
using index_pair = pair<token,post_list>;

template <class Seq>
parlay::sequence<index_elt> parse(Seq const &Str, bool verbose, timer t) {
  size_t n = Str.size();

  // find start of each document based on header
  string header = "<doc id=";
  size_t header_size = header.size();
  auto doc_ends = parlay::tabulate(n, [&] (size_t i) -> bool {
      if (i > n - header_size - 1) return false;
      for (size_t j=0; j < header_size; j++)
	if (header[j] != Str[i+j+1]) return false;
      return true;
    });
  t.next("get headers");

  // only consider alphabetic characters, and turn to lowercase
  parlay::sequence<char> cleanstr = parlay::map(Str, [&] (char a) -> char {
      return isalpha(a) ? tolower(a) : ' ';});
  t.next("clean");

  // partition cleaned string based on document ends
  auto docs = parlay::split_at(cleanstr, doc_ends);
  t.next("partition");
    
  // tokenize each document and tag with document id and weight
  auto pairs = parlay::tabulate(docs.size(), [&] (size_t i) {
      //auto is_space = [] (char a) {return a == ' ' || a == '\t';};
      auto is_space = [] (char a) {return isspace(a);};
      size_t m = docs[i].size();
      // remove header from each doc
      auto doc = docs[i].cut(std::min(header_size, m), m);
      
      // get tokens from the remaining doc, and tag each with doc id and weight
      // not sure if map or dmap is faster
      return parlay::dmap(parlay::tokens(doc, is_space), [=] (parlay::sequence<char> w) {
	  return index_elt(w, post_elt(i,1.0));});
    });
  t.next("tokens");

  auto r = parlay::flatten(pairs);
  t.next("flatten");

  if (verbose) {
    cout << "Words = " << r.size() << endl;
    cout << "Documents = " << docs.size() << endl;
  }

  return r;
}

int main(int argc, char** argv) {
  string default_filename = "/ssd0/text/wikipedia.txt";
  commandLine P(argc, argv,
		"./index [-o] [-v] [-r rounds] [-n max_chars] [-q num_queries] [-f file]");
  size_t max_chars = P.getOptionLongValue("-n", 1000000000000);
  size_t num_queries = P.getOptionLongValue("-q", 10000);
  string fname = string(P.getOptionValue("-f", default_filename));
  //bool write_output = P.getOption("-o");
  bool verbose = P.getOption("-v");
  int rounds = P.getOptionIntValue("-r", 1);
  size_t threads = parlay::num_workers();
  timer t;
  timer tdetail("build index detail", verbose);
  
  // read
  auto Str_ = parlay::file_map(fname);
  auto Str__ = parlay::make_slice(Str_);
  tdetail.next("mmap file");
  auto Str = Str__.cut(0,std::min(max_chars,Str__.size()));
  tdetail.next("copy file");

  // parse input 
  parlay::sequence<index_elt> KV = parse(Str, verbose, tdetail);
  size_t n = KV.size();
  inv_index test_idx;

  for (int i=0; i < rounds; i++) {
    test_idx = inv_index();
    // Build the index
    t.start();
    test_idx = inv_index(KV);
    double t_build = t.stop();
    cout << "index build"
	 << ", threads = " << threads
	 << ", rounds = 1" 
	 << ", n = " << n
	 << ", q = " << num_queries
	 << ", time = " << t_build
	 << endl;
    if (verbose)
      cout << "unique words = " << test_idx.idx.size() << endl;
  }
  
  using idx = typename inv_index::index;

  parlay::sequence<pair<token,token>> test_word_pairs(num_queries);
  size_t* size_in = new size_t[num_queries];
  size_t* size_out = new size_t[num_queries];

  // Setup the queries
  // filters out any words which appear fewer than 100 times
  idx common_words = idx::filter(test_idx.idx, [] (index_pair e) {
      return (e.second.size() > 100); });
  if (verbose)
    cout << "filter size = " << common_words.size() << endl;

  //size_t num_words = test_idx.idx.size();
  size_t num_common_words = common_words.size();
  if (num_common_words == 0) {
    cout << "data too small for running query" << endl;
    return 0;
  }

  parlay::random r(0);
  for(size_t i =0; i < num_queries; i++) {
    test_word_pairs[i].first = KV[r.ith_rand(i) % (n - 1)].first;
    test_word_pairs[i].second =
      (*common_words.select(r.ith_rand(num_queries+i)%num_common_words)).first;
  }
  
  // run the queries
  t.start();
  parlay::parallel_for(0, num_queries, [&] (size_t i) {
    post_list l1 = test_idx.get_list(test_word_pairs[i].first);
    post_list l2 = test_idx.get_list(test_word_pairs[i].second);
    size_in[i] = l1.size() + l2.size();
    post_list l3 = test_idx.And(l1,l2);
    vector<post_elt> r = test_idx.top_k(l3,10);
    size_out[i] = l3.size() + r.size();
    });
  double t_query = t.stop();
  cout << "index query"
       << ", threads = " << threads
       << ", rounds = 1" 
       << ", n = " << n
       << ", q = " << num_queries
       << ", time = " << t_query
       << endl;  
  size_t total_in = 0;
  size_t total_out = 0;

  for (size_t i =0; i < num_queries; i++) {
    if (i < 0) 
      cout << test_word_pairs[i].first << ", " << test_word_pairs[i].second << endl;
    total_in += size_in[i];
    total_out += size_out[i];
  }
  
  if (verbose) {

    cout << "total in = " << total_in << endl;
    cout << "total out = " << total_out << endl;

    cout << "Unique words = " << test_idx.idx.size() << endl;
    cout << "Map: ";  inv_index::index::GC::print_stats();
    cout << "Set: ";  post_list::GC::print_stats();
  }

  return 0;
}
