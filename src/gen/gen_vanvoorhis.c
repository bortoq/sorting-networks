#include "gen_common.h"
#include <string.h>

unsigned next_vv_size(unsigned n)
{
  unsigned p = 16;
  if(n <= 16) return 16;
  while(p < n){
    if(p > (1u<<30)) die("van-voorhis size too large");
    if(p > (1u<<30)) die("van-voorhis size too large");
    p = p * p;
  }
  return p;
}

net_t gen_van_voorhis(unsigned n)
{
  if(n==0 || n>65536) die("gen: n out of range 1..65536");
  seq_t seq = {0, 0, NULL};
  unsigned *map;
  unsigned m = 0;
  unsigned i;
  unsigned p;
  net_t out;
  unsigned orig_n = n;

  for(p = n; p > 1; p >>= 1) ++m;
  int is_vv = is_power2(n) && m != 0 && (m & (m - 1)) == 0;
  if(!is_vv){
    // Generalized: pad to next 2^(2^k), generate, truncate wires >= orig_n
    unsigned padded = next_vv_size(n);
    net_t padded_net = gen_van_voorhis(padded);
    seq_t filtered = {0,0,NULL};
    for(unsigned l=0;l<padded_net.layers;++l)
      for(unsigned k=0;k<padded_net.layer[l].count;++k){
        unsigned a=padded_net.layer[l].pair[k].left, b=padded_net.layer[l].pair[k].right;
        if(a < orig_n && b < orig_n) seq_add(&filtered, a,b);
      }
    net_free(&padded_net);
    out = seq_pack(&filtered, orig_n);
    seq_free(&filtered);
    return out;
  }

  map = malloc(n * sizeof *map);
  if(map == NULL) die("cannot allocate top-level map");
  for(i=0;i<n;++i) map[i]=i;
  // Reserve to avoid ~12 reallocs for large n (e.g. 65536 -> 3.9M)
  if(n >= 256) seq_reserve(&seq, 4000000);
  gen_vv_sort_seq(&seq, map, m);
  out = seq_pack(&seq, n);
  free(map);
  seq_free(&seq);
  return out;
}

