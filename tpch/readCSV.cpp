#include <cctype>
#include "lineitem.h"
#include "pbbslib/get_time.h"
#include "pbbslib/par_string.h"
#include "pbbslib/parse_command_line.h"

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

struct data {
  data(sequence<char> a, sequence<char*> b, sequence<sequence<char*>> c) :
    str(move(a)), splits(move(b)), lines(move(c)) {};
  sequence<char> str;
  sequence<char*> splits;
  sequence<sequence<char*>> lines;
};

template <class F>
data readCSV(string filename, F sep, bool verbose = false) {

  startTime();
  sequence<char> str = read_string_from_file(filename, 0, 100000000000ul);
  size_t n = str.size();
  if (verbose) nextTime("read file");
  
  sequence<bool> FL(n);
  FL[0] = 1;
  parallel_for (size_t i=0; i < n-1; i++) 
    FL[i+1] = (is_line(str[i]) || sep(str[i]));
  if (verbose) nextTime("separator check");
      
  auto f = [&] (size_t i) {return &str[i];};
  auto offset = make_sequence<char*>(n, f);
  sequence<char*> splits = pbbs::pack(offset, FL);
  size_t m = splits.size();
  if (verbose) nextTime("splits");

  sequence<bool> FX(m);
  FX[0] = 1;
  parallel_for (size_t i=1; i < m; i++) 
    FX[i] = is_line(*(splits[i]-1));
  parallel_for (size_t i=1; i < m; i++) 
    *(splits[i]-1) = 0;

  sequence<size_t> line_starts = pbbs::pack_index<size_t>(FX);
  size_t l = line_starts.size();
  if (verbose) nextTime("line starts");

  sequence<sequence<char*>> lines(l);
  parallel_for (size_t i=0; i < l; i++) {
    size_t start = line_starts[i];
    size_t end = (i == l-1) ? m : line_starts[i+1];
    lines[i] = splits.slice(start, end);
  }
  if (verbose) nextTime("slice");

  if (verbose) {
    size_t e = lines[0].size();
    cout << "lines = " << l << endl;
    cout << "entries/line = " << e << endl;
    size_t j = n;
    parallel_for (size_t i=0; i < l; i++) 
      if (lines[i].size() != e) j = i;
    if (j != n)
      cout << "items in line " << j << " not equal to line 0" << endl;
  }

  if (verbose) nextTime("check same length");
  return data(move(str), move(splits), move(lines));
}

    
