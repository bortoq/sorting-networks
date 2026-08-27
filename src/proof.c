/*
 * heuristic sorting network proof - anti-sorted + rotations
 *
 * Theory:
 *   Fast unsound filter: build anti-sorted array [n,..,1], run network,
 *   check is_ordered, rotate array by 1 and repeat n times. Catches many
 *   broken networks quickly but MISSES some: not all permutations tested,
 *   does not imply correctness (see docs/optimality.md).
 *   Use sorter_exp (zero-one) for sound verification.
 *
 *   Complexity: O(n * size) vs O(2^n * size) for strict proof.
 *
 *   This file exists only for speed during search; search still calls
 *   network_proof which may be linked to either proof.c or proof_exp.c.
 */
#include "sorter.h"

/*
 * create array and populate it with anti-ordered position indices.
 * return pointer to array.
 * NOTE: malloc()
 */
static size *array_generator(size n)
{
  size i;
  size *a = malloc(n * sizeof *a);

  if(a == NULL)
    return NULL;

  for(i = 0; i < n; ++i)
    a[i] = n - i;

  return a;
}

/*
 * return 0 if array a is not ordered
 */
static int is_ordered(size n, const size *a)
{
  size i;

  for(i = 1; i < n; ++i)
    if(a[i - 1] > a[i])
      return 0;
  return 1;
}

/*
 * cycle shift array 1 position toward the last position
 */
static void shift_array(size n, size *a)
{
  size i;
  size tmp;

  if(n == 0)
    return;

  tmp = a[n - 1];
  for(i = n - 1; i > 0; --i)
    a[i] = a[i - 1];
  a[0] = tmp;
}

/*
 * sort array using sorting network
 */
/* network_sort moved to network.c */

/*
 * full test of sorter: all pathes from any position to any other should exist.
 * return 0 if test failed.
 */
/* WARNING: heuristic proof only (anti-sorted + rotations), not sound.
 * Use sorter_exp (zero-one principle) for strict verification. */
int network_proof(const network_t *net)
{
  size i;
  int ok = 1;
  size *a;

  if(net == NULL)
    return 0;

  if(net->wires > SORTER_MAX_WIRES)
  {
    fprintf(stderr, "warning: heuristic proof on %u wires (n > %d may be incomplete)\n", (unsigned)net->wires, SORTER_MAX_WIRES);
  }
  a = array_generator(net->wires);
  if(a == NULL && net->wires != 0)
    return 0;

  for(i = 0; i < net->wires; ++i)
  {
    ok = network_sort(net, a) && is_ordered(net->wires, a);
    if(ok == 0)
      break;

    shift_array(net->wires, a);
  }

  free(a);
  return ok;
}
