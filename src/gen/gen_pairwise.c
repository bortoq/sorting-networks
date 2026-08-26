#include "gen_common.h"
#include <string.h>

net_t gen_pairwise(unsigned n)
{
  net_t out = {0, 0, NULL};
  unsigned p;
  unsigned orig_n = n;
  if(!is_power2(n)){
    unsigned padded = next_pow2(n);
    net_t pn = gen_pairwise(padded);
    seq_t f={0,0,NULL};
    for(unsigned l=0;l<pn.layers;++l) for(unsigned k=0;k<pn.layer[l].count;++k){
      unsigned a=pn.layer[l].pair[k].left, b=pn.layer[l].pair[k].right;
      if(a < orig_n && b < orig_n) seq_add(&f,a,b);
    }
    net_free(&pn);
    out = seq_pack(&f, orig_n);
    seq_free(&f);
    return out;
  }

  for(p = n / 2; p >= 1; p /= 2)
  {
    unsigned a;
    unsigned b;
    unsigned q;
    unsigned layer = out.layers;

    net_ensure_layer(&out, layer);
    for(a = 0; a < n; a += p * 2)
      for(b = 0; b < p; ++b)
        net_add(&out, layer, a + b, a + b + p);

    for(q = n / 2; q >= p * 2; q /= 2)
    {
      unsigned c;
      unsigned d;

      layer = out.layers;
      net_ensure_layer(&out, layer);
      for(c = 0; c < n; c += p * 2)
        for(d = 0; d < p; ++d)
          if(c + d + q < n)
            net_add(&out, layer, c + d + p, c + d + q);
    }

    if(p == 1)
      break;
  }

  return out;
}

// --- Bose-Nelson (1962) -------------------------------------------------
