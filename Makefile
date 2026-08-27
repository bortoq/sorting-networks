CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -D_GNU_SOURCE

SRC_COMMON = src/network.c src/search.c
SRC_GEN_COMMON = src/gen/gen_common.c
SRC_GEN = src/gen/gen_batcher.c src/gen/gen_pipelined.c src/gen/gen_pairwise.c src/gen/gen_bose.c src/gen/gen_bitonic.c src/gen/gen_brick.c src/gen/gen_green.c src/gen/gen_vanvoorhis.c src/gen/gen_zigzag.c
SRC_GEN_ALL = $(SRC_GEN_COMMON) $(SRC_GEN) src/gen/gen_network.c

.PHONY: all clean test

all: sorter sorter_exp layer_perm gen_network

sorter: $(SRC_COMMON) src/main.c src/proof.c src/sorter.h
	$(CC) $(CFLAGS) -O2 $(SRC_COMMON) src/main.c src/proof.c -o sorter

sorter_exp: $(SRC_COMMON) src/main.c src/proof_exp.c src/sorter.h
	$(CC) $(CFLAGS) -O2 $(SRC_COMMON) src/main.c src/proof_exp.c -o sorter_exp

layer_perm: src/layer_perm.c
	$(CC) $(CFLAGS) -O2 src/layer_perm.c -o layer_perm

gen_network: $(SRC_GEN_ALL) src/gen/gen.h src/gen/gen_common.h
	$(CC) $(CFLAGS) -O2 $(SRC_GEN_ALL) -o gen_network

test: sorter_exp
	@echo "== strict proof (zero-one) on known networks =="
	@for f in known/n*.txt; do \
	  echo -n "$$f: "; \
	  ./sorter_exp proof < $$f > /dev/null && echo "OK" || echo "FAIL"; \
	done
	@echo "== generator smoke test =="
	@./gen_network --count pairwise 16 | grep -q "63 10" && echo "pairwise 16: OK" || echo "pairwise 16: FAIL"
	@./gen_network --count batcher-odd-even 16 | grep -q "63" && echo "batcher 16: OK" || echo "batcher 16: FAIL"
	@./gen_network --count bose-nelson 16 | grep -q "65" && echo "bose 16: OK" || echo "bose 16: FAIL"
	@./gen_network --count bitonic 16 | grep -q "80" && echo "bitonic 16: OK" || echo "bitonic 16: FAIL"

test-strict: sorter_exp
	./tests/run.sh

clean:
	rm -f sorter sorter_exp layer_perm gen_network src/gen/*.o
