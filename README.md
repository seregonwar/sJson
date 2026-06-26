# sJson

**sJson** is a single-header **C99** JSON library focused on safety, portability and measurable speed. It uses an arena allocator, has no third-party runtime dependency, and exposes a stable C API that can be wrapped by C++, Rust and other static languages.

Version: **1.1.0**  
License: **GPL-3.0-only**

## Project goal

The main goal is to make sJson as fast as possible without hiding correctness or safety regressions. Performance work must be driven by reproducible benchmarks, not by claims.

Priorities:

1. iterative parser with no C recursion;
2. reusable arena allocation through `json_arena_reset`;
3. object lookup through FNV-1a hashing and an optional sorted index;
4. benchmark suites that report throughput and latency clearly;
5. a stable C ABI suitable for Rust FFI, C++ wrappers and other static-language integrations.

## Features

- C99 single-header integration through `src/json_pal.h`.
- No third-party dependency for the library itself.
- Arena allocator with pluggable backing allocator.
- RFC 8259-style parser with configurable limits for depth, nodes and collection sizes.
- `\uXXXX` decoding and surrogate-pair support.
- Integers stored as `int64_t`; floating-point values stored as `double`.
- Objects preserve insertion order and can build a sorted index for fast lookup.
- Compact writer, pretty writer, sorted-key output and write-size measurement.

## Quick start

```c
#define JSON_IMPLEMENTATION
#include "src/json_pal.h"

int main(void) {
    JsonArena* arena = json_arena_create(NULL, 64 * 1024);
    JsonError err = JSON_OK;
    JsonValue* root = json_parse_cstr(arena, "{\"x\":1,\"ok\":true}", &err);

    if (!root) {
        json_arena_destroy(arena);
        return 1;
    }

    json_obj_finalize(arena, root);

    JsonValue* x_value = NULL;
    int64_t x = 0;
    json_obj_get(root, "x", &x_value);
    json_get_int(x_value, &x);

    json_arena_destroy(arena);
    return x == 1 ? 0 : 1;
}
```

## Build tools

On MSYS2/MinGW64, install the toolchain with:

```sh
/c/msys64/usr/bin/pacman.exe -S --needed --noconfirm \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  make
```

For the optional comparative benchmark against cJSON:

```sh
/c/msys64/usr/bin/pacman.exe -S --needed --noconfirm mingw-w64-x86_64-cjson
```

Use the MSYS2 make executable and make sure MinGW64 is at the front of `PATH`:

```sh
export PATH=/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH
export TEMP=C:/msys64/tmp TMP=C:/msys64/tmp TMPDIR=/c/msys64/tmp
/c/msys64/usr/bin/make.exe CC=gcc test
```

## Build, test and benchmark

### Make

```sh
make test           # regression suite
make asan           # AddressSanitizer + UBSan, if supported
make bench          # sJson parser and lookup benchmark
make bench-compare  # comparative benchmark against cJSON, if installed
make fetch-assets   # download JSONTestSuite + nativejson-benchmark assets
make corpus-test    # run JSONTestSuite test_parsing corpus
make bench-assets   # benchmark official/nativejson assets
make check          # regression + corpus + fuzz smoke
```

### CMake

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
./build/bench_sjson              # Linux/macOS/MSYS single-config
./build/Release/bench_sjson.exe  # Windows multi-config
```

Installable CMake package:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSJSON_BUILD_TESTS=OFF
cmake --build build
cmake --install build --prefix /usr/local
```

Downstream CMake usage:

```cmake
find_package(sJson CONFIG REQUIRED)
target_link_libraries(app PRIVATE sJson::sjson)
```

The benchmark accepts an optional iteration count:

```sh
./benchmarks/bench_sjson 1000
./benchmarks/bench_compare 1000
./benchmarks/bench_assets --iterations 100 benchmarks/assets/canada.json benchmarks/assets/twitter.json
```

## Benchmarks

### Native sJson benchmark

`benchmarks/bench_sjson.c` measures:

| Case | What it measures |
| --- | --- |
| `numbers` | numeric-array parsing, number validation and node creation |
| `object` | object parsing, key allocation and object storage growth |
| `strings` | string copying and arena allocation behavior |
| `lookup finalized object` | average lookup cost after `json_obj_finalize` |

Example output:

