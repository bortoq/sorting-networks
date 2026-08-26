# sorting-networks

[![CI](https://github.com/bortoq/sorting-networks/actions/workflows/ci.yml/badge.svg)](https://github.com/bortoq/sorting-networks/actions/workflows/ci.yml)


Search and generation of optimal and near-optimal sorting networks.

- **Incremental symmetric search** `n-1 -> n`: insert a new wire, try symmetric comparator orbits, pack into existing layers first, then add one new layer, verify with a proof.
- **Strict proof** via the zero-one principle (exhaustive binary inputs, `n < 20`).
- **Fast proof** via anti-sorted array + cyclic shifts (heuristic, not strictly sound).
- **Generators** for classical networks: pairwise, Batcher odd-even mergesort, pipelined mergesort, Van Voorhis square.
- **Layer permutation analyzer** to count permutations that break a network.

No dependencies, C11, `MIT`.

## Build

```sh
make        # builds sorter, sorter_exp, layer_perm, gen_network
make test   # strict proof on known/n03-n16 + generator smoke test
make clean
```

Requirements: `cc` with `-std=c11`.

## Usage

### 1. Prove a network

Network format: one comparator `i j` per line, empty line separates layers, `#` is a comment.

```
0 1
2 3

0 2
1 3

1 2
```

```sh
# fast heuristic proof (antisorted + rotations)
./sorter proof < known/n16.txt
# strict proof (zero-one principle, n < 20)
./sorter_exp proof 16 < known/n16.txt
```

Exit code `0` = sorted, `1` = failed. `stderr` prints `sort OK` / `sort failed`.

### 2. Search for an extension

Take an optimal network for `n-1`, extend to `n`:

```sh
./sorter search [target_wires] [max_extra_layers] < seed.txt > out.txt
# examples
./sorter search < known/n09.txt > n10_candidate.txt        # n=9 -> 10
./sorter search 10 1 < known/n09.txt > n10_candidate.txt   # explicit
./sorter_exp search 10 1 < known/n09.txt > n10_candidate.txt # with strict proof
```

`max_extra_layers` is capped at `1`. The search:
1. clones seed, inserts a new wire at each position `0..n-1`
2. enumerates symmetric orbits `a b <-> n-1-b n-1-a` touching the new wire or its mirror
3. tries to place orbits into existing layers first, then into one fresh layer (tried at each insertion position)
4. runs `network_proof` after each complete placement

### 3. Generate classical networks

```sh
./gen_network --count pairwise 16              # prints "comparators layers"
./gen_network pairwise 16 > net.txt            # 63 comparators, 10 layers
./gen_network batcher-odd-even 16 > net.txt    # Batcher (power of two)
./gen_network pipelined-mergesort 16 > net.txt # pipelined merge stages
./gen_network van-voorhis 16 > net.txt         # Van Voorhis square, n=2^(2^k)
./gen_network van-voorhis 65536 --count        # 3907497 comparators, 136 layers

# verify
./sorter_exp proof 16 < net.txt
```

Limits:
- `pairwise`, `batcher-odd-even`, `pipelined-mergesort`: `n = 2^k`
- `van-voorhis`: `n = 2^(2^k)` (16, 256, 65536, ...)

### 4. Analyze layer permutations

Counts how many permutations of layers break the network:

```sh
./layer_perm < known/n08.txt
# output: wires layers total_permutations bad_permutations
# e.g. 8 6 720 16 704
```

## Known optimal networks

`known/n03.txt` .. `known/n16.txt` -- reference networks (verified with `proof_exp.c`, exhaustive zero-one, `n<20`).

| n | comparators | layers |
|---|-------------|--------|
| 3 | 3 | 3 |
| 4 | 5 | 3 |
| 5 | 9 | 5 |
| 6 | 12 | 5 |
| 7 | 16 | 6 |
| 8 | 19 | 6 |
| 9 | 25 | 7 |
| 10 | 29 | 8 |
| 11 | 35 | 8 |
| 12 | 39 | 9 |
| 13 | 45 | 10 |
| 14 | 51 | 10 |
| 15 | 56 | 10 |
| 16 | 60 | 10 |

All match known optimum size/depth (Knuth 5.3.4, Codish et al.). See [docs/optimality.md](docs/optimality.md) for details.

> Note: generators (`pairwise 16` = 63 comps/10 layers, `van-voorhis 16` = 61/10) are not optimal -- they are classical constructions for comparison.

## Design

- `sorter.h` -- network structure (`network_t`, `layer_t`, `cmp_t`), API:
  `network_new/clone/free`, `network_add_cmp`, `network_insert_wire`, `network_load/write`, `network_sort`, `network_proof`, `search_extension`
- `sorter.c` -- loader, layer packing, orbit enumeration, backtracking search
- `proof.c` -- fast heuristic proof (anti-sorted + rotate)
- `proof_exp.c` -- strict zero-one proof (`2^n` exhaustive, `n < 20`)
- `gen_network.c` -- generators + sequence-to-layers packer (wire-ready table)
- `layer_perm.c` -- permutation robustness checker

Symmetry: comparator `a b` is added with its mirror `n-1-b n-1-a`. Self-mirrored comparators count as one. Search works on *orbits*, not individual pairs.

## Limitations

- `proof.c` is **not sound** -- heuristic only (anti-sorted + rotations). Use `sorter_exp` (zero-one, `2^n`, `n < 32`, practical `n < 20`) for verification.
- Search is **heuristic, not exhaustive**: `max_extra_layers` capped at `1`, only comparators incident to inserted wire/mirror, `n <= 64` (64-bit masks).
- `proof_exp` and search both limited to `n <= 64`; exhaustive proof limited to `n < 32` (and practical `n < 20`).

## License

MIT -- see [LICENSE](LICENSE).

Author: Dmitri Bortoq <bortoq@gmail.com>
