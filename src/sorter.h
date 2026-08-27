/*
 * sorting network helpers
 *
 * copyright 2024-10-31, dmitri bortoq, mit license
 *
 * Module layout:
 *   network.c     - core storage/I/O/matching (network_t, layer_t)
 *   search.c  - symmetric search (orbits, DFS packing)
 *   main.c        - CLI (proof/search)
 *   proof.c       - heuristic proof, proof_exp.c - strict zero-one
 *   gen_network.c - Batcher/pairwise/Van Voorhis generators
 *   layer_perm.c  - permutation tester (standalone)
 */
#ifndef SORTER_H_
#define SORTER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SORTER_MAX_WIRES 64 /* search uses 64-bit masks */
#define PROOF_MAX_WIRES 30  /* 2^n exhaustive */

typedef uint32_t size;

typedef struct
{
  size left;
  size right;
} cmp_t;

typedef struct
{
  size count;
  size cap;
  cmp_t *pairs;
} layer_t;

typedef struct
{
  size wires;
  size layers;
  size cap;
  layer_t *layer;
} network_t;

network_t *network_new(size wires, size layers);
network_t *network_clone(const network_t *src);
void network_free(network_t *net);

int network_reserve_layers(network_t *net, size layers);
int network_append_layer(network_t *net);
int network_add_cmp(network_t *net, size layer_idx, size left, size right);
int network_insert_wire(network_t *net, size pos);

size network_max_wire(const network_t *net);
int network_has_cmp(const network_t *net, size left, size right);

network_t *network_load(FILE *in);
int network_write(FILE *out, const network_t *net);

int network_sort(const network_t *net, size *data);
int network_proof(const network_t *net);

network_t *search_extension(const network_t *seed, size target_wires,
                            size max_extra_layers);

int run_proof_cmd(size target_wires, int have_target);
int run_search_cmd(size target_wires, int have_target, size max_extra_layers);

// internal helpers exposed for modular build (network.c -> sorter.c)
int network_insert_empty_layer(network_t *net, size pos);
uint64_t bit_mask(size pos);
int cmp_pair(const cmp_t *a, const cmp_t *b);

#define HELP                                                                             \
  "usage:\n"                                                                             \
  "  sorter proof [wires]          read network and prove it (strict proof: n < 20)\n"   \
  "  sorter search [wires] [extra] read seed network and search extension, extra is "    \
  "capped at 1\n"                                                                        \
  "  limits: search supports n <= 64 (64-bit masks), proof_exp supports n < 20\n"

#endif /* SORTER_H_ */
