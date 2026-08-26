/*
 * optimal sorting network search
 */
#include "sorter.h"
#include <string.h>

#define HELP \
  "usage:\n" \
  "  sorter proof [wires]          read network and prove it\n" \
  "  sorter search [wires] [extra] read seed network and search extension, extra is capped at 1\n"

typedef struct {
  cmp_t first;
  cmp_t second;
  uint64_t mask;
  uint8_t count;
  size score;
} orbit_t;

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
  return 1ULL << index;
}

size network_max_wire(const network_t *net)
{
  size i;
  size j;
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
  size i;
  size j;

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
    if(layer->pairs[i].left == left || layer->pairs[i].left == right ||
       layer->pairs[i].right == left || layer->pairs[i].right == right)
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
  size i;
  size j;

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
  size i;
  size j;

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
    unsigned a;
    unsigned b;

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

static unsigned orbit_score(size wires, size pivot, const orbit_t *orbit)
{
  unsigned score = 0;
  size i;

  for(i = 0; i < orbit->count; ++i)
  {
    const cmp_t *cmp = (i == 0) ? &orbit->first : &orbit->second;
    size left = cmp->left;
    size right = cmp->right;
    if(left > pivot)
      score += (unsigned)(left - pivot);
    else
      score += (unsigned)(pivot - left);
    if(right > pivot)
      score += (unsigned)(right - pivot);
    else
      score += (unsigned)(pivot - right);
  }
  (void)wires;
  return score;
}

static int orbit_cmp(const void *pa, const void *pb)
{
  const orbit_t *a = pa;
  const orbit_t *b = pb;

  if(a->score < b->score)
    return -1;
  if(a->score > b->score)
    return 1;
  if(cmp_pair(&a->first, &b->first) != 0)
    return cmp_pair(&a->first, &b->first);
  return cmp_pair(&a->second, &b->second);
}

static int orbit_make(size wires, size pivot, size left, size right, orbit_t *orbit)
{
  cmp_t a;
  cmp_t b;
  size mirror = wires - 1 - pivot;
  uint64_t mask = 0;

  if(left > right)
  {
    size tmp = left;
    left = right;
    right = tmp;
  }

  if(!(left == pivot || right == pivot || left == mirror || right == mirror))
    return 0;

  a.left = left;
  a.right = right;
  b.left = (size)(wires - 1 - right);
  b.right = (size)(wires - 1 - left);

  if(cmp_pair(&a, &b) > 0)
    return 0;

  mask |= bit_mask(a.left);
  mask |= bit_mask(a.right);
  mask |= bit_mask(b.left);
  mask |= bit_mask(b.right);

  orbit->first = a;
  orbit->second = b;
  orbit->mask = mask;
  orbit->count = cmp_pair(&a, &b) == 0 ? 1 : 2;
  orbit->score = orbit_score(wires, pivot, orbit);
  return 1;
}

static orbit_t *build_orbits(const network_t *net, size pivot, size *count)
{
  size wires = net->wires;
  size cap = wires * 2 + 1;
  orbit_t *list = calloc(cap, sizeof *list);
  size n = 0;
  size left;
  size right;

  if(list == NULL)
    return NULL;

  for(left = 0; left < wires; ++left)
    for(right = left + 1; right < wires; ++right)
    {
      orbit_t orbit;
      if(!orbit_make(wires, pivot, left, right, &orbit))
        continue;
      if(network_has_cmp(net, orbit.first.left, orbit.first.right) &&
         (orbit.count == 1 || network_has_cmp(net, orbit.second.left, orbit.second.right)))
        continue;
      list[n++] = orbit;
    }

  qsort(list, n, sizeof *list, orbit_cmp);
  *count = n;
  return list;
}

static uint64_t *build_layer_masks(const network_t *net)
{
  size i;
  size j;
  uint64_t *mask = calloc(net->layers ? net->layers : 1, sizeof *mask);

  if(mask == NULL)
    return NULL;

  for(i = 0; i < net->layers; ++i)
    for(j = 0; j < net->layer[i].count; ++j)
    {
      const cmp_t *cmp = &net->layer[i].pairs[j];
      mask[i] |= bit_mask(cmp->left);
      mask[i] |= bit_mask(cmp->right);
    }

  return mask;
}

static size orbit_place(network_t *net, size layer_idx, const orbit_t *orbit)
{
  size added = 0;

  if(!network_add_cmp(net, layer_idx, orbit->first.left, orbit->first.right))
    return 0;
  ++added;
  if(orbit->count == 2)
  {
    if(!network_add_cmp(net, layer_idx, orbit->second.left, orbit->second.right))
      return 0;
    ++added;
  }
  return added;
}

static void orbit_unplace(network_t *net, size layer_idx, size added)
{
  layer_t *layer;

  if(net == NULL || layer_idx >= net->layers || added == 0)
    return;

  layer = &net->layer[layer_idx];
  if(layer->count >= added)
    layer->count -= added;
}

static int search_orbits(size idx, const orbit_t *cand, size cand_count, network_t *work, uint64_t *masks)
{
  size layer;

  if(network_proof(work))
    return 1;

  if(idx >= cand_count)
    return 0;

  if(cand[idx].count == 1)
  {
    for(layer = 0; layer < work->layers; ++layer)
    {
      size added;
      uint64_t mask = bit_mask(cand[idx].first.left) | bit_mask(cand[idx].first.right);

      if(masks[layer] & mask)
        continue;
      added = orbit_place(work, layer, &cand[idx]);
      if(added == 0)
        continue;
      masks[layer] |= mask;
      if(search_orbits(idx + 1, cand, cand_count, work, masks))
        return 1;
      masks[layer] &= ~mask;
      orbit_unplace(work, layer, added);
    }
  }
  else
  {
    size layer2;
    uint64_t mask1 = bit_mask(cand[idx].first.left) | bit_mask(cand[idx].first.right);
    uint64_t mask2 = bit_mask(cand[idx].second.left) | bit_mask(cand[idx].second.right);

    for(layer = 0; layer < work->layers; ++layer)
    {
      if(masks[layer] & mask1)
        continue;
      if(!network_add_cmp(work, layer, cand[idx].first.left, cand[idx].first.right))
        continue;
      masks[layer] |= mask1;

      for(layer2 = 0; layer2 < work->layers; ++layer2)
      {
        if(masks[layer2] & mask2)
          continue;
        if(!network_add_cmp(work, layer2, cand[idx].second.left, cand[idx].second.right))
          continue;
        masks[layer2] |= mask2;
        if(search_orbits(idx + 1, cand, cand_count, work, masks))
          return 1;
        masks[layer2] &= ~mask2;
        orbit_unplace(work, layer2, 1);
      }

      masks[layer] &= ~mask1;
      orbit_unplace(work, layer, 1);
    }
  }

  return search_orbits(idx + 1, cand, cand_count, work, masks);
}

network_t *search_extension(const network_t *seed, size target_wires, size max_extra_layers)
{
  size extra;

  if(seed == NULL || target_wires <= seed->wires)
    return NULL;

  for(extra = 0; extra <= max_extra_layers; ++extra)
  {
    size insert;
    for(insert = 0; insert < target_wires; ++insert)
    {
      size layer_pos;
      network_t *base = network_clone(seed);

      if(base == NULL)
        return NULL;
      if(!network_insert_wire(base, insert))
      {
        network_free(base);
        return NULL;
      }

      if(base->wires != target_wires)
      {
        network_free(base);
        continue;
      }

      for(layer_pos = 0; layer_pos <= (extra ? base->layers : 0); ++layer_pos)
      {
        network_t *work = network_clone(base);
        orbit_t *cand;
        size cand_count = 0;
        uint64_t *masks;

        if(work == NULL)
        {
          network_free(base);
          return NULL;
        }

        if(extra && !network_insert_empty_layer(work, layer_pos))
        {
          network_free(work);
          network_free(base);
          return NULL;
        }

        cand = build_orbits(work, insert, &cand_count);
        if(cand == NULL && cand_count != 0)
        {
          network_free(work);
          network_free(base);
          return NULL;
        }
        masks = build_layer_masks(work);
        if(masks == NULL)
        {
          free(cand);
          network_free(work);
          network_free(base);
          return NULL;
        }

        fprintf(stderr, "try insert=%u extra=%u layer_pos=%u orbits=%u\n",
                (unsigned)insert, (unsigned)extra, (unsigned)(extra ? layer_pos : 0),
                (unsigned)cand_count);

        if(search_orbits(0, cand, cand_count, work, masks))
        {
          free(cand);
          free(masks);
          network_free(base);
          return work;
        }

        free(cand);
        free(masks);
        network_free(work);
      }

      network_free(base);
    }
  }

  return NULL;
}

static int run_proof_cmd(size target_wires, int have_target)
{
  network_t *net = network_load(stdin);
  int ok;

  if(net == NULL)
  {
    fprintf(stderr, "cannot read network\n");
    return 1;
  }

  if(have_target && target_wires != 0 && target_wires != net->wires)
  {
    if(net->wires == 0 && target_wires != 0)
      net->wires = target_wires;
    else
      fprintf(stderr, "warning: input network has %u wires, requested %u\n",
              (unsigned)net->wires, (unsigned)target_wires);
  }

  ok = network_proof(net);
  fprintf(stderr, "%s\n", ok ? "sort OK" : "sort failed");
  network_free(net);
  return ok ? 0 : 1;
}

static int run_search_cmd(size target_wires, int have_target, size max_extra_layers)
{
  network_t *seed = network_load(stdin);
  network_t *found;

  if(seed == NULL)
  {
    fprintf(stderr, "cannot read seed network\n");
    return 1;
  }

  if(!have_target)
    target_wires = seed->wires + 1;
  if(target_wires <= seed->wires)
    target_wires = seed->wires + 1;
  if(seed->wires == 0 && target_wires > 0)
    seed->wires = target_wires - 1;
  if(max_extra_layers > 1)
  {
    fprintf(stderr, "extra layers capped at 1\n");
    max_extra_layers = 1;
  }

  found = search_extension(seed, target_wires, max_extra_layers);
  network_free(seed);

  if(found == NULL)
  {
    fprintf(stderr, "no extension found\n");
    return 1;
  }

  if(!network_write(stdout, found))
  {
    network_free(found);
    return 1;
  }
  network_free(found);
  return 0;
}

int main(int argc, char **argv)
{
  const char *cmd = argc > 1 ? argv[1] : "help";

  if(strcmp(cmd, "proof") == 0 || strcmp(cmd, "test") == 0)
  {
    size target = 0;
    int have_target = 0;
    if(argc > 2)
    {
      target = (size)strtoul(argv[2], NULL, 10);
      have_target = 1;
    }
    return run_proof_cmd(target, have_target);
  }

  if(strcmp(cmd, "search") == 0 || strcmp(cmd, "bld") == 0)
  {
    size target = 0;
    size extra = 1;
    int have_target = 0;

    if(argc > 2)
    {
      target = (size)strtoul(argv[2], NULL, 10);
      have_target = 1;
    }
    if(argc > 3)
      extra = (size)strtoul(argv[3], NULL, 10);
    return run_search_cmd(target, have_target, extra);
  }

  fputs(HELP, stdout);
  return 0;
}