```text
sJson benchmark (iterations=500)
parse numbers               48891 B     500 it     255.39 MB/s    3.734 ns/B
parse object                26891 B     500 it     345.37 MB/s    2.761 ns/B
parse strings              132001 B     500 it     782.35 MB/s    1.219 ns/B
lookup finalized object     2000 keys    500 it      74.02 ns/lookup
```

### Comparative benchmark

`benchmarks/bench_compare.c` currently compares sJson against **cJSON** on the same generated datasets. It is intentionally simple: parse the same buffer repeatedly, then release or reset the produced tree.

Example from this workstation with GCC 15.2, MinGW64, `-O3 -DNDEBUG`:

```text
sJson comparative benchmark (iterations=300)
sJson    numbers       48891 B    300 it    263.65 MB/s   3.617 ns/B    1.00x vs sJson
cJSON    numbers       48891 B    300 it     25.57 MB/s  37.300 ns/B    0.10x vs sJson
sJson    object        26891 B    300 it    361.10 MB/s   2.641 ns/B    1.00x vs sJson
cJSON    object        26891 B    300 it     54.27 MB/s  17.574 ns/B    0.15x vs sJson
sJson    strings      132001 B    300 it    834.49 MB/s   1.143 ns/B    1.00x vs sJson
cJSON    strings      132001 B    300 it    253.14 MB/s   3.767 ns/B    0.30x vs sJson
```

Interpretation: values below `1.00x vs sJson` mean the competitor is slower than sJson for that dataset.

#### Generated-dataset throughput chart

GCC 15.2, MinGW64, `-O3 -DNDEBUG`, 300 iterations. Longer is better.

```text
numbers MB/s
sJson  | ████████████████████████████████████████ 263.65
cJSON  | ████                                      25.57

object MB/s
sJson  | ████████████████████████████████████████ 361.10
cJSON  | ██████                                    54.27

strings MB/s
sJson  | ████████████████████████████████████████ 834.49
cJSON  | ████████████                             253.14
```

### Official corpus and asset benchmarks

sJson can be validated against external, well-known JSON assets without committing those assets to the repository:

```sh
make fetch-assets
make corpus-test
make bench-assets
```

Fetched sources:

- `nst/JSONTestSuite`, `test_parsing`, for correctness and edge-case validation.
- `miloyip/nativejson-benchmark`, `canada.json`, `citm_catalog.json`, `twitter.json`, for real-world performance datasets.

Current JSONTestSuite result on this workstation:

```text
JSONTestSuite corpus: tests/JSONTestSuite/test_parsing
  valid accepted:      95
  valid rejected:      0
  invalid rejected:    188
  invalid accepted:    0
  impl accepted:       12
  impl rejected:       23
  skipped:             0
```

Official/nativejson asset benchmark, GCC 15.2, MinGW64, `-O3 -DNDEBUG`, 50 iterations:

```text
sJson    canada.json               2251051 B    50 it    563.08 MB/s   1.694 ns/B   1.00x vs sJson
cJSON    canada.json               2251051 B    50 it     22.83 MB/s  41.773 ns/B   0.04x vs sJson
sJson    citm_catalog.json         1727204 B    50 it    859.85 MB/s   1.109 ns/B   1.00x vs sJson
cJSON    citm_catalog.json         1727204 B    50 it    218.34 MB/s   4.368 ns/B   0.25x vs sJson
sJson    twitter.json               631514 B    50 it    628.67 MB/s   1.517 ns/B   1.00x vs sJson
cJSON    twitter.json               631514 B    50 it    181.46 MB/s   5.255 ns/B   0.29x vs sJson
```

#### Official/nativejson throughput chart

```text
canada.json MB/s
sJson  | ████████████████████████████████████████ 563.08
cJSON  | ██                                        22.83

citm_catalog.json MB/s
sJson  | ████████████████████████████████████████ 859.85
cJSON  | ██████████                               218.34

twitter.json MB/s
sJson  | ████████████████████████████████████████ 628.67
cJSON  | ████████████                             181.46
```

## Benchmarking policy

Every performance-oriented change should report:

- compiler and version;
- CPU and operating system;
- build flags, preferably `-O3 -DNDEBUG` for benchmarks;
- iteration count;
- MB/s, ns/byte and ns/lookup where applicable;
- full regression-suite status.

