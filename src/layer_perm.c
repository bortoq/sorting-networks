/*
 * Layer permutation robustness analyzer
 *
 * Theory:
 *   Question: how many permutations of layers still sort? For a network
 *   with L layers, test all L! permutations: apply layers in permuted order
 *   to all 2^n binary inputs (zero-one). Count permutations that break
 *   sorting (at least one binary input not sorted).
 *
 *   Example: known/n08.txt (6 layers, 19 comps) -> 720 perms, 704 break,
 *   only 16 permutations still sort (reported as "8 6 720 16 704").
 *   This measures how order-sensitive the network is.
 *
 *   Limits: MAX_WIRES 20, MAX_LAYERS 16, MAX_PAIRS 16 (exhaustive fits).
 *   Brute force L! * 2^n, so practical only for small n/L.
 *
 *   Use: ./layer_perm < network.txt
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_WIRES 20
#define MAX_LAYERS 16
#define MAX_PAIRS 16

typedef struct {
  unsigned left;
  unsigned right;
} pair_t;

typedef struct {
  unsigned count;
  pair_t pair[MAX_PAIRS];
} layer_t;

typedef struct {
  unsigned wires;
  unsigned layers;
  layer_t layer[MAX_LAYERS];
} network_t;

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

static int load_network(FILE *in, network_t *net)
{
  char line[256];
  int active = 0;

  memset(net, 0, sizeof *net);

  while(fgets(line, sizeof line, in) != NULL)
  {
    char *p = trim(line);
    unsigned left;
    unsigned right;
    layer_t *layer;

    if(*p == '\0' || *p == '#')
    {
      active = 0;
      continue;
    }

    char *end;
    long a_val = strtol(p, &end, 10);
    if(end==p || errno==ERANGE || a_val<0 || a_val>=MAX_WIRES) return 0;
    while(*end==' '||*end=='\t') ++end;
    char *b_start=end;
    long b_val = strtol(b_start, &end, 10);
    if(end==b_start || errno==ERANGE || b_val<0 || b_val>=MAX_WIRES) return 0;
    while(*end==' '||*end=='\t') ++end;
    if(*end!='\0') return 0;
    left=(unsigned)a_val; right=(unsigned)b_val;

    if(!active)
    {
      if(net->layers >= MAX_LAYERS)
        return 0;
      ++net->layers;
      active = 1;
    }

    layer = &net->layer[net->layers - 1];
    if(layer->count >= MAX_PAIRS)
      return 0;
    layer->pair[layer->count].left = left;
    layer->pair[layer->count].right = right;
    ++layer->count;

    if(left + 1 > net->wires)
      net->wires = left + 1;
    if(right + 1 > net->wires)
      net->wires = right + 1;
  }

  return net->wires <= MAX_WIRES;
}

static uint32_t apply_layer(uint32_t mask, const layer_t *layer)
{
  unsigned i;

  for(i = 0; i < layer->count; ++i)
  {
    unsigned left = layer->pair[i].left;
    unsigned right = layer->pair[i].right;
    uint32_t lb = (mask >> left) & 1u;
    uint32_t rb = (mask >> right) & 1u;

    if(lb > rb)
    {
      mask &= ~(1u << left);
      mask |= 1u << right;
    }
  }

  return mask;
}

static int sorted_mask(uint32_t mask, unsigned wires)
{
  unsigned i;
  int seen_one = 0;

  for(i = 0; i < wires; ++i)
  {
    int bit = (mask >> i) & 1u;
    if(seen_one && !bit)
      return 0;
    if(bit)
      seen_one = 1;
  }

  return 1;
}

static int permutation_sorts(const network_t *net, const unsigned *perm)
/* test if permuted layer order still sorts (zero-one over 2^n inputs) */
{
  uint32_t limit = 1u << net->wires;
  uint32_t input;

  for(input = 0; input < limit; ++input) // 2^n binary inputs
  {
    uint32_t mask = input; // input bits are binary array
    unsigned i;

    for(i = 0; i < net->layers; ++i)
      mask = apply_layer(mask, &net->layer[perm[i]]);

    if(!sorted_mask(mask, net->wires))
      return 0;
  }

  return 1;
}

static uint64_t fact(unsigned n)
{
  uint64_t r = 1;
  unsigned i;

  for(i = 2; i <= n; ++i)
    r *= i;
  return r;
}

static void count_rec(const network_t *net, unsigned depth, unsigned used,
                      unsigned *perm, uint64_t *ok, uint64_t *bad)
{
  unsigned i;

  if(depth == net->layers)
  {
    if(permutation_sorts(net, perm))
      ++*ok;
    else
      ++*bad;
    return;
  }

  for(i = 0; i < net->layers; ++i)
    if(!(used & (1u << i)))
    {
      perm[depth] = i;
      count_rec(net, depth + 1, used | (1u << i), perm, ok, bad);
    }
}

int main(int argc, char **argv)
{
  network_t net;
  unsigned perm[MAX_LAYERS];
  uint64_t ok = 0;
  uint64_t bad = 0;

  if(!load_network(stdin, &net))
  {
    fprintf(stderr, "cannot read network\n");
    return 1;
  }

  if(argc > 1){
    char *end; errno=0;
    long v=strtol(argv[1], &end, 10);
    if(end==argv[1] || errno==ERANGE || v<=0 || v>MAX_WIRES || *end!='\0'){
      fprintf(stderr, "invalid wires arg\n"); return 1;
    }
    unsigned req=(unsigned)v;
    if(req < net.wires){
      fprintf(stderr, "warning: requested wires %u < network wires %u, using %u\n", req, net.wires, net.wires);
    } else if(req > net.wires){
      // allow larger wires as padding, but check limit
      net.wires = req;
    }
  }
  if(net.wires==0 || net.wires>MAX_WIRES){ fprintf(stderr, "invalid wires\n"); return 1; }

  count_rec(&net, 0, 0, perm, &ok, &bad);
  printf("%u %u %llu %llu %llu\n",
         net.wires,
         net.layers,
         (unsigned long long)fact(net.layers),
         (unsigned long long)ok,
         (unsigned long long)bad);
  return 0;
}
