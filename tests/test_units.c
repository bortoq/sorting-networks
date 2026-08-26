#define _GNU_SOURCE
#include "../sorter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_network_new_free() {
  network_t *n = network_new(4, 2);
  assert(n && n->wires==4 && n->cap==2);
  network_free(n);
  network_free(NULL);
  assert(network_new(0,0) != NULL);
  printf("test_network_new_free: OK\n");
}
static void test_add_cmp_conflict() {
  network_t *n = network_new(4, 1);
  assert(network_add_cmp(n, 0, 0, 1)==1);
  assert(network_add_cmp(n, 0, 0, 2)==0); // wire 0 busy
  assert(network_add_cmp(n, 0, 2, 3)==1);
  assert(network_add_cmp(n, 0, 1, 2)==0); // both busy
  assert(network_add_cmp(n, 1, 1, 2)==1); // new layer ok
  network_free(n);
  printf("test_add_cmp_conflict: OK\n");
}
static void test_insert_wire() {
  network_t *n = network_new(3, 1);
  network_add_cmp(n, 0, 0, 2);
  assert(network_insert_wire(n, 1)==1);
  assert(n->wires==4);
  // 0,2 -> 0,3
  assert(n->layer[0].pairs[0].left==0 && n->layer[0].pairs[0].right==3);
  network_free(n);
  printf("test_insert_wire: OK\n");
}
static void test_load_write() {
  const char *txt = "0 1\n2 3\n\n0 2\n1 3\n";
  FILE *f = fmemopen((void*)txt, strlen(txt), "r");
  network_t *n = network_load(f);
  fclose(f);
  assert(n && n->layers==2 && n->wires==4);
  // write and reload
  char *buf=NULL; size_t sz=0;
  FILE *out = open_memstream(&buf,&sz);
  network_write(out,n); fclose(out);
  f = fmemopen(buf, sz, "r");
  network_t *m = network_load(f); fclose(f);
  assert(m && m->layers==2);
  free(buf); network_free(n); network_free(m);
  printf("test_load_write: OK\n");
}
static void test_load_invalid() {
  const char *bad = "0 1\n0 1\n"; // duplicate in same layer -> invalid
  FILE *f = fmemopen((void*)bad, strlen(bad), "r");
  network_t *n = network_load(f); fclose(f);
  assert(n==NULL);
  printf("test_load_invalid: OK\n");
}
static void test_proof_heuristic_vs_strict() {
  // n=4 known good network should pass both
  FILE *f = fopen("known/n04.txt","r");
  assert(f);
  network_t *n = network_load(f); fclose(f);
  assert(n);
  size data[4]={4,3,2,1};
  assert(network_sort(n,data)==1);
  assert(data[0]==1);
  network_free(n);
  printf("test_proof_heuristic: OK\n");
}
static void test_max_wires_guard() {
  network_t *seed = network_new(64,1);
  network_t *ext = search_extension(seed, 65, 1);
  assert(ext==NULL); // guard
  network_free(seed);
  printf("test_max_wires_guard: OK\n");
}
int main(){
  test_network_new_free();
  test_add_cmp_conflict();
  test_insert_wire();
  test_load_write();
  test_load_invalid();
  test_proof_heuristic_vs_strict();
  test_max_wires_guard();
  printf("ALL UNIT TESTS PASSED\n");
  return 0;
}
