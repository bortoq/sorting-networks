/*
 * strict sorting network proof - zero-one principle
 *
 * Theory:
 *   Knuth 5.3.4 Thm Z (Zero-One Principle): network sorts all inputs over
 *   an ordered set iff it sorts all 2^n binary inputs. So exhaustive
 *   binary test is sound and complete.
 *
 *   Implementation: enumerate masks 0..(1<<n)-1, build array data[i]=
 *   (mask>>i)&1, run network_sort, check is_sorted (non-decreasing).
 *
 *   Cost: 2^n masks, each does size comparators. Practical n < 20
 *   (1M masks), hard limit n < 32 (would overflow 64-bit enumeration)
 *   and n > PROOF_MAX_WIRES (30) rejected. Search keeps target small.
 *
 * Refs: Knuth TAOCP 5.3.4, proof by 0-1 principle.
 */
#include "sorter.h"
#include <string.h>

#ifndef PROOF_MAX_WIRES
#define PROOF_MAX_WIRES 30
#endif

/* network_sort moved to network.c */

static int is_sorted(const size *data, size n)
{
  size i;

  for (i = 1; i < n; ++i)
    if (data[i - 1] > data[i])
      return 0;
  return 1;
}

static int proof_mask(const network_t *net, uint64_t mask, size *data)
{
  size i;
  for (i = 0; i < net->wires; ++i)
    data[i] = (size)((mask >> i) & 1u); // binary input from mask bit i
  return network_sort(net, data) && is_sorted(data, net->wires);
}

int network_proof(const network_t *net)
{
  uint64_t limit;
  uint64_t mask;

  if (net == NULL)
    return 0;

  if (net->wires == 0)
    return 1;

  /*
   * Exhaustive zero-one proof.
   * Practical for small networks; the search code keeps the target small.
   */
  if (net->wires > PROOF_MAX_WIRES || net->wires >= 64)
  {
    fprintf(stderr, "proof limit exceeded: %u wires (max %d)\n", (unsigned)net->wires,
            PROOF_MAX_WIRES);
    return 0;
  }
  if (net->wires >= 32)
  {
    /* 2^n would overflow 64-bit, use iterative counter */
    fprintf(stderr, "proof limit exceeded: %u wires too large for exhaustive check\n",
            (unsigned)net->wires);
    return 0;
  }

  limit = 1ULL << net->wires; // 2^n masks
  size *data = malloc(net->wires * sizeof *data);
  if (data == NULL)
    return 0;
  for (mask = 0; mask < limit; ++mask)
    if (!proof_mask(net, mask, data))
    {
      free(data);
      return 0; // found binary counterexample -> not sorting
    }
  free(data);
  return 1;
}
