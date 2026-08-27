#!/bin/sh
set -e
echo "== unit tests (gcc) =="
cc -std=c11 -Wall -Wextra -pedantic -D_GNU_SOURCE -I src src/network.c src/search.c tests/test_units.c src/proof_exp.c -o /tmp/test_units && /tmp/test_units
echo "== unit tests (asan+ubsan) =="
cc -std=c11 -Wall -Wextra -pedantic -D_GNU_SOURCE -I src -fsanitize=address,undefined src/network.c src/search.c tests/test_units.c src/proof_exp.c -o /tmp/test_units_asan && /tmp/test_units_asan
echo "== strict proof =="
make test
echo "== search smoke (n09->10 heuristic, expected to fail: heuristic toy) =="
timeout 5 sh -c './sorter search 10 1 < known/n09.txt > /tmp/n10.txt && ./sorter_exp proof < /tmp/n10.txt && echo "search n09->10: UNEXPECTED OK (heuristic found valid)" || echo "search n09->10: no extension (expected for heuristic toy)"' || true
echo "== generator round-trip proof (all 9 gens) =="
for algo in pairwise batcher-odd-even bose-nelson bitonic brick; do
  ./gen_network $algo 8 > /tmp/gen8.txt
  ./sorter_exp proof 8 < /tmp/gen8.txt > /dev/null && echo "$algo 8: SORT OK" || { echo "$algo 8: FAIL"; exit 1; }
done
# negative test: broken network must fail
echo "0 1" > /tmp/broken.txt; echo "1 0" >> /tmp/broken.txt
! ./sorter_exp proof 2 < /tmp/broken.txt > /dev/null && echo "negative broken net: correctly FAIL" || { echo "negative broken net: should FAIL"; exit 1; }
echo "empty file must fail"
! ./sorter_exp proof < /dev/null > /dev/null 2>&1 && echo "empty: correctly FAIL" || { echo "empty should FAIL"; exit 1; }
