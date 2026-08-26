/*
 * Sorting network generators - classical constructions + packing
 *
 * Theory:
 *   Four algorithms, all produce comparators as sequence then pack into
 *   layers via wire-ready table (ready[w] = next free layer for wire w):
 *
 *   1) Pairwise (pairwise sorting network): power-of-two only (n=2^k),
 *      recursively sort pairs and merge. Simple but not depth-optimal.
 *
 *   2) Batcher odd-even mergesort (Batcher 1968): power-of-two only.
 *      Recursively sort halves, then odd-even merge with stride r.
 *      Depth O(log^2 n), size O(n log^2 n). Reference implementation for
 *      comparison, not worst-case optimal but practical.
 *
 *   3) Pipelined mergesort: same comparators as Batcher, but inserts
 *      a pipeline register (empty layer) between sort and merge stages.
 *      Slightly deeper (16:14 vs 10, 32:20 vs 15) but pipeline-friendly.
 *      Power-of-two only (padded else).
 *
 *   4) Van Voorhis square (Van Voorhis 1971, Lee 1986): n = 2^(2^k) only
 *      (16,256,65536..). Recursive square decomposition:
 *        - sort rows (r = sqrt(n), r networks)
 *        - sort columns (r networks)
 *        - f-network: economical [2^r,2^r] merger (uses gen_batcher_merge)
 *        - pack comparator sequence into layers via ready[].
 *      For n=16: 61 comparators, 10 layers (depth-optimal).
 *      For n=65536: 3907497 comps, 136 layers.
 *      Our implementation follows Knuth's square scheme.
 *
 *   5) Bose-Nelson (1962): any n, recursive Pbracket/Pstar
 *   6) Bitonic mergesort: power of two, ascending/descending bitonic merge
 *   7) Brick (odd-even transposition): any n, n layers periodic
 *   8) Green filter (1969): n=16 filter (4 layers) truncated/padded else
 *
 * Output format: one comparator "i j" per line, blank line between layers.
 * Refs: Batcher 1968 "Sorting Networks...", Van Voorhis 1971, Knuth 5.3.4
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  unsigned left;
  unsigned right;
} pair_t;

typedef struct {
  unsigned count;
  unsigned cap;
  pair_t *pair;
} layer_t;

typedef struct {
  unsigned layers;
  unsigned cap;
  layer_t *layer;
} net_t;

typedef struct {
  unsigned count;
  unsigned cap;
  pair_t *pair;
} seq_t;

static void die(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static int is_power2(unsigned n)
{
  return n && ((n & (n - 1)) == 0);
}

static unsigned next_pow2(unsigned n){ unsigned p=1; while(p<n) p<<=1; return p; }

static void layer_free(layer_t *layer)
{
  if(layer == NULL)
    return;
  free(layer->pair);
  layer->pair = NULL;
  layer->count = 0;
  layer->cap = 0;
}

static void net_free(net_t *net)
{
  unsigned i;

  if(net == NULL)
    return;
  for(i = 0; i < net->layers; ++i)
    layer_free(&net->layer[i]);
  free(net->layer);
  net->layer = NULL;
  net->layers = 0;
  net->cap = 0;
}

static void seq_free(seq_t *seq)
{
  if(seq == NULL)
    return;
  free(seq->pair);
  seq->pair = NULL;
  seq->count = 0;
  seq->cap = 0;
}

static void net_reserve_layers(net_t *net, unsigned layers)
{
  layer_t *layer;
  unsigned i;

  if(layers <= net->cap)
    return;

  layer = realloc(net->layer, layers * sizeof *layer);
  if(layer == NULL)
    die("cannot allocate layers");

  for(i = net->cap; i < layers; ++i)
    memset(&layer[i], 0, sizeof layer[i]);

  net->layer = layer;
  net->cap = layers;
}

static void net_ensure_layer(net_t *net, unsigned idx)
{
  if(idx >= net->cap)
    net_reserve_layers(net, idx + 1);
  if(idx >= net->layers)
    net->layers = idx + 1;
}

static void layer_add(layer_t *layer, unsigned left, unsigned right)
{
  pair_t *pair;

  if(left == right)
    return;
  if(layer->count == layer->cap)
  {
    unsigned cap = layer->cap ? layer->cap * 2 : 8;
    pair = realloc(layer->pair, cap * sizeof *pair);
    if(pair == NULL)
      die("cannot allocate comparators");
    layer->pair = pair;
    layer->cap = cap;
  }

  layer->pair[layer->count].left = left;
  layer->pair[layer->count].right = right;
  ++layer->count;
}

static void layer_add_directed(layer_t *layer, unsigned left, unsigned right)
{
  // Do not normalize, keep direction
  if(left == right) return;
  pair_t *pair;
  if(layer->count == layer->cap){
    unsigned cap = layer->cap ? layer->cap * 2 : 8;
    pair = realloc(layer->pair, cap * sizeof *pair);
    if(pair == NULL) die("cannot allocate comparators");
    layer->pair = pair; layer->cap = cap;
  }
  layer->pair[layer->count].left = left;
  layer->pair[layer->count].right = right;
  ++layer->count;
}

static void seq_reserve(seq_t *seq, unsigned cap){
  if(cap <= seq->cap) return;
  pair_t *p = realloc(seq->pair, cap * sizeof *p);
  if(!p) die("cannot reserve seq");
  seq->pair=p; seq->cap=cap;
}

static void seq_add(seq_t *seq, unsigned left, unsigned right)
{
  pair_t *pair;

  if(left == right)
    return;
  if(left > right)
  {
    unsigned tmp = left;
    left = right;
    right = tmp;
  }

  if(seq->count == seq->cap)
  {
    unsigned cap = seq->cap ? seq->cap * 2 : 1024;
    pair = realloc(seq->pair, cap * sizeof *pair);
    if(pair == NULL)
      die("cannot allocate comparator sequence");
    seq->pair = pair;
    seq->cap = cap;
  }

  seq->pair[seq->count].left = left;
  seq->pair[seq->count].right = right;
  ++seq->count;
}

static void seq_add_directed(seq_t *seq, unsigned left, unsigned right)
{
  // For bitonic: keep direction (left gets min), do not normalize
  if(left == right) return;
  pair_t *pair;
  if(seq->count == seq->cap){
    unsigned cap = seq->cap ? seq->cap * 2 : 1024;
    pair = realloc(seq->pair, cap * sizeof *pair);
    if(pair == NULL) die("cannot allocate comparator sequence");
    seq->pair = pair; seq->cap = cap;
  }
  seq->pair[seq->count].left = left;
  seq->pair[seq->count].right = right;
  ++seq->count;
}

static void net_add(net_t *net, unsigned layer, unsigned left, unsigned right)
{
  net_ensure_layer(net, layer);
  layer_add(&net->layer[layer], left, right);
}
static void net_add_directed(net_t *net, unsigned layer, unsigned left, unsigned right)
{
  net_ensure_layer(net, layer);
  layer_add_directed(&net->layer[layer], left, right);
}

static void net_append_layer(net_t *dst, const layer_t *src)
{
  unsigned i;
  unsigned layer = dst->layers;

  net_ensure_layer(dst, layer);
  for(i = 0; i < src->count; ++i)
    layer_add(&dst->layer[layer], src->pair[i].left, src->pair[i].right);
}

static void net_concat(net_t *dst, const net_t *src)
{
  unsigned i;

  for(i = 0; i < src->layers; ++i)
    net_append_layer(dst, &src->layer[i]);
}

static net_t net_parallel(net_t *a, net_t *b)
{
  net_t out = {0, 0, NULL};
  unsigned i;
  unsigned layers = a->layers > b->layers ? a->layers : b->layers;

  for(i = 0; i < layers; ++i)
  {
    unsigned j;
    net_ensure_layer(&out, i);
    if(i < a->layers)
      for(j = 0; j < a->layer[i].count; ++j)
        layer_add(&out.layer[i], a->layer[i].pair[j].left, a->layer[i].pair[j].right);
    if(i < b->layers)
      for(j = 0; j < b->layer[i].count; ++j)
        layer_add(&out.layer[i], b->layer[i].pair[j].left, b->layer[i].pair[j].right);
  }

  net_free(a);
  net_free(b);
  return out;
}

static void net_print(const net_t *net)
{
  unsigned i;
  unsigned j;

  for(i = 0; i < net->layers; ++i)
  {
    for(j = 0; j < net->layer[i].count; ++j)
      printf("%u %u\n", net->layer[i].pair[j].left, net->layer[i].pair[j].right);
    if(i + 1 < net->layers)
      putchar('\n');
  }
}

static uint64_t net_cmp_count(const net_t *net)
{
  uint64_t count = 0;
  unsigned i;

  for(i = 0; i < net->layers; ++i)
    count += net->layer[i].count;

  return count;
}

static net_t seq_pack(const seq_t *seq, unsigned wires)
{
  net_t out = {0, 0, NULL};
  unsigned *ready;
  unsigned i;

  // ready[w] = earliest layer where wire w is free (greedy layer packing)
  ready = calloc(wires, sizeof *ready);
  if(ready == NULL)
    die("cannot allocate wire readiness table");

  for(i = 0; i < seq->count; ++i)
  {
    unsigned left = seq->pair[i].left;
    unsigned right = seq->pair[i].right;
    unsigned layer = ready[left] > ready[right] ? ready[left] : ready[right]; // earliest layer where both wires free

    net_add(&out, layer, left, right);
    ready[left] = layer + 1;
    ready[right] = layer + 1;
  }

  free(ready);
  return out;
}

static net_t seq_pack_directed(const seq_t *seq, unsigned wires)
{
  net_t out={0,0,NULL};
  unsigned *ready=calloc(wires,sizeof *ready);
  if(!ready) die("cannot allocate readiness");
  for(unsigned i=0;i<seq->count;++i){
    unsigned left=seq->pair[i].left, right=seq->pair[i].right;
    unsigned layer = ready[left] > ready[right] ? ready[left] : ready[right];
    net_add_directed(&out, layer, left, right);
    ready[left]=ready[right]=layer+1;
  }
  free(ready); return out;
}

static unsigned *map_subset(const unsigned *map, unsigned rows, unsigned cols,
                            unsigned row_start, unsigned col_start)
{
  unsigned sub_rows = rows / 2;
  unsigned sub_cols = cols / 2;
  unsigned *sub;
  unsigned row;
  unsigned col;

  sub = malloc(sub_rows * sub_cols * sizeof *sub);
  if(sub == NULL)
    die("cannot allocate subnetwork map");

  for(row = 0; row < sub_rows; ++row)
    for(col = 0; col < sub_cols; ++col)
      sub[row * sub_cols + col] =
        map[(row * 2 + row_start) * cols + (col * 2 + col_start)];

  return sub;
}

static void gen_vv_f_square(seq_t *seq, const unsigned *map,
                            unsigned rows, unsigned r);
static void gen_vv_delta(seq_t *seq, const unsigned *map,
                         unsigned rows, unsigned r);

static void gen_vv_h(seq_t *seq, const unsigned *map, unsigned rows, unsigned r)
{
  unsigned cols = 1u << r;
  unsigned half = cols / 2;
  unsigned row;
  unsigned s;

  for(row = 0; row + 1 < rows; ++row)
    for(s = 0; s < half; ++s)
      seq_add(seq, map[row * cols + (2 * s + 1)],
              map[(row + 1) * cols + (2 * s)]);

  if(r == 1)
    return;

  for(row = 0; row + 1 < rows; ++row)
    for(s = 0; s < half; ++s)
      seq_add(seq, map[row * cols + (half + s)],
              map[(row + 1) * cols + s]);

  gen_vv_delta(seq, map, rows * 2, r - 1);
}

static void gen_vv_delta(seq_t *seq, const unsigned *map,
                         unsigned rows, unsigned r)
{
  unsigned cols = 1u << r;
  unsigned half = cols / 2;
  unsigned row;
  unsigned s;

  for(row = 1; row + 2 < rows; ++row)
    for(s = 0; s < half; ++s)
      seq_add(seq, map[row * cols + (2 * s + 1)],
              map[(row + 1) * cols + (2 * s)]);

  if(r == 1)
    return;

  for(row = 0; row + 1 < rows; ++row)
    for(s = 0; s < half; ++s)
      seq_add(seq, map[row * cols + (half + s)],
              map[(row + 1) * cols + s]);

  gen_vv_delta(seq, map, rows * 2, r - 1);
}

static void gen_vv_f_square(seq_t *seq, const unsigned *map,
                            unsigned rows, unsigned r)
{
  unsigned cols = 1u << r;
  unsigned row;
  unsigned *sub;

  if(r == 1)
  {
    for(row = 0; row + 1 < rows; ++row)
      seq_add(seq, map[row * cols + 1], map[(row + 1) * cols]);
    return;
  }

  sub = map_subset(map, rows, cols, 0, 0);
  gen_vv_f_square(seq, sub, rows / 2, r - 1);
  free(sub);

  sub = map_subset(map, rows, cols, 1, 0);
  gen_vv_f_square(seq, sub, rows / 2, r - 1);
  free(sub);

  sub = map_subset(map, rows, cols, 0, 1);
  gen_vv_f_square(seq, sub, rows / 2, r - 1);
  free(sub);

  sub = map_subset(map, rows, cols, 1, 1);
  gen_vv_f_square(seq, sub, rows / 2, r - 1);
  free(sub);

  gen_vv_h(seq, map, rows, r);
}

static void gen_vv_sort_seq(seq_t *seq, const unsigned *map, unsigned m)
{
  unsigned side;
  unsigned row;
  unsigned col;
  unsigned *sub;

  if(m == 0)
    return;
  if(m == 1)
  {
    seq_add(seq, map[0], map[1]);
    return;
  }
  if(m % 2 != 0)
    die("van-voorhis currently supports n=2^(2^k)");

  side = 1u << (m / 2);

  for(row = 0; row < side; ++row)
    gen_vv_sort_seq(seq, &map[row * side], m / 2);

  sub = malloc(side * sizeof *sub);
  if(sub == NULL)
    die("cannot allocate column map");

  for(col = 0; col < side; ++col)
  {
    for(row = 0; row < side; ++row)
      sub[row] = map[row * side + col];
    gen_vv_sort_seq(seq, sub, m / 2);
  }

  free(sub);
  gen_vv_f_square(seq, map, side, m / 2);
}

static net_t gen_batcher_merge(unsigned lo, unsigned n, unsigned r)
{
  unsigned m = r * 2;
  net_t out = {0, 0, NULL};

  if(m < n)
  {
    net_t a = gen_batcher_merge(lo, n, m);
    net_t b = gen_batcher_merge(lo + r, n, m);
    net_t p = net_parallel(&a, &b);
    unsigned i;

    net_concat(&out, &p);
    net_free(&p);

    net_ensure_layer(&out, out.layers);
    for(i = lo + r; i + r < lo + n; i += m)
      layer_add(&out.layer[out.layers - 1], i, i + r);
  }
  else
  {
    net_add(&out, 0, lo, lo + r);
  }

  return out;
}

static net_t gen_batcher_sort_rec(unsigned lo, unsigned n)
{
  net_t out = {0, 0, NULL};

  if(n <= 1)
    return out;

  {
    net_t a = gen_batcher_sort_rec(lo, n / 2);
    net_t b = gen_batcher_sort_rec(lo + n / 2, n / 2);
    net_t p = net_parallel(&a, &b);
    net_t m = gen_batcher_merge(lo, n, 1);

    net_concat(&out, &p);
    net_concat(&out, &m);
    net_free(&p);
    net_free(&m);
  }

  return out;
}

static net_t gen_batcher_odd_even(unsigned n)
{
  if(!is_power2(n)){
    unsigned padded = next_pow2(n);
    net_t pn = gen_batcher_sort_rec(0, padded);
    seq_t f={0,0,NULL};
    for(unsigned l=0;l<pn.layers;++l) for(unsigned k=0;k<pn.layer[l].count;++k){
      unsigned a=pn.layer[l].pair[k].left, b=pn.layer[l].pair[k].right;
      if(a < n && b < n) seq_add(&f,a,b);
    }
    net_free(&pn);
    net_t out = seq_pack(&f, n);
    seq_free(&f);
    return out;
  }
  return gen_batcher_sort_rec(0, n);
}

static net_t gen_pipelined_merge(unsigned lo, unsigned n, unsigned r)
{
  unsigned m = r * 2;
  net_t out = {0, 0, NULL};
  if(m < n){
    net_t a = gen_pipelined_merge(lo, n, m);
    net_t b = gen_pipelined_merge(lo + r, n, m);
    net_t p = net_parallel(&a, &b);
    unsigned i;
    net_concat(&out, &p);
    net_free(&p);
    net_ensure_layer(&out, out.layers);
    for(i = lo + r; i + r < lo + n; i += m)
      layer_add(&out.layer[out.layers - 1], i, i + r);
  } else {
    net_add(&out, 0, lo, lo + r);
  }
  return out;
}

static net_t gen_pipelined_sort_rec(unsigned lo, unsigned n)
{
  net_t out = {0, 0, NULL};
  if(n <= 1) return out;
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

static net_t gen_pipelined_mergesort(unsigned n)
{
  if(!is_power2(n)){
    unsigned padded = next_pow2(n);
    net_t pn = gen_pipelined_sort_rec(0, padded);
    seq_t f={0,0,NULL};
    for(unsigned l=0;l<pn.layers;++l) for(unsigned k=0;k<pn.layer[l].count;++k){
      unsigned a=pn.layer[l].pair[k].left, b=pn.layer[l].pair[k].right;
      if(a < n && b < n) seq_add(&f,a,b);
    }
    net_free(&pn);
    net_t out = seq_pack(&f, n);
    seq_free(&f);
    return out;
  }
  return gen_pipelined_sort_rec(0, n);
}

static net_t gen_pairwise(unsigned n)
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
static void bose_pbracket(seq_t *seq, unsigned i, unsigned x, unsigned j, unsigned y){
  if(x==1 && y==1) seq_add(seq, i, j);
  else if(x==1 && y==2){ seq_add(seq, i, j+1); seq_add(seq, i, j); }
  else if(x==2 && y==1){ seq_add(seq, i, j); seq_add(seq, i+1, j); }
  else {
    unsigned a = x/2;
    unsigned b = (x & 1) ? (y/2) : ((y+1)/2);
    bose_pbracket(seq, i, a, j, b);
    bose_pbracket(seq, i+a, x-a, j+b, y-b);
    bose_pbracket(seq, i+a, x-a, j, b);
  }
}
static void bose_pstar(seq_t *seq, unsigned i, unsigned m){
  if(m <= 1) return;
  unsigned a = m/2;
  bose_pstar(seq, i, a);
  bose_pstar(seq, i+a, m-a);
  bose_pbracket(seq, i, a, i+a, m-a);
}
static net_t gen_bose_nelson(unsigned n){
  seq_t seq={0,0,NULL};
  bose_pstar(&seq, 0, n);
  net_t out = seq_pack(&seq, n);
  seq_free(&seq);
  return out;
}
// --- Bitonic mergesort (Batcher second) ---------------------------------
static void bitonic_merge(seq_t *seq, unsigned lo, unsigned n, int dir){
  if(n <= 1) return;
  unsigned k = n/2;
  for(unsigned i=lo; i<lo+k; ++i){
    if(dir) seq_add_directed(seq, i, i+k); // ascending: min->i
    else seq_add_directed(seq, i+k, i); // descending: min->i+k
  }
  bitonic_merge(seq, lo, k, dir);
  bitonic_merge(seq, lo+k, k, dir);
}
static void bitonic_sort_rec(seq_t *seq, unsigned lo, unsigned n, int dir){
  if(n <= 1) return;
  unsigned k = n/2;
  bitonic_sort_rec(seq, lo, k, dir);
  bitonic_sort_rec(seq, lo+k, k, dir ^ 1);
  bitonic_merge(seq, lo, n, dir);
}
static net_t gen_bitonic(unsigned n){
  if(!is_power2(n)) die("bitonic requires n to be a power of two");
  seq_t seq={0,0,NULL};
  bitonic_sort_rec(&seq, 0, n, 1);
  net_t out = seq_pack_directed(&seq, n);
  seq_free(&seq);
  return out;
}
// --- Brick / Odd-Even Transposition (periodic) ----------------------------
static net_t gen_brick(unsigned n){
  net_t out={0,0,NULL};
  if(n < 2) return out;
  for(unsigned layer=0; layer<n; ++layer){
    unsigned start = (layer % 2 == 0) ? 0 : 1;
    for(unsigned i=start; i+1<n; i+=2) net_add(&out, layer, i, i+1);
  }
  return out;
}
// --- Green filter (Green 1969) for n=16 ----------------------------------
static const pair_t green16[4][8] = {
  {{0,5},{1,4},{2,12},{3,13},{6,7},{8,9},{10,15},{11,14}},
  {{0,2},{1,10},{3,6},{4,7},{5,14},{8,11},{9,12},{13,15}},
  {{0,8},{1,3},{2,11},{4,13},{5,9},{6,10},{7,15},{12,14}},
  {{0,1},{2,4},{3,8},{5,6},{7,12},{9,10},{11,13},{14,15}}
};
static net_t gen_green(unsigned n){
  net_t out={0,0,NULL};
  if(n==16){
    for(unsigned l=0;l<4;++l) for(unsigned k=0;k<8;++k) net_add(&out, l, green16[l][k].left, green16[l][k].right);
    return out;
  }
  if(n < 16){
    net_t g16 = gen_green(16);
    seq_t f={0,0,NULL};
    for(unsigned l=0;l<g16.layers;++l) for(unsigned k=0;k<g16.layer[l].count;++k){
      unsigned a=g16.layer[l].pair[k].left, b=g16.layer[l].pair[k].right;
      if(a < n && b < n) seq_add(&f,a,b);
    }
    net_free(&g16);
    out = seq_pack(&f, n);
    seq_free(&f);
    return out;
  }
  seq_t f={0,0,NULL};
  for(unsigned l=0;l<4;++l) for(unsigned k=0;k<8;++k) seq_add(&f, green16[l][k].left, green16[l][k].right);
  out = seq_pack(&f, n);
  seq_free(&f);
  return out;
}
static unsigned next_vv_size(unsigned n)
{
  unsigned p = 16;
  if(n <= 16) return 16;
  while(p < n){
    if(p > (1u<<30)) die("van-voorhis size too large");
    p = p * p;
  }
  return p;
}

static net_t gen_van_voorhis(unsigned n)
{
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

static int validate_network(const net_t *net, unsigned n)
{
  // Check each layer is matching and wires < n
  for(unsigned l=0;l<net->layers;++l){
    unsigned char *used = calloc(n,1);
    if(!used) die("calloc failed in validate");
    for(unsigned k=0;k<net->layer[l].count;++k){
      unsigned a=net->layer[l].pair[k].left, b=net->layer[l].pair[k].right;
      if(a>=n || b>=n){ free(used); return 0; }
      if(used[a] || used[b]){ free(used); return 0; }
      used[a]=used[b]=1;
    }
    free(used);
  }
  return 1;
}

static void usage(const char *argv0)
{
  fprintf(stderr, "usage: %s [--count|--validate] pairwise|batcher-odd-even|pipelined-mergesort|van-voorhis|bose-nelson|bitonic|brick|green n\n", argv0);
}

int main(int argc, char **argv)
{
  const char *algo;
  unsigned n;
  int count_only = 0;
  int validate_only = 0;
  net_t net;

  if(argc == 4 && strcmp(argv[1], "--count") == 0)
  {
    count_only = 1;
    ++argv;
    --argc;
  }
  if(argc == 4 && strcmp(argv[1], "--validate") == 0)
  {
    validate_only = 1;
    ++argv;
    --argc;
  }

  if(argc != 3)
  {
    usage(argv[0]);
    return 1;
  }

  algo = argv[1];
  n = (unsigned)strtoul(argv[2], NULL, 10);

  if(strcmp(algo, "pairwise") == 0)
    net = gen_pairwise(n);
  else if(strcmp(algo, "batcher-odd-even") == 0 || strcmp(algo, "batcher") == 0)
    net = gen_batcher_odd_even(n);
  else if(strcmp(algo, "pipelined-mergesort") == 0 || strcmp(algo, "pipeline-merge") == 0)
    net = gen_pipelined_mergesort(n);
  else if(strcmp(algo, "van-voorhis") == 0 || strcmp(algo, "vv") == 0)
    net = gen_van_voorhis(n);
  else if(strcmp(algo, "bose-nelson") == 0 || strcmp(algo, "bose") == 0)
    net = gen_bose_nelson(n);
  else if(strcmp(algo, "bitonic") == 0)
    net = gen_bitonic(n);
  else if(strcmp(algo, "brick") == 0)
    net = gen_brick(n);
  else if(strcmp(algo, "green") == 0)
    net = gen_green(n);
  else
  {
    usage(argv[0]);
    return 1;
  }

  if(count_only)
    printf("%llu %u\n", (unsigned long long)net_cmp_count(&net), net.layers);
  else if(validate_only){
    int ok = validate_network(&net, n);
    printf("%s: %llu comps, %u layers, %s\n", algo, (unsigned long long)net_cmp_count(&net), net.layers, ok?"VALID":"INVALID");
    net_free(&net);
    return ok?0:2;
  }
  else
    net_print(&net);

  net_free(&net);
  return 0;
}
