#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <cctype>
#include <fstream>
#include <set>
#include "pbbslib/get_time.h"
#include "pbbslib/strings/string_basics.h"
#include "pbbslib/parse_command_line.h"
#include "index.h"

using namespace std;
using namespace pbbs;

long str_to_long(char* str) {
    return strtol(str, NULL, 10);
}

using post_elt = inv_index::post_elt;
using post_list = inv_index::post_list;
using index_elt = inv_index::index_elt;
using index_pair = pair<token,post_list>;

template <class Seq>
sequence<index_elt> parse(Seq const &Str, bool verbose, timer t) {
  size_t n = Str.size();

  // find start of each document based on header
  string header = "<doc id=";
  //string header = "ab";
  //string header = "\n";
  size_t header_size = header.size();
  pbbs::sequence<bool> doc_starts(n, [&] (size_t i) {
      if (i > n - header_size) return false;
      for (size_t j=0; j < header.size(); j++)
	if (header[j] != Str[i+j]) return false;
      return true;
    });
  t.next("get headers");

  // only consider alphabetic characters, and turn to lowercase
  pbbs::sequence<char> cleanstr = pbbs::map(Str, [&] (char a) -> char {
      return isalpha(a) ? tolower(a) : ' ';});
  t.next("clean");

  // partition cleaned string based on document starts
  auto docs = pbbs::partition_at(cleanstr, doc_starts);
  t.next("partition");
    
  // tokenize each document and tag with document id and weight
  auto pairs = pbbs::tabulate(docs.size(), [&] (size_t i) {
      auto is_space = [] (char a) {return a == ' ' || a == '\t';};
      size_t m = docs[i].size();
      // remove header from each doc
      auto doc = docs[i].slice(std::min(header_size, m), m);
      
      // get tokens from the remaining doc, and tag each with doc id and weight
      return pbbs::dmap(pbbs::tokens(doc, is_space), [=] (pbbs::sequence<char> w) {
	  return index_elt(w, post_elt(i,1.0));});
    });
  t.next("tokens");

  auto r = flatten(pairs);
  //cout << to_char_seq(r) << endl;
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
  size_t threads = num_workers();
  timer t;
  timer tdetail("build index detail", verbose);
  
  // read
  auto Str_ = char_range_from_file(fname);
  tdetail.next("mmap file");
  auto Str = Str_.slice(0,std::min(max_chars,Str_.size()));
  tdetail.next("copy file");

  pbbs::allocator_reserve(Str.size()*9);

  // parse input 
  pbbs::sequence<index_elt> KV = parse(Str, verbose, tdetail);
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

  sequence<pair<token,token>> test_word_pairs(num_queries);
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

  //FILE* y = freopen("queries.txt", "w", stdout);
  //ofstream y; y.open("queries.txt");
  pbbs::random r(0);
  for(size_t i =0; i < num_queries; i++) {
    test_word_pairs[i].first = KV[r.ith_rand(i) % (n - 1)].first;
    test_word_pairs[i].second =
      (*common_words.select(r.ith_rand(num_queries+i)%num_common_words)).first;
  }
  //y.close();
  
  // run the queries
  t.start();
  parallel_for(0, num_queries, [&] (size_t i) {
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

  //delete[] KV;

  // if (write_output) {
  //   vector<pair<char*,post_list> >out;

  //   FILE* x = freopen("sol.out", "w", stdout);
  //   if (x == NULL) abort();

  //   for (size_t i = 0; i < out.size(); i++) { 
  //     std::cout << (out[i].first) << " " << (out[i].second).size() << std::endl;
  //   }
  // 	fclose(x);
  // }

  // free the original character array
  //free(X.second);
  return 0;
}
