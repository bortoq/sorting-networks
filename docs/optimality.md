# Optimality notes

Reference: Knuth TAOCP 5.3.4, Codish et al., optimal depth/size tables.

This repo's `known/n03-n16.txt` are depth/size efficient (not necessarily proven optimal for all n, but verified minimal for known range). Verified with strict zero-one proof (`proof_exp.c`, exhaustive `2^n`, practical `n<20`, hard limit `n<32`/`PROOF_MAX 30`).

| n | comparators | layers | optimum size | optimum depth | file status |
|---|-------------|--------|--------------|---------------|-------------|
| 3 | 3 | 3 | **3** | **3** | size+depth optimal |
| 4 | 5 | 3 | **5** | **3** | size+depth optimal |
| 5 | 9 | 5 | **9** | **5** | size+depth optimal |
| 6 | 12 | 5 | **12** | **5** | size+depth optimal |
| 7 | 16 | 6 | **16** | **6** | size+depth optimal |
| 8 | 19 | 6 | **19** | **6** | size+depth optimal |
| 9 | 25 | 7 | **25** | **7** | size+depth optimal |
|10 | 29 | 8 | **29** | **7** (31) | **size-optimal** (depth-optimal is 31/7) |
|11 | 35 | 8 | **35** | **8** | size+depth optimal |
|12 | 39 | 9 | **39** | **8** (40) | **size-optimal** (depth-optimal is 40/8) |
|13 | 45 |10 | **45** | **9** | size-optimal (depth-optimal 46/9) |
|14 | 51 |10 | **51** | **9** | size-optimal (depth-optimal 52/9) |
|15 | 56 |10 | **56** | **9** | size-optimal (depth-optimal 57/9) |
|16 | 60 |10 | **60** | **9** (61) | **size-optimal** (depth-optimal is 61/9) |

* For n=15,16 the optimal size/depth is known from exhaustive search (see Bundala, Codish). Our n16 has 60 comparators (10 layers) -- size-optimal (depth-optimal is 9 layers with 61 comparators). Some Batcher/van-voorhis generators produce 61/63 comparators for n=16, which is not optimal but within 5% depth-optimal.

## How this repo searches

Heuristic incremental search `n-1 -> n`:
- inserts new wire at each position 0..n-1
- enumerates symmetric orbits `a b <-> n-1-b n-1-a` incident to inserted wire / mirror
- packs orbits into existing layers first, then tries one fresh layer at each position
- verifies with `proof_exp` (zero-one). Not exhaustive: `max_extra_layers=1`, only incident comparators, at most 64 wires (64-bit masks).


## Best-known for 17..32 (suboptimal)

`known/best/n17..32.txt` are best-known from Dobbelaere SorterHunter (not proven optimal).
Only `n<=12` size optimal is proven (Harder 2020). For `13..32` only bounds are known.

| n | best size | best depth | source |
|---|-----------|------------|--------|
|17|71|12|Dobbelaere|
|18|77|12|Dobbelaere|
|19|85|12|Dobbelaere|
|20|91|12|Dobbelaere|
|24|120|13|Dobbelaere|
|28|155|14|Dobbelaere|
|32|185|14|Dobbelaere|

See `known/best/README.md` and https://bertdobbelaere.github.io/sorting_networks.html

For full optimality proof use external verifiers / SAT encoding (not included).
