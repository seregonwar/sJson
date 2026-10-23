# Changelog

## 2.0.0 (unreleased)

- Fixed retained arena blocks being orphaned after reset; added `json_arena_trim`.
- Checked allocation arithmetic, float-to-int conversions and path indices.
- Validated raw UTF-8, corrected ASCII-only Unicode output, and fixed empty keys.
- Made sorted output lexical without requiring prior finalization; bounded
  writer, equality and clone traversal for constructed cyclic/deep values.
- Matched writer measurement to trailing-newline and zero-capacity behavior.
- Added an opaque C ABI, Python ctypes package and safe Rust crate.
- Extended the C++17 wrapper with shared storage, invalidation checks,
  replacement parsing, trimming, cloning and serialization.
- Fixed cJSON-wrapper allocation failure cleanup, retained child lifetimes,
  repeated string extraction and saturated numeric conversion.
- Added allocation/failure, configured-limit, ownership, binding, corpus and
  fuzz regression tests; isolated sanitizer binaries from normal Make builds.
- Added Windows/macOS/Linux CI, Rust checks and Linux sanitizer checks.
- Removed decorative implementation banners and speculative performance comments.
- Aligned CMake/package version metadata with the documented 2.0.0 version.

## 1.1.0

- Added arena-backed parser/writer API.
- Added regression suite and JSONTestSuite corpus runner.
- Added native and comparative benchmarks.
- Added official asset benchmark support.
- Added fast integer and floating-point parsing paths.
- Added initial C++ RAII wrapper.
- Added source-level cJSON compatibility subset.
