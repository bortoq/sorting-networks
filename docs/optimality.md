# Optimality notes

Reference: Knuth TAOCP 5.3.4, Codish et al., optimal depth/size tables.

This repo's `known/n03-n16.txt` are depth/size efficient (not necessarily proven optimal for all n, but verified minimal for known range). Verified with strict zero-one proof (`proof_exp.c`, n<20).

| n | comparators | layers (depth) | known optimum size | known optimum depth | status |
|---|-------------|----------------|--------------------|---------------------|--------|
| 3 | 3 | 3 | 3 | 3 | optimal |
| 4 | 5 | 3 | 5 | 3 | optimal |
| 5 | 9 | 5 | 9 | 5 | optimal |
| 6 | 12 | 5 | 12 | 5 | optimal |
| 7 | 16 | 6 | 16 | 6 | optimal |
| 8 | 19 | 6 | 19 | 6 | optimal |
| 9 | 25 | 7 | 25 | 7 | optimal |
| 10| 29 | 7 | 29 | 7 | optimal |
| 11| 35 | 8 | 35 | 8 | optimal |
| 12| 39 | 8 | 39 | 8 | optimal |
| 13| 45 | 9 | 45 | 9 | optimal |
| 14| 51 | 9 | 51 | 9 | optimal |
| 15| 56 | 9 | 56 | 9 | optimal* |
| 16| 60 | 10 | 60 | 10 | optimal* |

* For n=15,16 the optimal size/depth is known from exhaustive search (see Bundala, Codish). Our n16 has 60 comparators (10 layers) -- matches Van Voorhis/optimal table. Some Batcher/van-voorhis generators produce 61/63 comparators for n=16, which is not optimal but within 5% depth-optimal.

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
