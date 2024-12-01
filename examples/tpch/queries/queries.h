#include <string.h>
#include <mutex>
#include "parlay/hash_table.h"

constexpr bool QUERY_OUT = false;
std::atomic<bool> finish(false);
constexpr size_t NUM_QUERIES = 22;
constexpr size_t RPT = 5;

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

void collect_history()
{
	return;
	// if (!if_collect) return;
	// size_t y = history.size();
	// while (start_history < y - 2) {
	// 	history[start_history++].clear();
	// }
	// size_t t = line_item_set::GC::used_node();
	// if (t > max_lineitem) max_lineitem = t;
	// t = order_map::GC::used_node();
	// if (t > max_order) max_order = t;
	// t = customer_map::GC::used_node();
	// if (t > max_customer) max_customer = t;
}

maps index_snapshot;
std::mutex index_mutex;

template <typename M, typename F, typename... Args> 
auto with_lock(M& mx, F&& f, Args&&... args) {
	std::scoped_lock<M> lk(mx);
	return std::forward<F>(f)(std::forward<Args>(args)...);
}

constexpr std::array<double (*)(maps, bool), NUM_QUERIES> queries{
		Q1time,
		Q2time,
		Q3time,
		Q4time,
		Q5time,
		Q6time,
		Q7time,
		Q8time,
		Q9time,
		Q10time,
		Q11time,
		Q12time,
		Q13time,
		Q14time,
		Q15time,
		Q16time,
		Q17time,
		Q18time,
		Q19time,
		Q20time,
		Q21time,
		Q22time
};

using query_timings = std::array<parlay::sequence<double>, NUM_QUERIES>;

query_timings exe_query(bool verbose)
{
	std::cout << "start queries" << endl;

	ios::fmtflags cout_settings = std::cout.flags();
	std::cout.precision(4);
	std::cout << fixed;

	query_timings timings;

	size_t round = 0;
	for (; round <= RPT && finish.load(); round++)
	{
		std::cout << "Round " << round << endl;

		for (size_t i = 0; i < NUM_QUERIES; i++)
		{
			size_t q = (i + 21) % 22;

			auto indices = with_lock(index_mutex, []() {
				return index_snapshot;
			});
			
			double elapsed = queries[q](std::move(indices), verbose);
			timings[i].push_back(elapsed);
			if (verbose)
			{
				std::cout << "Q" << q + 1 << " Time: " << elapsed << '\n';
			}
		}
	}
	std::cout.flags(cout_settings);
	std::cout << round << " rounds of queries" << endl;
	return timings;
}
