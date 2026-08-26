/*
 * sorting network core - storage, I/O, comparators
 */
#include "sorter.h"
#include <string.h>

static void layer_free(layer_t *layer)
{
  if(layer == NULL)
    return;
  free(layer->pairs);
  layer->pairs = NULL;
  layer->count = 0;
  layer->cap = 0;
}

static int layer_reserve(layer_t *layer, size cap)
{
  cmp_t *pairs;
  if(layer == NULL)
    return 0;
  if(cap <= layer->cap)
    return 1;
  pairs = realloc(layer->pairs, cap * sizeof *pairs);
  if(pairs == NULL)
    return 0;
  layer->pairs = pairs;
  layer->cap = cap;
  return 1;
}

network_t *network_new(size wires, size layers)
{
  network_t *net = calloc(1, sizeof *net);
  if(net == NULL)
    return NULL;
  net->wires = wires;
  if(layers)
  {
    net->layer = calloc(layers, sizeof *net->layer);
    if(net->layer == NULL)
    {
      free(net);
      return NULL;
    }
    net->cap = layers;
  }
  return net;
}

network_t *network_clone(const network_t *src)
{
  size i;
  network_t *dst;
  if(src == NULL)
    return NULL;
  dst = network_new(src->wires, src->cap);
  if(dst == NULL)
    return NULL;
  dst->layers = src->layers;
  for(i = 0; i < src->layers; ++i)
  {
    const layer_t *sl = &src->layer[i];
    layer_t *dl = &dst->layer[i];
    if(sl->count == 0)
      continue;
    if(!layer_reserve(dl, sl->count))
    {
      network_free(dst);
      return NULL;
    }
    memcpy(dl->pairs, sl->pairs, sl->count * sizeof *sl->pairs);
    dl->count = sl->count;
  }
  return dst;
}

void network_free(network_t *net)
{
  size i;
  if(net == NULL)
    return;
  for(i = 0; i < net->cap; ++i)
    layer_free(&net->layer[i]);
  free(net->layer);
  free(net);
}

int network_reserve_layers(network_t *net, size layers)
{
  layer_t *layer;
  size i;
  if(net == NULL)
    return 0;
  if(layers <= net->cap)
    return 1;
  layer = realloc(net->layer, layers * sizeof *layer);
  if(layer == NULL)
    return 0;
  for(i = net->cap; i < layers; ++i)
    memset(&layer[i], 0, sizeof layer[i]);
  net->layer = layer;
  net->cap = layers;
  return 1;
}

int network_append_layer(network_t *net)
{
  if(net == NULL)
    return 0;
  if(!network_reserve_layers(net, net->layers + 1))
    return 0;
  memset(&net->layer[net->layers], 0, sizeof net->layer[net->layers]);
  ++net->layers;
  return 1;
}

static int network_insert_empty_layer(network_t *net, size pos)
{
  size i;
  if(net == NULL)
    return 0;
  if(pos > net->layers)
    pos = net->layers;
  if(!network_reserve_layers(net, net->layers + 1))
    return 0;
  for(i = net->layers; i > pos; --i)
    net->layer[i] = net->layer[i - 1];
  memset(&net->layer[pos], 0, sizeof net->layer[pos]);
  ++net->layers;
  return 1;
}

static int cmp_pair(const cmp_t *a, const cmp_t *b)
{
  if(a->left < b->left)
    return -1;
  if(a->left > b->left)
    return 1;
  if(a->right < b->right)
    return -1;
  if(a->right > b->right)
    return 1;
  return 0;
}

static uint64_t bit_mask(size index)
{
  if(index >= 64)
    return 0;
  return 1ULL << index;
}

size network_max_wire(const network_t *net)
{
  size i, j;
  size max = 0;
  if(net == NULL)
    return 0;
  for(i = 0; i < net->layers; ++i)
    for(j = 0; j < net->layer[i].count; ++j)
    {
      const cmp_t *cmp = &net->layer[i].pairs[j];
      if(cmp->left + 1 > max)
        max = cmp->left + 1;
      if(cmp->right + 1 > max)
        max = cmp->right + 1;
    }
  if(net->wires > max)
    max = net->wires;
  return max;
}