The benchmark suite must be honest and repeatable. If a competing library wins a dataset, that result should stay visible and become an optimization target.

## Quality gates

The repository includes:

- GitHub Actions CI for Linux GCC/Clang and Windows MSYS2;
- sanitizer targets: `make asan`, `make ubsan`;
- fuzz smoke target: `make fuzz-smoke`;
- JSONTestSuite corpus runner;
- CMake package/install support.

Additional documentation:

- `docs/api.md`
- `docs/performance.md`
- `docs/compatibility.md`
- `CONTRIBUTING.md`
- `SECURITY.md`

## C API, FFI and compatibility wrappers

The public C API is the stable foundation for external bindings.

Recommended rules:

- do not transfer ownership of individual nodes: every parsed value lives in a `JsonArena`;
- create and destroy arenas explicitly from the host language or through a wrapper;
- expose strings as pointer + length; do not assume NUL termination;
- call `json_obj_finalize` before repeated object lookups.

FFI roadmap:

- **C++**: initial RAII wrapper in `bindings/cpp/sjson.hpp` around `JsonArena*` and `JsonValue*`.
- **Rust**: `sjson-sys` crate generated from the C ABI, followed by a safe wrapper crate.
- **Flat C ABI**: stable opaque-handle API for languages that should not depend on internal structs.
- **Cross-language benchmarks**: C vs C++ wrapper vs Rust FFI using the same datasets.

### cJSON compatibility layer

For projects migrating from cJSON, sJson includes a source-level compatibility subset:

```c
#define JSON_IMPLEMENTATION
#include "src/json_pal.h"
#include "compat/cjson/sjson_cjson_compat.h"

cJSON* root = cJSON_Parse("{\"x\":1}");
cJSON* x = cJSON_GetObjectItemCaseSensitive(root, "x");
double value = cJSON_GetNumberValue(x);
cJSON_Delete(x);
cJSON_Delete(root);
```

This is **not ABI-compatible** with the real cJSON shared library. It is a migration aid for common parse/read code paths: parsing, deletion, type checks, object lookup, array access, string extraction and number extraction.

## Configuration

Define configuration macros before including `json_pal.h`:

```c
#define JSON_MAX_DEPTH        64
#define JSON_MAX_STRING_LEN   1048576
#define JSON_MAX_NODES        1048576
#define JSON_MAX_ARRAY_LEN    1048576
#define JSON_INTROSORT_THRESH 16
#define JSON_ARENA_ALIGN      8
#define JSON_FAST_FLOAT       1  /* 0 = always use strtod for floating point */
```

PAL overrides:

```c
#define JSON_MEMCPY   my_memcpy
#define JSON_MEMSET   my_memset
#define JSON_MEMCMP   my_memcmp
#define JSON_STRLEN   my_strlen
#define JSON_ASSERT   my_assert
#define JSON_STRTOD   my_strtod
#define JSON_STRTOLL  my_strtoll
```

With `JSON_NO_STDLIB`, sJson avoids standard-library headers and expects suitable PAL overrides.

## Path queries

```c
json_path(root, "users[0].name", NULL);
json_path(root, "config[\"host\"]", NULL);
json_path(root, "[2][1]", NULL);
```

Supported forms: `.key`, `["key"]` and `[N]`.

## Repository layout

```text
src/json_pal.h            public include header
src/sJson.c               single-header implementation
src/test_json.c           regression suite
benchmarks/bench_sjson.c  native sJson performance benchmark
benchmarks/bench_compare.c comparative benchmark against cJSON
benchmarks/bench_assets.c  benchmark runner for downloaded official assets
tests/json_corpus_runner.c JSONTestSuite corpus runner
fuzz/fuzz_parse.c          libFuzzer-compatible parse/write fuzz target
compat/cjson/              source-level cJSON migration wrapper
bindings/cpp/sjson.hpp    initial C++ RAII wrapper
scripts/fetch_official_assets.sh official asset downloader
docs/                     API, performance and compatibility notes
.github/workflows/ci.yml  Linux/Windows CI
Makefile                  quick build/test/benchmark targets
CMakeLists.txt            cross-platform build/install/package file
```

## Threading note

sJson does not require global mutable parser state. For concurrent use, give each thread or document its own `JsonArena`; do not reset or mutate the same arena from multiple threads at the same time.
