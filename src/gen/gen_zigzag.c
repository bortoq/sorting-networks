#include "gen_common.h"
#include <string.h>

static void zz_halver(seq_t *seq, unsigned *A, unsigned *B, unsigned n){
  for(unsigned i=0;i<n;++i) seq_add(seq, A[i], B[i]);
}
static void zz_sort_small(seq_t *seq, unsigned *W, unsigned n){
  for(unsigned i=0;i<n;++i) for(unsigned j=i+1;j<n;++j) seq_add(seq, W[i], W[j]);
}
static void zz_attenuate(seq_t *seq, unsigned *A, unsigned *B, unsigned n);
static void zz_reduce(seq_t *seq, unsigned *A, unsigned *B, unsigned n);
static void zz_attenuate(seq_t *seq, unsigned *A, unsigned *B, unsigned n){
  if(n <= 4){
    unsigned W[16];
    for(unsigned i=0;i<n;++i) W[i]=A[i];
    for(unsigned i=0;i<n;++i) W[n+i]=B[i];
    zz_sort_small(seq, W, 2*n);
    return;
  }
  unsigned n2=n/2, n4=n/4;
  unsigned *A1=A, *A2=A+n2;
  unsigned *B1=B, *B2=B+n2;
  zz_halver(seq, A1, A2, n2);
  zz_halver(seq, B1, B2, n2);
  zz_halver(seq, A2, B1, n2);
  zz_attenuate(seq, A2, B1, n2);
  unsigned *A1_2=A2, *A2_2=A2+n4;
  unsigned *B1_2=B1, *B2_2=B1+n4;
  zz_halver(seq, A1_2, A2_2, n4);
  zz_halver(seq, B1_2, B2_2, n4);
  zz_halver(seq, A2_2, B1_2, n4);
  zz_attenuate(seq, A2_2, B1_2, n4);
}
static void zz_reduce(seq_t *seq, unsigned *A, unsigned *B, unsigned n){
  if(n <= 4){
    unsigned W[16];
    for(unsigned i=0;i<n;++i) W[i]=A[i];
    for(unsigned i=0;i<n;++i) W[n+i]=B[i];
    zz_sort_small(seq, W, 2*n);
    return;
  }
  zz_halver(seq, A, B, n);
  zz_attenuate(seq, A, B, n);
}
static void zigzag_sort_rec(seq_t *seq, unsigned *W, unsigned n){
  if(n <= 1) return;
  unsigned k=0; for(unsigned t=n; t>1; t>>=1) ++k;
  for(unsigned j=1; j<=k; ++j){
    unsigned num_sub = 1u << j;
    unsigned sub_sz = n >> j;
    for(unsigned i=0; i<num_sub; i+=2){
      unsigned off1 = i*sub_sz;
      unsigned off2 = (i+1)*sub_sz;
      zz_reduce(seq, W+off1, W+off2, sub_sz);
    }
    for(unsigned i=0; i+1<num_sub; ++i){
      unsigned off1 = i*sub_sz;
      unsigned off2 = (i+1)*sub_sz;
      for(unsigned t=0; t<sub_sz; ++t){ unsigned tmp=W[off1+t]; W[off1+t]=W[off2+t]; W[off2+t]=tmp; }
      zz_reduce(seq, W+off1, W+off2, sub_sz);
    }
    for(int i=(int)num_sub-1; i>=1; --i){
      unsigned off1 = (i-1)*sub_sz;
      unsigned off2 = i*sub_sz;
      for(unsigned t=0; t<sub_sz; ++t){ unsigned tmp=W[off1+t]; W[off1+t]=W[off2+t]; W[off2+t]=tmp; }
      zz_reduce(seq, W+off1, W+off2, sub_sz);
    }
  }
}
net_t gen_zigzag(unsigned n){
  if(n==0 || n>65536) die("gen: n out of range 1..65536");
  if(!is_power2(n)) die("zig-zag requires n to be a power of two");
  seq_t seq={0,0,NULL};
  unsigned *W = malloc(n*sizeof *W);
  if(!W) die("cannot allocate W");
  for(unsigned i=0;i<n;++i) W[i]=i;
  zigzag_sort_rec(&seq, W, n);
  net_t out = seq_pack(&seq, n);
  free(W);
  seq_free(&seq);
  return out;
}
