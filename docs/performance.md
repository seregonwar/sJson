# Performance Methodology

sJson benchmark numbers must be reproducible and tied to a command line.

Recommended local flow:

```sh
make fetch-assets
make check
make bench
make bench-compare
make bench-assets
```

Report at least:

- CPU and operating system;
- compiler and version;
- build flags;
- iteration count;
- raw benchmark output;
- JSONTestSuite result.

## Datasets

- Generated micro datasets in `benchmarks/bench_sjson.c` and `benchmarks/bench_compare.c` isolate number, object and string-heavy paths.
- Downloaded nativejson-benchmark datasets exercise real-world structures:
  - `canada.json`: floating-point heavy;
  - `citm_catalog.json`: object/string heavy;
  - `twitter.json`: mixed object/string/array payload.

## Benchmark policy

Do not hide regressions. If another library wins a dataset, keep it visible and use it as an optimization target.
