/*
 * Sorting network generators.
 *
 * Output format:
 *   one comparator "i j" per line
 *   empty line between layers
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
  if(left > right)
  {
    unsigned tmp = left;
    left = right;
    right = tmp;
  }

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

static void net_add(net_t *net, unsigned layer, unsigned left, unsigned right)
{
  net_ensure_layer(net, layer);
  layer_add(&net->layer[layer], left, right);
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

  ready = calloc(wires, sizeof *ready);
  if(ready == NULL)
    die("cannot allocate wire readiness table");

  for(i = 0; i < seq->count; ++i)
  {
    unsigned left = seq->pair[i].left;
    unsigned right = seq->pair[i].right;
    unsigned layer = ready[left] > ready[right] ? ready[left] : ready[right];

    net_add(&out, layer, left, right);
    ready[left] = layer + 1;
    ready[right] = layer + 1;
  }

  free(ready);
  return out;
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
  if(!is_power2(n))
    die("batcher-odd-even requires n to be a power of two");
  return gen_batcher_sort_rec(0, n);
}

static net_t gen_pipelined_mergesort(unsigned n)
{
  if(!is_power2(n))
    die("pipelined-mergesort requires n to be a power of two");
  return gen_batcher_sort_rec(0, n);
}

static net_t gen_pairwise(unsigned n)
{
  net_t out = {0, 0, NULL};
  unsigned p;

  if(!is_power2(n))
    die("pairwise requires n to be a power of two");

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

static net_t gen_van_voorhis(unsigned n)
{
  seq_t seq = {0, 0, NULL};
  unsigned *map;
  unsigned m = 0;
  unsigned i;
  unsigned p;
  net_t out;

  if(!is_power2(n))
    die("van-voorhis requires n to be a power of two");

  for(p = n; p > 1; p >>= 1)
    ++m;
  if(m == 0 || (m & (m - 1)) != 0)
    die("van-voorhis currently supports n=2^(2^k)");

  map = malloc(n * sizeof *map);
  if(map == NULL)
    die("cannot allocate top-level map");

  for(i = 0; i < n; ++i)
    map[i] = i;

  gen_vv_sort_seq(&seq, map, m);
  out = seq_pack(&seq, n);

  free(map);
  seq_free(&seq);
  return out;
}

static void usage(const char *argv0)
{
  fprintf(stderr, "usage: %s [--count] pairwise|batcher-odd-even|pipelined-mergesort|van-voorhis n\n", argv0);
}

int main(int argc, char **argv)
{
  const char *algo;
  unsigned n;
  int count_only = 0;
  net_t net;

  if(argc == 4 && strcmp(argv[1], "--count") == 0)
  {
    count_only = 1;
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
  else
  {
    usage(argv[0]);
    return 1;
  }

  if(count_only)
    printf("%llu %u\n", (unsigned long long)net_cmp_count(&net), net.layers);
  else
    net_print(&net);

  net_free(&net);
  return 0;
}
