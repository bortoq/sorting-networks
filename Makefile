CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic

.PHONY: all clean test

all: sorter sorter_exp layer_perm gen_network

sorter: sorter.c proof.c sorter.h
	$(CC) $(CFLAGS) sorter.c proof.c -o sorter

sorter_exp: sorter.c proof_exp.c sorter.h
	$(CC) $(CFLAGS) sorter.c proof_exp.c -o sorter_exp

layer_perm: layer_perm.c
	$(CC) $(CFLAGS) -O2 layer_perm.c -o layer_perm

gen_network: gen_network.c
	$(CC) $(CFLAGS) -O2 gen_network.c -o gen_network

test: sorter_exp
	@echo "== strict proof (zero-one) on known networks =="
	@for f in known/n*.txt; do \
	  echo -n "$$f: "; \
	  ./sorter_exp proof < $$f > /dev/null && echo "OK" || echo "FAIL"; \
	done
	@echo "== generator smoke test =="
	@./gen_network --count pairwise 16 | grep -q "63 10" && echo "pairwise 16: OK" || echo "pairwise 16: FAIL"
	@./gen_network --count batcher-odd-even 16 | grep -q "63" && echo "batcher 16: OK" || echo "batcher 16: FAIL"

clean:
	rm -f sorter sorter_exp layer_perm gen_network
