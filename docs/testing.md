# Validation

The main CMake suite covers the C API, allocator failures, arena reuse/trim,
UTF-8, integer conversion, output buffers, configurable limits, cJSON ownership,
C++ lifetimes, the opaque C ABI, Python, JSONTestSuite and deterministic fuzz
smoke inputs. C++ tests are enabled when a C++ compiler is available. Python
tests require a shared FFI build and an available Python interpreter.

```sh
cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DSJSON_BUILD_BENCHMARKS=OFF \
  -DSJSON_BUILD_FUZZERS=ON -DSJSON_WARNINGS_AS_ERRORS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cargo test --manifest-path bindings/rust/Cargo.toml
```

The allocator regression poisons fresh allocations and counts outstanding
blocks/bytes. Two hundred identical reset cycles must allocate no new blocks
after warmup, and destroy must return both counters to zero. The regression
fails against the original arena implementation, which overwrites the retained
block chain. Allocation failure is injected at each early allocation point.

On Linux with GCC or Clang, run the native suite with sanitizers:

```sh
cmake -S . -B build/asan -DCMAKE_BUILD_TYPE=Debug -DSJSON_SANITIZERS=ON \
  -DSJSON_BUILD_BENCHMARKS=OFF -DSJSON_BUILD_FUZZERS=ON
cmake --build build/asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build/asan --output-on-failure
```

Use a static FFI build for this sanitizer run so an unsanitized Python process
does not load an ASan-instrumented shared library. `make asan` and `make ubsan`
build separate binaries, preventing reuse of a previously built unsanitized test.

The checked-in JSONTestSuite has 318 cases: 95 required-valid, 188
required-invalid and 35 implementation-defined. Acceptance of the last category
depends on the documented UTF-8, depth and signed-integer policies. The fuzz
smoke test checks write/parse/write stability and 2,000 deterministic byte inputs;
it is not a replacement for a long-running libFuzzer campaign.
