#include "gen_common.h"
#include <string.h>

void die(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

int is_power2(unsigned n) { return n && ((n & (n - 1)) == 0); }

unsigned next_pow2(unsigned n)
{
  if (n == 0)
    return 1;
  if (n > 65536)
    die("gen: n too large (max 65536)");
  unsigned p = 1;
  while (p < n)
  {
    if (p > (1u << 30))
      die("gen: next_pow2 overflow");
    p <<= 1;
  }
  return p;
}

void layer_free(layer_t *layer)
{
  if (layer == NULL)
    return;
  free(layer->pair);
  layer->pair = NULL;
  layer->count = 0;
  layer->cap = 0;
}

void net_free(net_t *net)
{
  unsigned i;

  if (net == NULL)
    return;
  for (i = 0; i < net->layers; ++i)
    layer_free(&net->layer[i]);
  free(net->layer);
  net->layer = NULL;
  net->layers = 0;
  net->cap = 0;
}

void seq_free(seq_t *seq)
{
  if (seq == NULL)
    return;
  free(seq->pair);
  seq->pair = NULL;
  seq->count = 0;
  seq->cap = 0;
}

void net_reserve_layers(net_t *net, unsigned layers)
{
  layer_t *layer;
  unsigned i;

  if (layers <= net->cap)
    return;

  layer = realloc(net->layer, layers * sizeof *layer);
  if (layer == NULL)
    die("cannot allocate layers");

  for (i = net->cap; i < layers; ++i)
    memset(&layer[i], 0, sizeof layer[i]);

  net->layer = layer;
  net->cap = layers;
}

void net_ensure_layer(net_t *net, unsigned idx)
{
  if (idx >= net->cap)
    net_reserve_layers(net, idx + 1);
  if (idx >= net->layers)
    net->layers = idx + 1;
}

void layer_add(layer_t *layer, unsigned left, unsigned right)
{
  pair_t *pair;

  if (left == right)
    return;
  if (layer->count == layer->cap)
  {
    unsigned cap = layer->cap ? layer->cap * 2 : 8;
    pair = realloc(layer->pair, cap * sizeof *pair);
    if (pair == NULL)
      die("cannot allocate comparators");
    layer->pair = pair;
    layer->cap = cap;
  }

  layer->pair[layer->count].left = left;
  layer->pair[layer->count].right = right;
  ++layer->count;
}

void layer_add_directed(layer_t *layer, unsigned left, unsigned right)
{
  // Do not normalize, keep direction
  if (left == right)
    return;
  pair_t *pair;
  if (layer->count == layer->cap)
  {
    unsigned cap = layer->cap ? layer->cap * 2 : 8;
    pair = realloc(layer->pair, cap * sizeof *pair);
    if (pair == NULL)
      die("cannot allocate comparators");
    layer->pair = pair;
    layer->cap = cap;
  }
  layer->pair[layer->count].left = left;
  layer->pair[layer->count].right = right;
  ++layer->count;
}

void seq_reserve(seq_t *seq, unsigned cap)
{
  if (cap <= seq->cap)
    return;
  pair_t *p = realloc(seq->pair, cap * sizeof *p);
  if (!p)
    die("cannot reserve seq");
  seq->pair = p;
  seq->cap = cap;
}

void seq_add(seq_t *seq, unsigned left, unsigned right)
{
  pair_t *pair;

  if (left == right)
    return;
  if (left > right)
  {
    unsigned tmp = left;
    left = right;
    right = tmp;
  }

  if (seq->count == seq->cap)
  {
    unsigned cap = seq->cap ? seq->cap * 2 : 1024;
    pair = realloc(seq->pair, cap * sizeof *pair);
    if (pair == NULL)
      die("cannot allocate comparator sequence");
    seq->pair = pair;
    seq->cap = cap;
  }

  seq->pair[seq->count].left = left;
  seq->pair[seq->count].right = right;
  ++seq->count;
}

void seq_add_directed(seq_t *seq, unsigned left, unsigned right)
{
  // For bitonic: keep direction (left gets min), do not normalize
  if (left == right)
    return;
  pair_t *pair;
  if (seq->count == seq->cap)
  {
    unsigned cap = seq->cap ? seq->cap * 2 : 1024;
    pair = realloc(seq->pair, cap * sizeof *pair);
    if (pair == NULL)
      die("cannot allocate comparator sequence");
    seq->pair = pair;
    seq->cap = cap;
  }
  seq->pair[seq->count].left = left;
  seq->pair[seq->count].right = right;
  ++seq->count;
}

void net_add(net_t *net, unsigned layer, unsigned left, unsigned right)
{
  net_ensure_layer(net, layer);
  layer_add(&net->layer[layer], left, right);
}
void net_add_directed(net_t *net, unsigned layer, unsigned left, unsigned right)
{
  net_ensure_layer(net, layer);
  layer_add_directed(&net->layer[layer], left, right);
}

void net_append_layer(net_t *dst, const layer_t *src)
{
  unsigned i;
  unsigned layer = dst->layers;

  net_ensure_layer(dst, layer);
  for (i = 0; i < src->count; ++i)
    layer_add(&dst->layer[layer], src->pair[i].left, src->pair[i].right);
}

void net_concat(net_t *dst, const net_t *src)
{
  unsigned i;

  for (i = 0; i < src->layers; ++i)
    net_append_layer(dst, &src->layer[i]);
}

net_t net_parallel(net_t *a, net_t *b)
{
  net_t out = {0, 0, NULL};
  unsigned i;
  unsigned layers = a->layers > b->layers ? a->layers : b->layers;

  for (i = 0; i < layers; ++i)
  {
    unsigned j;
    net_ensure_layer(&out, i);
    if (i < a->layers)
      for (j = 0; j < a->layer[i].count; ++j)
        layer_add(&out.layer[i], a->layer[i].pair[j].left, a->layer[i].pair[j].right);
    if (i < b->layers)
      for (j = 0; j < b->layer[i].count; ++j)
        layer_add(&out.layer[i], b->layer[i].pair[j].left, b->layer[i].pair[j].right);
  }

  net_free(a);
  net_free(b);
  return out;
}

void net_print(const net_t *net)
{
  unsigned i;
  unsigned j;

  for (i = 0; i < net->layers; ++i)
  {
    for (j = 0; j < net->layer[i].count; ++j)
      printf("%u %u\n", net->layer[i].pair[j].left, net->layer[i].pair[j].right);
    if (i + 1 < net->layers)
      putchar('\n');
  }
}

uint64_t net_cmp_count(const net_t *net)
{
  uint64_t count = 0;
  unsigned i;

  for (i = 0; i < net->layers; ++i)
    count += net->layer[i].count;

  return count;
}

net_t seq_pack(const seq_t *seq, unsigned wires)
{
  net_t out = {0, 0, NULL};
  unsigned *ready;
  unsigned i;

  // ready[w] = earliest layer where wire w is free (greedy layer packing)
  ready = calloc(wires, sizeof *ready);
  if (ready == NULL)
    die("cannot allocate wire readiness table");

  for (i = 0; i < seq->count; ++i)
  {
    unsigned left = seq->pair[i].left;
    unsigned right = seq->pair[i].right;
    unsigned layer = ready[left] > ready[right]
                         ? ready[left]
                         : ready[right]; // earliest layer where both wires free

    net_add(&out, layer, left, right);
    ready[left] = layer + 1;
    ready[right] = layer + 1;
  }

  free(ready);
  return out;
}

net_t seq_pack_directed(const seq_t *seq, unsigned wires)
{
  net_t out = {0, 0, NULL};
  unsigned *ready = calloc(wires, sizeof *ready);
  if (!ready)
    die("cannot allocate readiness");
  for (unsigned i = 0; i < seq->count; ++i)
  {
    unsigned left = seq->pair[i].left, right = seq->pair[i].right;
    unsigned layer = ready[left] > ready[right] ? ready[left] : ready[right];
    net_add_directed(&out, layer, left, right);
    ready[left] = ready[right] = layer + 1;
  }
  free(ready);
  return out;
}

unsigned *map_subset(const unsigned *map, unsigned rows, unsigned cols,
                     unsigned row_start, unsigned col_start)
{
  unsigned sub_rows = rows / 2;
  unsigned sub_cols = cols / 2;
  unsigned *sub;
  unsigned row;
  unsigned col;

  sub = malloc(sub_rows * sub_cols * sizeof *sub);
  if (sub == NULL)
    die("cannot allocate subnetwork map");

  for (row = 0; row < sub_rows; ++row)
    for (col = 0; col < sub_cols; ++col)
      sub[row * sub_cols + col] =
          map[(row * 2 + row_start) * cols + (col * 2 + col_start)];

  return sub;
}

void gen_vv_f_square(seq_t *seq, const unsigned *map, unsigned rows, unsigned r);
void gen_vv_delta(seq_t *seq, const unsigned *map, unsigned rows, unsigned r);

void gen_vv_h(seq_t *seq, const unsigned *map, unsigned rows, unsigned r)
{
  unsigned cols = 1u << r;
  unsigned half = cols / 2;
  unsigned row;
  unsigned s;

  for (row = 0; row + 1 < rows; ++row)
    for (s = 0; s < half; ++s)
      seq_add(seq, map[row * cols + (2 * s + 1)], map[(row + 1) * cols + (2 * s)]);

  if (r == 1)
    return;

  for (row = 0; row + 1 < rows; ++row)
    for (s = 0; s < half; ++s)
      seq_add(seq, map[row * cols + (half + s)], map[(row + 1) * cols + s]);

  gen_vv_delta(seq, map, rows * 2, r - 1);
}

void gen_vv_delta(seq_t *seq, const unsigned *map, unsigned rows, unsigned r)
{
  unsigned cols = 1u << r;
  unsigned half = cols / 2;
  unsigned row;
  unsigned s;

  for (row = 1; row + 2 < rows; ++row)
    for (s = 0; s < half; ++s)
      seq_add(seq, map[row * cols + (2 * s + 1)], map[(row + 1) * cols + (2 * s)]);

  if (r == 1)
    return;

  for (row = 0; row + 1 < rows; ++row)
    for (s = 0; s < half; ++s)
      seq_add(seq, map[row * cols + (half + s)], map[(row + 1) * cols + s]);

  gen_vv_delta(seq, map, rows * 2, r - 1);
}

void gen_vv_f_square(seq_t *seq, const unsigned *map, unsigned rows, unsigned r)
{
  unsigned cols = 1u << r;
  unsigned row;
  unsigned *sub;

  if (r == 1)
  {
    for (row = 0; row + 1 < rows; ++row)
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

void gen_vv_sort_seq(seq_t *seq, const unsigned *map, unsigned m)
{
  unsigned side;
  unsigned row;
  unsigned col;
  unsigned *sub;

  if (m == 0)
    return;
  if (m == 1)
  {
    seq_add(seq, map[0], map[1]);
    return;
  }
  if (m % 2 != 0)
    die("van-voorhis currently supports n=2^(2^k)");

  side = 1u << (m / 2);

  for (row = 0; row < side; ++row)
    gen_vv_sort_seq(seq, &map[row * side], m / 2);

  sub = malloc(side * sizeof *sub);
  if (sub == NULL)
    die("cannot allocate column map");

  for (col = 0; col < side; ++col)
  {
    for (row = 0; row < side; ++row)
      sub[row] = map[row * side + col];
    gen_vv_sort_seq(seq, sub, m / 2);
  }

  free(sub);
  gen_vv_f_square(seq, map, side, m / 2);
}
