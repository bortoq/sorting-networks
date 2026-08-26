/*
 * sorting network helpers
 *
 * copyright 2024-10-31, dmitri bortoq, mit license
 */
#ifndef SORTER_H_
#define SORTER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint16_t size;

typedef struct {
  size left;
  size right;
} cmp_t;

typedef struct {
  size count;
  size cap;
  cmp_t *pairs;
} layer_t;

typedef struct {
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

network_t *search_extension(const network_t *seed, size target_wires, size max_extra_layers);

#endif /* SORTER_H_ */
