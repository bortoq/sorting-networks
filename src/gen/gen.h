#ifndef GEN_H
#define GEN_H
#include "gen_common.h"
net_t gen_pairwise(unsigned n);
net_t gen_batcher_odd_even(unsigned n);
net_t gen_pipelined_mergesort(unsigned n);
net_t gen_bose_nelson(unsigned n);
net_t gen_bitonic(unsigned n);
net_t gen_brick(unsigned n);
net_t gen_green(unsigned n);
net_t gen_van_voorhis(unsigned n);
net_t gen_zigzag(unsigned n);
#endif