int network_has_cmp(const network_t *net, size left, size right)
{
  cmp_t needle;
  size i, j;
  if(net == NULL)
    return 0;
  if(left > right)
  {
    size tmp = left;
    left = right;
    right = tmp;
  }
  needle.left = left;
  needle.right = right;
  for(i = 0; i < net->layers; ++i)
    for(j = 0; j < net->layer[i].count; ++j)
      if(cmp_pair(&needle, &net->layer[i].pairs[j]) == 0)
        return 1;
  return 0;
}

int network_add_cmp(network_t *net, size layer_idx, size left, size right)
{
  layer_t *layer;
  size i;
  if(net == NULL)
    return 0;
  if(left == right)
    return 1;
  if(left > right)
  {
    size tmp = left;
    left = right;
    right = tmp;
  }
  while(net->layers <= layer_idx)
  {
    if(!network_append_layer(net))
      return 0;
  }
  layer = &net->layer[layer_idx];
  for(i = 0; i < layer->count; ++i)
    if(layer->pairs[i].left == left || layer->pairs[i].left == right || layer->pairs[i].right == left || layer->pairs[i].right == right)
      return 0;
  if(layer->count == layer->cap && !layer_reserve(layer, layer->cap ? layer->cap * 2 : 4))
    return 0;
  layer->pairs[layer->count].left = left;
  layer->pairs[layer->count].right = right;
  ++layer->count;
  if(right + 1 > net->wires)
    net->wires = right + 1;
  if(left + 1 > net->wires)
    net->wires = left + 1;
  return 1;
}

int network_insert_wire(network_t *net, size pos)
{
  size i, j;
  if(net == NULL)
    return 0;
  for(i = 0; i < net->layers; ++i)
    for(j = 0; j < net->layer[i].count; ++j)
    {
      cmp_t *cmp = &net->layer[i].pairs[j];
      if(cmp->left >= pos)
        ++cmp->left;
      if(cmp->right >= pos)
        ++cmp->right;
    }
  ++net->wires;
  return 1;
}

static void network_print_cmp(FILE *out, const cmp_t *cmp)
{
  fprintf(out, "%u %u\n", (unsigned)cmp->left, (unsigned)cmp->right);
}

int network_write(FILE *out, const network_t *net)
{
  size i, j;
  if(out == NULL || net == NULL)
    return 0;
  for(i = 0; i < net->layers; ++i)
  {
    for(j = 0; j < net->layer[i].count; ++j)
      network_print_cmp(out, &net->layer[i].pairs[j]);
    if(i + 1 < net->layers)
      fputc('\n', out);
  }
  return 1;
}

static char *trim(char *line)
{
  char *p = line;
  char *end;
  while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
    ++p;
  end = p + strlen(p);
  while(end > p && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n'))
    --end;
  *end = '\0';
  return p;
}

network_t *network_load(FILE *in)
{
  char line[256];
  network_t *net = network_new(0, 0);
  int active = 0;
  if(net == NULL || in == NULL)
  {
    network_free(net);
    return NULL;
  }
  while(fgets(line, sizeof line, in) != NULL)
  {
    char *p = trim(line);
    unsigned a, b;
    if(*p == '\0' || *p == '#')
    {
      active = 0;
      continue;
    }
    if(sscanf(p, "%u %u", &a, &b) != 2)
    {
      network_free(net);
      return NULL;
    }
    if(!active)
    {
      if(!network_append_layer(net))
      {
        network_free(net);
        return NULL;
      }
      active = 1;
    }
    if(!network_add_cmp(net, net->layers - 1, (size)a, (size)b))
    {
      network_free(net);
      return NULL;
    }
  }
  return net;
}

/* internal helpers exposed for search.c via header */
int network_insert_empty_layer_pub(network_t *net, size pos) { return network_insert_empty_layer(net, pos); }
uint64_t bit_mask_pub(size index) { return bit_mask(index); }
int cmp_pair_pub(const cmp_t *a, const cmp_t *b) { return cmp_pair(a,b); }
