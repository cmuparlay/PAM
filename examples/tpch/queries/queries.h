bool query_out = false;
std::atomic<bool> finish(false);
int queries = 22;
int rpt = 5;


#include <string.h>
#include "parlay/hash_table.h"

using namespace parlay;
#include "Q1.h"
#include "Q2.h"
#include "Q3.h"
#include "Q4.h"
#include "Q5.h"
#include "Q6.h"
#include "Q7.h"
#include "Q8.h"
#include "Q9.h"
#include "Q10.h"
#include "Q11.h"
#include "Q12.h"
#include "Q13.h"
#include "Q14.h"
#include "Q15.h"
#include "Q16.h"
#include "Q17.h"
#include "Q18.h"
#include "Q19.h"
#include "Q20.h"
#include "Q21.h"
#include "Q22.h"


void collect_history() {
	return;
	if (!if_collect) return;
	size_t y = history.size();
	while (start_history < y - 2) {
		history[start_history++].clear();
	}
	size_t t = li_map::GC::used_node();
	if (t > max_lineitem) max_lineitem = t;
	t = order_map::GC::used_node();
	if (t > max_order) max_order = t;
	t = customer_map::GC::used_node();
	if (t > max_customer) max_customer = t;
}	

void exe_query(bool verbose, double** tm, int& round, int rpt = 0) {
  //maps m = history[history.size()-1];
  round = 0;
  cout << "start queries" << endl;

  ios::fmtflags cout_settings = cout.flags();
  std::cout.precision(4);
  cout << fixed;
  
  while (true) {
	  if (finish) break;
	  maps m2; 
	  
	  cout << "Round " << round << endl;
	  working_version = history.size()-1; 
	  m2 = history[working_version];
	  //tm[0][round] = Q22time(m2, verbose);
	  if (verbose) cout << "Q22 Time: " << tm[0][round] << endl;

	  working_version = history.size()-1; m2 = history[working_version];
	  tm[1][round] = Q1time(m2, verbose);
	  if (verbose) cout << "Q1 Time: " << tm[1][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[2][round] = Q2time(m2, verbose);
	  if (verbose) cout << "Q2 Time: " << tm[2][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[3][round] = Q3time(m2, verbose);
	  if (verbose) cout << "Q3 Time: " << tm[3][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[4][round] = Q4time(m2, verbose);
	  if (verbose) cout << "Q4 Time: " << tm[4][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[5][round] = Q5time(m2, verbose);
	  if (verbose) cout << "Q5 Time: " << tm[5][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[6][round] = Q6time(m2, verbose);
	  if (verbose) cout << "Q6 Time: " << tm[6][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[7][round] = Q7time(m2, verbose);
	  if (verbose) cout << "Q7 Time: " << tm[7][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[8][round] = Q8time(m2, verbose);
	  if (verbose) cout << "Q8 Time: " << tm[8][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[9][round] = Q9time(m2, verbose);
	  if (verbose) cout << "Q9 Time: " << tm[9][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  //tm[10][round] = Q10time(m2, verbose);
	  if (verbose) cout << "Q10 Time: " << tm[10][round] << endl; 
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[11][round] = Q11time(m2, verbose);
	  if (verbose) cout << "Q11 Time: " << tm[11][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[12][round] = Q12time(m2, verbose);
	  if (verbose) cout << "Q12 Time: " << tm[12][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[13][round] = Q13time(m2, verbose);
	  if (verbose) cout << "Q13 Time: " << tm[13][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[14][round] = Q14time(m2, verbose);
	  if (verbose) cout << "Q14 Time: " << tm[14][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  //tm[15][round] = Q15time(m2, verbose);
	  if (verbose) cout << "Q15 Time: " << tm[15][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[16][round] = Q16time(m2, verbose);
	  if (verbose) cout << "Q16 Time: " << tm[16][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[17][round] = Q17time(m2, verbose);
	  if (verbose) cout << "Q17 Time: " << tm[17][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[18][round] = Q18time(m2, verbose);
	  if (verbose) cout << "Q18 Time: " << tm[18][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[19][round] = Q19time(m2, verbose);
	  if (verbose) cout << "Q19 Time: " << tm[19][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[20][round] = Q20time(m2, verbose);
	  if (verbose) cout << "Q20 Time: " << tm[20][round] << endl;
	  
	  working_version = history.size()-1; m2 = history[working_version];
	  tm[21][round] = Q21time(m2, verbose);
	  if (verbose) cout << "Q21 Time: " << tm[21][round] << endl;

	  round++;
	  if (rpt > 0 && round > rpt) break;
  }
  cout.flags(cout_settings);
  cout << round << " rounds of queries" << endl;
}

