#include "gen_common.h"
#include <string.h>

static void bose_pbracket(seq_t *seq, unsigned i, unsigned x, unsigned j, unsigned y)
{
  if (x == 1 && y == 1)
    seq_add(seq, i, j);
  else if (x == 1 && y == 2)
  {
    seq_add(seq, i, j + 1);
    seq_add(seq, i, j);
  }
  else if (x == 2 && y == 1)
  {
    seq_add(seq, i, j);
    seq_add(seq, i + 1, j);
  }
  else
  {
    unsigned a = x / 2;
    unsigned b = (x & 1) ? (y / 2) : ((y + 1) / 2);
    bose_pbracket(seq, i, a, j, b);
    bose_pbracket(seq, i + a, x - a, j + b, y - b);
    bose_pbracket(seq, i + a, x - a, j, b);
  }
}
static void bose_pstar(seq_t *seq, unsigned i, unsigned m)
{
  if (m <= 1)
    return;
  unsigned a = m / 2;
  bose_pstar(seq, i, a);
  bose_pstar(seq, i + a, m - a);
  bose_pbracket(seq, i, a, i + a, m - a);
}
net_t gen_bose_nelson(unsigned n)
{
  if (n == 0 || n > 65536)
    die("gen: n out of range 1..65536");
  seq_t seq = {0, 0, NULL};
  bose_pstar(&seq, 0, n);
  net_t out = seq_pack(&seq, n);
  seq_free(&seq);
  return out;
}
// --- Bitonic mergesort (Batcher second) ---------------------------------
