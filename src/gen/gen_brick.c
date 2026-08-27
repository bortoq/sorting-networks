#include "gen_common.h"
#include <string.h>

net_t gen_brick(unsigned n){
  if(n==0 || n>65536) die("gen: n out of range 1..65536");
  net_t out={0,0,NULL};
  if(n < 2) return out;
  for(unsigned layer=0; layer<n; ++layer){
    unsigned start = (layer % 2 == 0) ? 0 : 1;
    for(unsigned i=start; i+1<n; i+=2) net_add(&out, layer, i, i+1);
  }
  return out;
}
// --- Green filter (Green 1969) for n=16 ----------------------------------
