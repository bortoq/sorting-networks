#include "gen_common.h"
#include <string.h>

static const pair_t green16[4][8] = {
    {{0, 5}, {1, 4}, {2, 12}, {3, 13}, {6, 7}, {8, 9}, {10, 15}, {11, 14}},
    {{0, 2}, {1, 10}, {3, 6}, {4, 7}, {5, 14}, {8, 11}, {9, 12}, {13, 15}},
    {{0, 8}, {1, 3}, {2, 11}, {4, 13}, {5, 9}, {6, 10}, {7, 15}, {12, 14}},
    {{0, 1}, {2, 4}, {3, 8}, {5, 6}, {7, 12}, {9, 10}, {11, 13}, {14, 15}}};
net_t gen_green(unsigned n)
{
  if (n == 0 || n > 65536)
    die("gen: n out of range 1..65536");
  net_t out = {0, 0, NULL};
  if (n == 16)
  {
    for (unsigned l = 0; l < 4; ++l)
      for (unsigned k = 0; k < 8; ++k)
        net_add(&out, l, green16[l][k].left, green16[l][k].right);
    return out;
  }
  if (n < 16)
  {
    net_t g16 = gen_green(16);
    seq_t f = {0, 0, NULL};
    for (unsigned l = 0; l < g16.layers; ++l)
      for (unsigned k = 0; k < g16.layer[l].count; ++k)
      {
        unsigned a = g16.layer[l].pair[k].left, b = g16.layer[l].pair[k].right;
        if (a < n && b < n)
          seq_add(&f, a, b);
      }
    net_free(&g16);
    out = seq_pack(&f, n);
    seq_free(&f);
    return out;
  }
  seq_t f = {0, 0, NULL};
  for (unsigned l = 0; l < 4; ++l)
    for (unsigned k = 0; k < 8; ++k)
      seq_add(&f, green16[l][k].left, green16[l][k].right);
  out = seq_pack(&f, n);
  seq_free(&f);
  return out;
}
// --- Zig-zag Sort (Goodrich 2014) ---------------------------------------
