/*
 * Sorting network generators - classical constructions + packing
 *
 * Theory:
 *   Four algorithms, all produce comparators as sequence then pack into
 *   layers via wire-ready table (ready[w] = next free layer for wire w):
 *
 *   1) Pairwise (pairwise sorting network): power-of-two only (n=2^k),
 *      recursively sort pairs and merge. Simple but not depth-optimal.
 *
 *   2) Batcher odd-even mergesort (Batcher 1968): power-of-two only.
 *      Recursively sort halves, then odd-even merge with stride r.
 *      Depth O(log^2 n), size O(n log^2 n). Reference implementation for
 *      comparison, not worst-case optimal but practical.
 *
 *   3) Pipelined mergesort: same comparators as Batcher, but inserts
 *      a pipeline register (empty layer) between sort and merge stages.
 *      Slightly deeper (16:14 vs 10, 32:20 vs 15) but pipeline-friendly.
 *      Power-of-two only (padded else).
 *
 *   4) Van Voorhis square (Van Voorhis 1971, Lee 1986): n = 2^(2^k) only
 *      (16,256,65536..). Recursive square decomposition:
 *        - sort rows (r = sqrt(n), r networks)
 *        - sort columns (r networks)
 *        - f-network: economical [2^r,2^r] merger (uses gen_batcher_merge)
 *        - pack comparator sequence into layers via ready[].
 *      For n=16: 61 comparators, 10 layers (depth-optimal).
 *      For n=65536: 3907497 comps, 136 layers.
 *      Our implementation follows Knuth's square scheme.
 *
 *   5) Bose-Nelson (1962): any n, recursive Pbracket/Pstar
 *   6) Bitonic mergesort: power of two, ascending/descending bitonic merge
 *   7) Brick (odd-even transposition): any n, n layers periodic
 *   8) Green filter (1969): n=16 filter (4 layers) truncated/padded else
 *
 *   9) Zig-zag (Goodrich 2014): O(n log n) size, O(n log n) depth, epsilon-halver based
 * (honest, large constants)
 *
 * Output format: one comparator "i j" per line, blank line between layers.
 * Refs: Batcher 1968 "Sorting Networks...", Van Voorhis 1971, Knuth 5.3.4
 */
#include "gen.h"
#include "gen_common.h"
#include <string.h>

int validate_network(const net_t *net, unsigned n)
{
  // Check each layer is matching and wires < n
  for (unsigned l = 0; l < net->layers; ++l)
  {
    unsigned char *used = calloc(n, 1);
    if (!used)
      die("calloc failed in validate");
    for (unsigned k = 0; k < net->layer[l].count; ++k)
    {
      unsigned a = net->layer[l].pair[k].left, b = net->layer[l].pair[k].right;
      if (a >= n || b >= n)
      {
        free(used);
        return 0;
      }
      if (used[a] || used[b])
      {
        free(used);
        return 0;
      }
      used[a] = used[b] = 1;
    }
    free(used);
  }
  return 1;
}

static void usage(const char *argv0)
{
  fprintf(stderr,
          "usage: %s [--count|--validate] "
          "pairwise|batcher-odd-even|pipelined-mergesort|van-voorhis|bose-nelson|bitonic|"
          "brick|green|zigzag n\n",
          argv0);
}

int main(int argc, char **argv)
{
  const char *algo;
  unsigned n;
  int count_only = 0;
  int validate_only = 0;
  net_t net;

  if (argc == 4 && strcmp(argv[1], "--count") == 0)
  {
    count_only = 1;
    ++argv;
    --argc;
  }
  if (argc == 4 && strcmp(argv[1], "--validate") == 0)
  {
    validate_only = 1;
    ++argv;
    --argc;
  }

  if (argc != 3)
  {
    usage(argv[0]);
    return 1;
  }

  algo = argv[1];
  n = (unsigned)strtoul(argv[2], NULL, 10);

  if (strcmp(algo, "pairwise") == 0)
    net = gen_pairwise(n);
  else if (strcmp(algo, "batcher-odd-even") == 0 || strcmp(algo, "batcher") == 0)
    net = gen_batcher_odd_even(n);
  else if (strcmp(algo, "pipelined-mergesort") == 0 ||
           strcmp(algo, "pipeline-merge") == 0)
    net = gen_pipelined_mergesort(n);
  else if (strcmp(algo, "van-voorhis") == 0 || strcmp(algo, "vv") == 0)
    net = gen_van_voorhis(n);
  else if (strcmp(algo, "bose-nelson") == 0 || strcmp(algo, "bose") == 0)
    net = gen_bose_nelson(n);
  else if (strcmp(algo, "bitonic") == 0)
    net = gen_bitonic(n);
  else if (strcmp(algo, "brick") == 0)
    net = gen_brick(n);
  else if (strcmp(algo, "green") == 0)
    net = gen_green(n);
  else if (strcmp(algo, "zigzag") == 0 || strcmp(algo, "zig-zag") == 0)
    net = gen_zigzag(n);
  else
  {
    usage(argv[0]);
    return 1;
  }

  if (count_only)
    printf("%llu %u\n", (unsigned long long)net_cmp_count(&net), net.layers);
  else if (validate_only)
  {
    int ok = validate_network(&net, n);
    printf("%s: %llu comps, %u layers, %s\n", algo,
           (unsigned long long)net_cmp_count(&net), net.layers, ok ? "VALID" : "INVALID");
    net_free(&net);
    return ok ? 0 : 2;
  }
  else
    net_print(&net);

  net_free(&net);
  return 0;
}
