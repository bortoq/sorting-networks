#include "gen_common.h"
#include <string.h>

static void bitonic_merge(seq_t *seq, unsigned lo, unsigned n, int dir)
{
  if (n <= 1)
    return;
  unsigned k = n / 2;
  for (unsigned i = lo; i < lo + k; ++i)
  {
    if (dir)
      seq_add_directed(seq, i, i + k); // ascending: min->i
    else
      seq_add_directed(seq, i + k, i); // descending: min->i+k
  }
  bitonic_merge(seq, lo, k, dir);
  bitonic_merge(seq, lo + k, k, dir);
}
static void bitonic_sort_rec(seq_t *seq, unsigned lo, unsigned n, int dir)
{
  if (n <= 1)
    return;
  unsigned k = n / 2;
  bitonic_sort_rec(seq, lo, k, dir);
  bitonic_sort_rec(seq, lo + k, k, dir ^ 1);
  bitonic_merge(seq, lo, n, dir);
}
net_t gen_bitonic(unsigned n)
{
  if (n == 0 || n > 65536)
    die("gen: n out of range 1..65536");
  if (!is_power2(n))
    die("bitonic requires n to be a power of two");
  seq_t seq = {0, 0, NULL};
  bitonic_sort_rec(&seq, 0, n, 1);
  net_t out = seq_pack_directed(&seq, n);
  seq_free(&seq);
  return out;
}
// --- Brick / Odd-Even Transposition (periodic) ----------------------------
