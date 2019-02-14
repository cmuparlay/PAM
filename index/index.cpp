#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <cctype>
#include <fstream>
#include <set>
#include "pbbslib/get_time.h"
#include "pbbslib/par_string.h"
#include "pbbslib/parse_command_line.h"
#include "index.h"

using namespace std;
using namespace pbbs;

long str_to_long(char* str) {
    return strtol(str, NULL, 10);
}

using word = char*;
using post_elt = inv_index::post_elt;
using post_list = inv_index::post_list;
using index_elt = inv_index::index_elt;
using index_pair = pair<word,post_list>;

pair<pair<index_elt*,size_t>, char*> parse(string filename, size_t max_size,
					   bool verbose) {
  startTime();
  sequence<char> Str = read_string_from_file(filename, 0, max_size);
  size_t n = Str.size();
  if (verbose) nextTime("read file");

  parallel_for (0, n, [&] (size_t i) {
    Str[i] = tolower(Str[i]);
    if (!islower(Str[i])) Str[i] = 0;
    });

  sequence<char*> W = string_to_words(Str);
  size_t num_words = W.size();
  if (verbose) nextTime("find words");

  sequence<bool> start_flag(num_words);
  start_flag[num_words-1] = 0;
  char doc[] = "doc";
  char ids[] = "id";
  
  parallel_for(0, num_words-1, [&] (size_t i) {
    if ((strcmp(W[i],doc) == 0) && strcmp(W[i+1],ids) == 0)
      start_flag[i] = 1;
    else start_flag[i] = 0;
    });

  sequence<size_t> I = pack_index<size_t>(start_flag);
  size_t total_docs = I.size();
  if (verbose) {
    cout << "Words = " << num_words << endl;
    cout << "Documents = " << total_docs << endl;
    nextTime("find documents");
  }

  size_t header_size = 2;
  size_t total_pairs = num_words - total_docs * header_size;
  if (verbose) cout << "Pairs = " << total_pairs << endl;

  index_elt* KV = pbbs::new_array_no_init<index_elt>(total_pairs);
  parallel_for (0, total_docs, [&] (size_t i) {
    size_t start = I[i];
    size_t end = (i == (total_docs-1)) ? num_words : I[i+1];
    size_t len = end - start - header_size;
    size_t start_out = start - (header_size * i);
    for (size_t j = 0; j < len; j++) {
      KV[start_out+j] = index_elt(W[start+header_size+j], 
				  post_elt(i,1.0));
    }
    });
  if (verbose) nextTime("create pairs");

  return make_pair(make_pair(KV,total_pairs),Str.as_array());
}

int main(int argc, char** argv) {
  string fname = "/usr3/data/wikipedia/wikipedia.txt";
  //string fname = "wiki_small.txt";

  commandLine P(argc, argv,
		"./index [-o] [-v] [-n max_chars] [-q num_queries] [-f file]");
  size_t max_chars = P.getOptionLongValue("-n", 1000000000000);
  size_t num_queries = P.getOptionLongValue("-q", 10000);
  //string fname = string(P.getOptionValue("-f", fname.c_str()));
  bool write_output = P.getOption("-o");
  bool verbose = P.getOption("-v");
  size_t threads = num_workers();

  // Read and parse input from file
  pbbs::random r(0);
  pair<pair<index_elt*,size_t>,char*> X = parse(fname, max_chars, verbose);
  index_elt* KV = X.first.first;
  size_t n = X.first.second;

  pair<char*,char*>* test_words = new pair<char*,char*>[num_queries];
  size_t* size_in = new size_t[num_queries];
  size_t* size_out = new size_t[num_queries];

  // Build the index
  timer t;
  t.start();
  inv_index test_idx(KV, KV +n);
  double t_build = t.stop();
  cout << "index build"
       << ", threads = " << threads
       << ", rounds = 1" 
       << ", n = " << n
       << ", q = " << num_queries
       << ", time = " << t_build
       << endl;
  
  using idx = typename inv_index::index;

  // Setup the queries
  // filters out any words which appear fewer than 100 times
  idx common_idx = idx::filter(test_idx.idx,
			       [] (index_pair e) {
				 return (e.second.size() > 100); });
  if (verbose) cout << "filter size = " << common_idx.size() << endl;

  //size_t num_words = test_idx.idx.size();
  size_t num_common_words = common_idx.size();
  if (num_common_words == 0) {
    cout << "data too small for running query" << endl;
    return 0;
  }

  //FILE* y = freopen("queries.txt", "w", stdout);
  ofstream y; y.open("queries.txt");
  for(size_t i =0; i < num_queries; i++) {
    test_words[i].first = KV[r.ith_rand(i)%(n-1)].first;
    test_words[i].second = (*common_idx.select(r.ith_rand(num_queries+i)%num_common_words)).first;
	y << (test_words[i].first) << " " << (test_words[i].second) << std::endl;
  }
  y.close();
  

  // run the queries
  t.start();
  parallel_for(0, num_queries, [&] (size_t i) {
    post_list l1 = test_idx.get_list(test_words[i].first);
    post_list l2 = test_idx.get_list(test_words[i].second);
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
      cout << test_words[i].first << ", " << test_words[i].second << endl;
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

  delete[] KV;

  if (write_output) {
    vector<pair<char*,post_list> >out;

    FILE* x = freopen("sol.out", "w", stdout);
    if (x == NULL) abort();

    for (size_t i = 0; i < out.size(); i++) { 
      std::cout << (out[i].first) << " " << (out[i].second).size() << std::endl;
    }
	fclose(x);
  }

  // free the original character array
  free(X.second);
  return 0;
}
