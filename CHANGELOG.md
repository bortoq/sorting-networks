# Changelog

All notable changes to this project will be documented in this file.

## [0.3.1] - 2026-08-26
### Fixed (audit v2 P0)
- parser: `strtol` + range `0..64`, reject `0 0`/empty/trailing garbage, `cannot read` now `exit 1`
- gen: `next_pow2` overflow guard + `n 1..65536` for all 9 generators
- search: `strict_proof` (exhaustive n<=20) + heuristic, no invalid output, `no extension` honest
- Makefile: `-O2` for `sorter/sorter_exp` (`n24 15.6s->3.8s`), docs: `--count` order, size vs depth table

### Added (audit v1 P1)
- `tests`: negative parse, roundtrip, 9 gens `VALID` + `SORT OK`, empty file must fail

## [0.3.0] - 2026-08-26
### Added
- Generalized `pairwise`/`batcher`/`pipelined` to any `n` via `next_pow2` padding (e.g. `10->31/9`)
- `--validate` flag for all generators (layer matching check)
- Real `pipelined-mergesort`: pipeline register between sort/merge (16:14 vs 10, 32:20 vs 15)
- Theory headers + intent comments (1+3-lite) in 6 modules
- `seq_reserve` for large `n` (65536: 0.11s -> 0.09s)

### Fixed
- `van-voorhis` now supports any `n` (20->116/18, 30->215/20) via pad to `2^(2^k)`
- `pipelined` was alias to Batcher -- now staged pipeline, docs updated
- `SECURITY.md` wording softened, ASCII-only
- `sorter.h` 64-wire guard, modular split `network.c`/`search_lib.c`/`main.c`

## [0.2.0] - 2026-08-26
### Added
- `network.c` / `search_lib.c` split (core vs search), `uint32_t` wire type, 64-wire guard
- `tests/test_units.c` + `tests/run.sh` (unit + ASAN)
- `docs/optimality.md` with full 3..16 table
- CI matrix gcc+clang + ASAN/UBSAN + -Werror
- `.clang-format` (LLVM, 2 spaces)
- `CONTRIBUTING.md`, `CHANGELOG.md`, `CITATION.cff`, `SECURITY.md`
- Full known table in README (25 comps for n=9 etc, not stub)

### Fixed
- `proof.c` now warns it is heuristic, not sound
- `proof_exp.c` / search now guard `n > 64` and `n > 30`
- `README` now ASCII-only, limitations clarified

## [0.1.0] - 2026-08-26
- Initial public release: search, proof, generators, known n=3..16
