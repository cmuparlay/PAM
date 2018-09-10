#pragma once

#ifdef LARGE
const size_t maxsize = ((size_t) 1) << 50; // 50 made up arbirarily
typedef size_t tree_size_t;
#else
// uses 32 bits for various counts, so no more than 2^32-1 nodes
const size_t maxsize = (((size_t) 1) << 32) - 1; // 2^32 - 1
typedef unsigned int tree_size_t;
#endif

// for granularity control
// if the size of a node is smaller than this number then it
// processed sequentially instead of in parallel
constexpr const size_t node_limit = 250;



