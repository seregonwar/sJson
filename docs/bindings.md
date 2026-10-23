# Language bindings

The C99 header remains usable without a runtime library. C++, Python and Rust
integrations are included and tested. All implementations use the same parser
and writer; none depend on json-c or a replacement JSON parser.

## C++17

Link the `sJson::sjson_cpp` CMake target and define `JSON_IMPLEMENTATION` in
exactly one translation unit, as for the C API:

```cpp
#define JSON_IMPLEMENTATION
#include <sjson.hpp>

sjson::Arena arena;
auto document = arena.parse_replace(R"({"name":"studio","frames":[1,2]})");
if (!document) throw std::runtime_error(arena.last_error_string());
auto name = document.obj("name").string_copy();
auto text = document.serialize();
```

Arena-produced `Value` objects share ownership of storage and may outlive the
`Arena` wrapper. `reset()`, `trim()` and `parse_replace()` invalidate prior
values; their `get()` then returns null and accessors return their defaults.
`parse()` appends allocations, allowing several documents to share one arena.
Use `parse_replace()` for repeated replacement, and `trim()` to release extra
blocks after a large document. Constructors can throw `std::bad_alloc`.

`as_string()` returns a borrowed `std::string_view`: do not keep that view after
reset, trim or release of its last owning value. `string_copy()` returns an
independent string. Explicit `Value(JsonValue*)` handles remain borrowed from
the native caller. Do not mutate or reset a shared arena concurrently.

## Opaque C ABI

Configure `-DSJSON_BUILD_FFI=ON` to build `sjson_ffi`. The header
`bindings/ffi/sjson_ffi.h` exposes document handles and borrowed values without
publishing struct layouts. `sjson_abi_version()` returns 1; `sjson_version()`
reports the library version. Link the exported `sJson::sjson_ffi` target.

`sjson_document_parse(bytes, length, &error)` copies document data into an owned
arena. Always pair a successful result with `sjson_document_free()`. Every value
and string pointer obtained from that document expires when it is freed.
Strings and keys use explicit byte lengths and can contain NUL characters.

`sjson_value_write(value, NULL, 0, &length)` measures the output without its
terminating NUL. Supply `length + 1` bytes for the actual write. Status values
match `JsonError`; missing members return null, while a JSON null member is a
non-null value of kind 0. Shared reads are allowed; freeing a document must
not race a read.

## Python 3.10+

Build the shared ABI, then install the Python wrapper locally:

```sh
cmake -S . -B build -DSJSON_BUILD_FFI=ON -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release
python -m pip install ./bindings/python
```

Set `SJSON_LIBRARY` to the absolute shared-library path. Typical outputs are
`build/libsjson_ffi.so` on Linux, `build/libsjson_ffi.dylib` on macOS,
`build/Release/sjson_ffi.dll` with MSVC, and `build/libsjson_ffi.dll` with MinGW.
System-installed libraries can also be found through `ctypes.util.find_library`.
The Python package does not bundle a precompiled native library.

```python
from sjson import Document, loads, normalize

assert loads('{"a":[1,true,null]}') == {"a": [1, True, None]}
assert normalize(' { "x" : 1 } ') == '{"x":1}'
with Document('{"message":"hello"}') as document:
    copied = document.to_python()
    compact = document.serialize()
```

`loads()` and `normalize()` release native memory before returning. Explicit
documents support `close()` and context managers; results are independent Python
objects. Access after close raises `ValueError`. Operations and close are locked
when a document is shared between threads. Invalid JSON/UTF-8 raises `ValueError`;
native allocation failure raises `MemoryError`.

## Rust

The in-repository `bindings/rust` crate has no crate dependencies. Its build
script builds a static C ABI using CMake and a C compiler:

```sh
cargo test --manifest-path bindings/rust/Cargo.toml
cargo clippy --manifest-path bindings/rust/Cargo.toml --all-targets -- -D warnings
```

```rust
use sjson_native::Document;
let doc = Document::parse(br#"{"frames":[17,true]}"#)?;
let frames = doc.root().get("frames").unwrap();
assert_eq!(frames.at(0).unwrap().as_i64(), Some(17));
```

`Document` frees its arena on drop. `Value<'doc>` and returned `&str` cannot outlive
the owning document; this is checked by a compile-fail doctest. The binding only
exposes immutable reads, so documents and values are `Send + Sync`. `to_json()`
returns an owned UTF-8 string. Numeric conversion reports overflow rather than
casting out-of-range floating-point values.

On MSVC targets the default generator is Visual Studio 2022; override
`CMAKE_GENERATOR` when using another supported generator. Cross-compiling requires
`SJSON_CMAKE_TOOLCHAIN_FILE`. The crate currently builds from the repository and
is deliberately not published to crates.io as a standalone source package.

## Scope

Integers remain signed 64-bit values. Values outside that range are rejected,
including Python integers larger than `int64_t`. Locale-sensitive floating-point
fallbacks inherit the native `strtod`/`snprintf` environment; applications using
another numeric locale should supply a suitable PAL or use a C numeric locale.
No cross-language performance advantage is claimed without measurements.
