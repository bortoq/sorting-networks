# Contributing

## Build and test
```sh
make all
make test          # strict proof + generators
./tests/run.sh     # unit tests + asan
```

## Style
- C11, `-Wall -Wextra -pedantic` must be clean, `-Werror` in CI.
- Run `clang-format` (LLVM, 2 spaces, 90 cols) before PR if available.
- No Python required for core.

## PR checklist
- `make all && make test` passes locally
- `tests/test_units.c` updated if you change `sorter.h` API
- Update `docs/optimality.md` if you change `known/`
- Add note to `CHANGELOG.md`
