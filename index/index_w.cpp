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
#include "index_w.h"

double global;
int exe_queries = 0, exe_updates = 0;
bool cut_time = false;
size_t half_docs, half_words;

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

const double TIME_INTERVAL = 30.0;
size_t num_threads = 100;

//using parse_result = pair<pair<size_t*, size_t>, pair<char**, size_t>>;
using parse_result = size_t*;

void update(inv_index& test_idx, index_elt* KV, size_t* doc_start, size_t total_docs) {
	timer tm; tm.start();
	size_t interval = 100;
	size_t start = half_docs;
	size_t update_docs = total_docs-start+1;
	size_t rounds = update_docs / interval;
	test_idx.reserve_vector(rounds+1);
	size_t i;
	for (i = start; i < total_docs; i += interval) {
		size_t t = (i+interval);
		if (t>=total_docs) t = total_docs-1;
		size_t s = doc_start[i];
		size_t e = doc_start[t];
		if (s==e) {
			exe_updates = i; break;
		}
		test_idx.update(KV+s,KV+e);
		if (cut_time && ((tm.get_time()-global) > TIME_INTERVAL)) {
			break;
		}
	}		
	exe_updates = i;
	tm.stop(); cout << "Update finish in time " << tm.get_total() << endl;
}

void query(size_t num_queries, inv_index& test_idx, pair<char*,char*>* test_words, 
		size_t* size_in, size_t* size_out) {
  size_t each = num_queries/num_threads;
  timer q_t;
  q_t.start();
  size_t finished[1000000];
  parallel_for(0, num_threads, [&] (size_t i) {
	  size_t offset = i*each;
	  timer t; finished[i] = each;
	  for (size_t j = 0; j < each; j++) {
		size_t cur = offset+j;
		inv_index::index idx = test_idx.idx;
		post_list l1 = test_idx.get_list(idx, test_words[cur].first);
		post_list l2 = test_idx.get_list(idx, test_words[cur].second);
		size_in[cur] = l1.size() + l2.size();
		post_list l3 = test_idx.And(l1,l2);
		vector<post_elt> r = test_idx.top_k(l3,10);
		size_out[cur] = l3.size() + r.size();
		if (cut_time && (t.get_time()-global > TIME_INTERVAL)) {
			finished[i] = j; break;
		}
	  }
    }, 1);
  exe_queries = 0;
  for (int i = 0; i < num_threads; i++) exe_queries+=finished[i];
  double t_query = q_t.stop();
  cout << exe_queries << " queries in time " << t_query << endl;
}

void query_(size_t num_queries, inv_index& test_idx, pair<char*,char*>* test_words, 
		size_t* size_in, size_t* size_out) {
  size_t each = num_queries/num_threads;
  timer q_t;
  q_t.start();
  parallel_for(0, num_queries, [&] (size_t i) {
	size_t cur = i;
	inv_index::index idx = test_idx.idx;
	post_list l1 = test_idx.get_list(idx, test_words[cur].first);
	post_list l2 = test_idx.get_list(idx, test_words[cur].second);
	size_in[cur] = l1.size() + l2.size();
	post_list l3 = test_idx.And(l1,l2);
	vector<post_elt> r = test_idx.top_k(l3,10);
	size_out[cur] = l3.size() + r.size();
    });

  double t_query = q_t.stop();
  cout << num_queries << " queries in time " << t_query << endl;
}
	

int main(int argc, char** argv) {
  string fname = "/usr3/data/wikipedia/wikipedia.txt";
  //string fname = "wiki_small.txt";

  commandLine P(argc, argv,
		"./index [-o] [-v] [-n max_chars] [-q num_queries] [-f file] [-t type(UQ)] [-c] [-d docs] [-h query_threads]");
  size_t max_chars = P.getOptionLongValue("-n", 1000000000000);
  size_t num_queries = P.getOptionLongValue("-q", 10000);
  size_t update_stops = P.getOptionLongValue("-d", 0);
  num_threads = P.getOptionLongValue("-h", 100);
  //string fname = string(P.getOptionValue("-f", fname.c_str()));
  bool write_output = P.getOption("-o");
  bool verbose = P.getOption("-v");
  cut_time = P.getOption("-c");
  size_t uq = P.getOptionLongValue("-t", 3);
  size_t threads = num_workers();

  // Read and parse input from file
  pbbs::random r(0);
  //parse_result X = parse(fname, max_chars, verbose);

  startTime();
  sequence<char> Str = read_string_from_file(fname, 0, max_chars);
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
  
  parallel_for(0, num_words - 1, [&] (size_t i) {
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
  
  size_t* doc_start = new size_t[total_docs];
  index_elt* KV = pbbs::new_array_no_init<index_elt>(total_pairs);
  for (size_t i = 0; i < total_docs; ++i) {	
    size_t start = I[i];
    size_t end = (i == (total_docs-1)) ? num_words : I[i+1];
    size_t len = end - start - header_size;
    size_t start_out = start - (header_size * i);
	doc_start[i] = start_out;
    for (size_t j = 0; j < len; j++) {
      KV[start_out+j] = index_elt(W[start+header_size+j], 
				  post_elt(i,1.0));
    }
  }
  if (verbose) nextTime("create points");

  pair<char*,char*>* test_words = new pair<char*,char*>[num_queries];
  size_t* size_in = new size_t[num_queries];
  size_t* size_out = new size_t[num_queries];

  // Build the index
  timer t;
  t.start();
  half_docs = total_docs/5*4+1;
  half_words = doc_start[half_docs];
  if (update_stops == 0) update_stops = total_docs; else update_stops += half_docs;
  inv_index test_idx(KV, KV + half_words, total_pairs);
  double t_build = t.stop();
  cout << "index build"
       << ", threads = " << threads
       << ", rounds = 1" 
       << ", n = " << total_pairs
       << ", q = " << num_queries
       << ", time = " << t_build
       << endl;

  // Setup the queries
  // filters out any words which appear fewer than 100 times
  //FILE* in = freopen("queries.txt", "r", stdin);
  ifstream y; y.open("queries.txt");
  for (size_t i =0; i < num_queries; i++) {
	test_words[i].first = new char[100];
	test_words[i].second = new char[100];
    y >> test_words[i].first >> test_words[i].second;
  }
  y.close();
  
  cout << "read queries" << endl;
  
  timer g;
  global = g.get_time();

  if (uq ==3) {
    parallel_for (0, 2, [&] (size_t i) {
		  if ((uq%2) && (i==0)) query(num_queries, test_idx, test_words, size_in, size_out);
		  if ((uq>>1) && (i==1)) update(test_idx, KV, doc_start, update_stops);
      });
  }
  if (uq ==1) query_(num_queries, test_idx, test_words, size_in, size_out);
  if (uq ==2) update(test_idx, KV, doc_start, update_stops);

  /*
  cilk_spawn update(test_idx, KV, doc_start, total_docs);
  query(num_queries, test_idx, test_words, size_in, size_out);
  cilk_sync;*/
      
	  
  cout << "updated docs: " << exe_updates-half_docs << endl;
  cout << "updated words: " << doc_start[exe_updates+1]-half_words << endl;
  
  size_t total_in = 0;
  size_t total_out = 0;

  for (size_t i =0; i < exe_queries; i++) {
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
  }

  return 0;
}
