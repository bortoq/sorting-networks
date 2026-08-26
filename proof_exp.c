/*
 * experimental sorter proof routines
 */
#include "sorter.h"
#include <string.h>

#ifndef PROOF_MAX_WIRES
#define PROOF_MAX_WIRES 30
#endif

int network_sort(const network_t *net, size *data)
{
  size l;
  size n;

  if(net == NULL || data == NULL)
    return 0;

  for(l = 0; l < net->layers; ++l)
  {
    const layer_t *layer = &net->layer[l];
    for(n = 0; n < layer->count; ++n)
    {
      size left = layer->pairs[n].left;
      size right = layer->pairs[n].right;
      if(left >= net->wires || right >= net->wires)
        return 0;
      if(left == right)
        continue;
      if(data[left] > data[right])
      {
        size tmp = data[left];
        data[left] = data[right];
        data[right] = tmp;
      }
    }
  }

  return 1;
}

static int is_sorted(const size *data, size n)
{
  size i;

  for(i = 1; i < n; ++i)
    if(data[i - 1] > data[i])
      return 0;
  return 1;
}

static int proof_mask(const network_t *net, uint64_t mask)
{
  size i;
  size *data = malloc(net->wires * sizeof *data);
  int ok;

  if(data == NULL)
    return 0;

  for(i = 0; i < net->wires; ++i)
    data[i] = (size)((mask >> i) & 1u);

  ok = network_sort(net, data) && is_sorted(data, net->wires);
  free(data);
  return ok;
}

int network_proof(const network_t *net)
{
  uint64_t limit;
  uint64_t mask;

  if(net == NULL)
    return 0;

  if(net->wires == 0)
    return 1;

  /*
   * Exhaustive zero-one proof.
   * Practical for small networks; the search code keeps the target small.
   */
  if(net->wires > PROOF_MAX_WIRES || net->wires >= 64)
  {
    fprintf(stderr, "proof limit exceeded: %u wires (max %d)\n", (unsigned)net->wires, PROOF_MAX_WIRES);
    return 0;
  }
  if(net->wires >= 32)
  {
    /* 2^n would overflow 64-bit, use iterative counter */
    fprintf(stderr, "proof limit exceeded: %u wires too large for exhaustive check\n", (unsigned)net->wires);
    return 0;
  }

  limit = 1ULL << net->wires;
  for(mask = 0; mask < limit; ++mask)
    if(!proof_mask(net, mask))
      return 0;

  return 1;
}
