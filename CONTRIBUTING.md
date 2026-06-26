# Contributing

Before submitting changes:

```sh
make check
make bench
```

For performance changes, include:

- compiler and version;
- OS and CPU;
- exact benchmark command;
- before/after output;
- correctness test output.

Rules:

- do not trade correctness for benchmark-only wins;
- keep benchmark datasets and commands reproducible;
- document public API changes in `docs/api.md` and `CHANGELOG.md`;
- add regression tests for parser fixes.
