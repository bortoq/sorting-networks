#ifndef GEN_COMMON_H
#define GEN_COMMON_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct
{
  unsigned left;
  unsigned right;
} pair_t;
typedef struct
{
  unsigned count;
  unsigned cap;
  pair_t *pair;
} layer_t;
typedef struct
{
  unsigned layers;
  unsigned cap;
  layer_t *layer;
} net_t;
typedef struct
{
  unsigned count;
  unsigned cap;
  pair_t *pair;
} seq_t;
void die(const char *msg);
int is_power2(unsigned n);
unsigned next_pow2(unsigned n);
unsigned next_vv_size(unsigned n);
void layer_free(layer_t *layer);
void net_free(net_t *net);
void seq_free(seq_t *seq);
void net_reserve_layers(net_t *net, unsigned layers);
void net_ensure_layer(net_t *net, unsigned idx);
void layer_add(layer_t *layer, unsigned left, unsigned right);
void layer_add_directed(layer_t *layer, unsigned left, unsigned right);
void seq_reserve(seq_t *seq, unsigned cap);
void seq_add(seq_t *seq, unsigned left, unsigned right);
void seq_add_directed(seq_t *seq, unsigned left, unsigned right);
void net_add(net_t *net, unsigned layer, unsigned left, unsigned right);
void net_add_directed(net_t *net, unsigned layer, unsigned left, unsigned right);
void net_append_layer(net_t *dst, const layer_t *src);
void net_concat(net_t *dst, const net_t *src);
net_t net_parallel(net_t *a, net_t *b);
void net_print(const net_t *net);
uint64_t net_cmp_count(const net_t *net);
net_t seq_pack(const seq_t *seq, unsigned wires);
net_t seq_pack_directed(const seq_t *seq, unsigned wires);
int validate_network(const net_t *net, unsigned n);

// Van Voorhis helpers (internal, exposed for gen_vanvoorhis)
unsigned *map_subset(const unsigned *map, unsigned rows, unsigned cols,
                     unsigned row_start, unsigned col_start);
void gen_vv_h(seq_t *seq, const unsigned *map, unsigned rows, unsigned r);
void gen_vv_delta(seq_t *seq, const unsigned *map, unsigned rows, unsigned r);
void gen_vv_f_square(seq_t *seq, const unsigned *map, unsigned rows, unsigned r);
void gen_vv_sort_seq(seq_t *seq, const unsigned *map, unsigned m);

#endif
