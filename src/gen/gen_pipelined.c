#include "gen_common.h"
#include <string.h>

net_t gen_pipelined_merge(unsigned lo, unsigned n, unsigned r)
{
  unsigned m = r * 2;
  net_t out = {0, 0, NULL};
  if (m < n)
  {
    net_t a = gen_pipelined_merge(lo, n, m);
    net_t b = gen_pipelined_merge(lo + r, n, m);
    net_t p = net_parallel(&a, &b);
    unsigned i;
    net_concat(&out, &p);
    net_free(&p);
    net_ensure_layer(&out, out.layers);
    for (i = lo + r; i + r < lo + n; i += m)
      layer_add(&out.layer[out.layers - 1], i, i + r);
  }
  else
  {
    net_add(&out, 0, lo, lo + r);
  }
  return out;
}

net_t gen_pipelined_sort_rec(unsigned lo, unsigned n)
{
  net_t out = {0, 0, NULL};
  if (n <= 1)
    return out;
  {
    net_t a = gen_pipelined_sort_rec(lo, n / 2);
    net_t b = gen_pipelined_sort_rec(lo + n / 2, n / 2);
    net_t p = net_parallel(&a, &b);
    net_t m = gen_pipelined_merge(lo, n, 1);
    net_concat(&out, &p);
    // Pipeline register: one idle layer between sort and merge stages
    net_ensure_layer(&out, out.layers);
    net_concat(&out, &m);
    net_free(&p);
    net_free(&m);
  }
  return out;
}

net_t gen_pipelined_mergesort(unsigned n)
{
  if (n == 0 || n > 65536)
    die("gen: n out of range 1..65536");
  if (!is_power2(n))
  {
    unsigned padded = next_pow2(n);
    net_t pn = gen_pipelined_sort_rec(0, padded);
    seq_t f = {0, 0, NULL};
    for (unsigned l = 0; l < pn.layers; ++l)
      for (unsigned k = 0; k < pn.layer[l].count; ++k)
      {
        unsigned a = pn.layer[l].pair[k].left, b = pn.layer[l].pair[k].right;
        if (a < n && b < n)
          seq_add(&f, a, b);
      }
    net_free(&pn);
    net_t out = seq_pack(&f, n);
    seq_free(&f);
    return out;
  }
  return gen_pipelined_sort_rec(0, n);
}
