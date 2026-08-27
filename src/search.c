/*
 * sorting network search - symmetric incremental extension n-1 -> n
 *
 * Theory:
 *   Empirical: near-optimal networks are almost symmetric. We exploit this
 *   to prune search. Take optimal network for n-1, insert new wire at
 *   each position p (0..n-1). Its mirror is m = n-1-p.
 *   Comparator (a,b) must appear with mirror (n-1-b, n-1-a) as one orbit
 *   (1 comparator if self-mirrored, else 2). Enumerate only orbits incident
 *   to p or m, sort by distance to p, DFS-pack into existing layers first,
 *   then try one fresh empty layer at each insertion point. Verify with
 *   proof after each full placement.
 *
 * Diagram n=7, p=2 (m=4), orbit {0,2} <-> {4,6}:
 *
 *   wires: 0 1 2 3 4 5 6   (p=2)
 *   mirror:6 5 4 3 2 1 0   (m=4)
 *   orbit :  0-2  <->  4-6   added together
 *            self-mirrored: {3,3}? no, {1,5} is self-mirrored? 1<->5
 *
 * Search order:
 *   try extra=0 (pack into existing layers) for all p,
 *   then extra=1 (one new layer at each layer_pos)
 *   backtrack: place orbit -> recurse -> unplace
 *
 * Limits: n <= 64 (uint64_t masks), max_extra_layers=1 (capped),
 *   only incident comparators -> heuristic, not exhaustive.
 *   Verification: proof_exp.c (zero-one) or proof.c (heuristic).
 *
 * Refs: Codish et al. "Sorting Networks: to the End Game", Bundala-Codish
 */
#include "sorter.h"
#include <string.h>
#include <stdio.h>

/* forward from network.c */
int network_insert_empty_layer_pub(network_t *net, size pos);
uint64_t bit_mask_pub(size index);
int cmp_pair_pub(const cmp_t *a, const cmp_t *b);

typedef struct {
  cmp_t first;
  cmp_t second;
  uint64_t mask;
  uint8_t count;
  size score;
} orbit_t;

static unsigned orbit_score(size wires, size pivot, const orbit_t *orbit)
{
  // Heuristic: prefer orbits close to pivot (short wires), so DFS finds packing early

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
  if(cmp_pair_pub(&a->first, &b->first) != 0)
    return cmp_pair_pub(&a->first, &b->first);
  return cmp_pair_pub(&a->second, &b->second);
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
    return 0; // orbit must touch inserted wire or its mirror

  a.left = left;
  a.right = right;
  b.left = (size)(wires - 1 - right);
  b.right = (size)(wires - 1 - left);

  if(cmp_pair_pub(&a, &b) > 0)
    return 0; // canonical: only keep one of mirror pair (a <= b) to avoid duplicates

  mask |= bit_mask_pub(a.left);
  mask |= bit_mask_pub(a.right);
  mask |= bit_mask_pub(b.left);
  mask |= bit_mask_pub(b.right);

  orbit->first = a;
  orbit->second = b;
  orbit->mask = mask;
  orbit->count = cmp_pair_pub(&a, &b) == 0 ? 1 : 2;
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
        continue; // already present in network, skip
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
      mask[i] |= bit_mask_pub(cmp->left);
      mask[i] |= bit_mask_pub(cmp->right);
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

static int strict_proof(const network_t *net){
  // Exhaustive zero-one proof for n<=20, else fallback to heuristic
  if(net==NULL) return 0;
  if(net->wires==0) return 1;
  if(net->wires > 20) return network_proof(net); // for large n, rely on linked proof
  // Use proof_exp logic inline to avoid link dependency
  uint64_t limit = 1ULL << net->wires;
  for(uint64_t mask=0; mask<limit; ++mask){
    size *data = malloc(net->wires * sizeof *data);
    if(!data) return 0;
    for(size i=0;i<net->wires;++i) data[i]=(size)((mask>>i)&1u);
    int ok = network_sort(net, data);
    // is_sorted
    int sorted=1;
    for(size i=1;i<net->wires;++i) if(data[i-1] > data[i]) sorted=0;
    free(data);
    if(!ok || !sorted) return 0;
  }
  return 1;
}

static int search_orbits(size idx, const orbit_t *cand, size cand_count, network_t *work, uint64_t *masks)
{
  size layer;

  if(network_proof(work) && strict_proof(work))
    return 1; // require both heuristic and strict (if n<=20) // early success: current partial placement already sorts

  if(idx >= cand_count)
    return 0;

  if(cand[idx].count == 1)
  {
    for(layer = 0; layer < work->layers; ++layer)
    {
      size added;
      uint64_t mask = bit_mask_pub(cand[idx].first.left) | bit_mask_pub(cand[idx].first.right);

      if(masks[layer] & mask)
        continue; // wire already occupied in this layer
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
    uint64_t mask1 = bit_mask_pub(cand[idx].first.left) | bit_mask_pub(cand[idx].first.right);
    uint64_t mask2 = bit_mask_pub(cand[idx].second.left) | bit_mask_pub(cand[idx].second.right);

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
  if(target_wires > SORTER_MAX_WIRES)
  {
    fprintf(stderr, "search limit exceeded: %u wires (max %d)\n", (unsigned)target_wires, SORTER_MAX_WIRES);
    return NULL;
  }

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

        if(extra && !network_insert_empty_layer_pub(work, layer_pos))
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
          if(!strict_proof(work)){
            fprintf(stderr, "candidate failed strict proof, continuing search\n");
          } else {
            free(cand);
            free(masks);
            network_free(base);
            return work;
          }
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

int run_proof_cmd(size target_wires, int have_target)
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

int run_search_cmd(size target_wires, int have_target, size max_extra_layers)
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

  if(!strict_proof(found)){
    fprintf(stderr, "warning: found network failed strict proof (heuristic only) - no valid extension\n");
    network_free(found);
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
