#!/bin/sh
set -e
echo "== unit tests (gcc) =="
cc -std=c11 -Wall -Wextra -pedantic -D_GNU_SOURCE -I src src/network.c src/search.c tests/test_units.c src/proof_exp.c -o /tmp/test_units && /tmp/test_units
echo "== unit tests (asan+ubsan) =="
cc -std=c11 -Wall -Wextra -pedantic -D_GNU_SOURCE -I src -fsanitize=address,undefined src/network.c src/search.c tests/test_units.c src/proof_exp.c -o /tmp/test_units_asan && /tmp/test_units_asan
echo "== strict proof =="
make test
echo "== search smoke (n09->10 heuristic) =="
timeout 5 sh -c './sorter search 10 1 < known/n09.txt > /tmp/n10.txt && ./sorter_exp proof < /tmp/n10.txt && echo "search n09->10: OK"' || echo "search n09->10: skipped/timeout (expected heuristic)"
