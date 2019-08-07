#include <cctype>
#include "pbbslib/get_time.h"
#include "pbbslib/strings/string_basics.h"

using namespace std;
using namespace pbbs;

auto is_line = [] (char c) -> bool {
  switch (c)  {
  case '\r': 
  case '\n': 
  case 0: return true;
  default : return false;
  }
};

struct csv_data {
  csv_data(sequence<char> a, sequence<char*> b, sequence<sequence<char*>> c) :
    str(move(a)), splits(move(b)), lines(move(c)) {};
  sequence<char> str;
  sequence<char*> splits;
  sequence<sequence<char*>> lines;
};

template <class F>
csv_data readCSV(string filename, F sep, bool verbose = false) {

  startTime();
  sequence<char> str = char_seq_from_file(filename, 0, 100000000000ul);
  size_t n = str.size();
  if (verbose) {
    string x = "read ";
    nextTime(x.append(filename));
  }
  
  sequence<bool> FL(n);
  FL[0] = 1;
  parallel_for (0, n-1, [&] (size_t i) {
    FL[i+1] = (is_line(str[i]) || sep(str[i]));
    });
  
  // offsets of fields based on separator
  auto f = [&] (size_t i) {return &str[i];};
  auto offset = pbbs::delayed_seq<char*>(n, f);
  sequence<char*> splits = pbbs::pack(offset, FL);
  size_t m = splits.size();

  sequence<bool> FX(m);
  FX[0] = 1;
  parallel_for (1, m, [&] (size_t i) {
      FX[i] = is_line(*(splits[i]-1)); });
  parallel_for (1, m, [&] (size_t i) {
      *(splits[i]-1) = 0; });

  // line offsets within the field offsets
  sequence<size_t> line_starts = pbbs::pack_index<size_t>(FX);
  size_t l = line_starts.size();

  // for each line grab slice of fields
  sequence<sequence<char*>> lines(l);
  parallel_for (0, l, [&] (size_t i) {
    size_t start = line_starts[i];
    size_t end = (i == l-1) ? m : line_starts[i+1];
    lines[i] = splits.slice(start, end);
    });

  // check all lines are the same length
  size_t e = lines[0].size();
  size_t j = n;
  parallel_for (0, l, [&] (size_t i) {
      if (lines[i].size() != e) j = i;
    });
  if (j != n)
    cout << "items in line " << j << " not equal to line 0" << endl;

  if (verbose) {
    cout << "lines = " << l << ", entries/line = " << e << endl;
  }

  return csv_data(move(str), move(splits), move(lines));
}

    
