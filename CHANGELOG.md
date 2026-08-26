# Changelog

All notable changes to this project will be documented in this file.

## [0.2.0] - 2026-08-26
### Added
- `network.c` / `sorter.c` split (core vs search), `uint32_t` wire type, 64-wire guard
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
