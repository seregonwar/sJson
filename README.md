# sJson — Safe single-header JSON library in C

**sJson** — also known as **Secure JSON** — is a safe, portable, dependency-free JSON parser and writer written in **C99**.

It is designed as a **single-header**, arena-based library with a small Platform Abstraction Layer (PAL), making it suitable for desktop, embedded, freestanding, and cross-platform environments.

Version **1.1.0** · **GPL-3.0 license**

## Highlights

* **Single-header integration** — define `JSON_IMPLEMENTATION` in one translation unit.
* **No third-party dependencies** — only the C standard library by default.
* **Freestanding-friendly** — define `JSON_NO_STDLIB` and provide PAL overrides to avoid standard-library headers.
* **C99 compatible** — designed for portable ISO C99 builds.
* **C++ friendly** — usable from C++ projects through a C-compatible API.
* **Arena allocator** — bulk allocation, zero fragmentation, fast reset/destroy.
* **Iterative parser** — no C recursion, bounded parsing depth.
* **Strict JSON handling** — RFC 8259-style parsing, UTF-8 validation, `\uXXXX` decoding and surrogate-pair support.
* **Fast object lookup** — insertion-order storage with optional sorted index for binary search.
* **Cross-compiler support** — MSVC, GCC and Clang.

## Features

| Category          | Detail                                                                              |
| ----------------- | ----------------------------------------------------------------------------------- |
| **Parsing**       | RFC 8259-style JSON parsing, iterative state machine, no C recursion                |
| **Safety limits** | Depth-limited, node-limited, string-length-limited, array/object-length-limited     |
| **Numbers**       | Integers stored exactly as `int64_t`; floating-point values stored as `double`      |
| **Strings**       | UTF-8 validation, two-pass decoding, full `\uXXXX` handling with surrogate pairs    |
| **Objects**       | Insertion-order storage plus on-demand sorted index                                 |
| **Lookup**        | O(log n) object lookup after `json_obj_finalize`; O(n) before finalization          |
| **Memory**        | Arena allocator, bulk free, zero fragmentation, pluggable allocator hooks           |
| **Dependencies**  | No third-party dependencies; standard library can be disabled with `JSON_NO_STDLIB` |
| **Portability**   | C99, freestanding-friendly, MSVC/GCC/Clang                                          |
| **C++**           | C-compatible API usable from C++ projects                                           |

## Quick start

```c
// In ONE translation unit:
#define JSON_IMPLEMENTATION
#include "json_pal.h"

JsonArena* arena = json_arena_create(NULL, 64 * 1024);
JsonError  err;
JsonValue* root  = json_parse_cstr(arena, "{\"x\":1}", &err);

if (!root) {
    // Handle parse error.
    json_arena_destroy(arena);
    return 1;
}

json_obj_finalize(arena, root);       // build sorted index

JsonValue* v;
json_obj_get(root, "x", &v);          // O(log n) lookup after finalize

int64_t x;
json_get_int(v, &x);                  // x == 1

json_arena_destroy(arena);
```

## C++ usage

sJson exposes a C-compatible API and can be used from C++ projects.

Recommended layout:

```c
// sjson_impl.c
#define JSON_IMPLEMENTATION
#include "json_pal.h"
```

```cpp
// app.cpp
extern "C" {
#include "json_pal.h"
}

int main() {
    JsonArena* arena = json_arena_create(nullptr, 64 * 1024);
    JsonError err;
    JsonValue* root = json_parse_cstr(arena, "{\"cpp\":true}", &err);

    json_arena_destroy(arena);
    return root ? 0 : 1;
}
```

## PAL overrides

Define these before including `json_pal.h` to replace platform primitives.

```c
#define JSON_MEMCPY   my_memcpy
#define JSON_MEMSET   my_memset
#define JSON_MEMCMP   my_memcmp
#define JSON_STRLEN   my_strlen
#define JSON_ASSERT   my_assert
#define JSON_STRTOD   my_strtod
#define JSON_STRTOLL  my_strtoll

#define JSON_NO_STDLIB          // exclude standard-library headers
```

With `JSON_NO_STDLIB`, sJson does not include standard-library headers and relies on the user-provided PAL definitions and minimal built-in type definitions.

## Configuration

Define configuration macros before including `json_pal.h`.

```c
#define JSON_MAX_DEPTH        64       // max nesting depth
#define JSON_MAX_STRING_LEN   1048576  // max decoded string length
#define JSON_MAX_NODES        1048576  // max value nodes per parse
#define JSON_MAX_ARRAY_LEN    1048576  // max items per array/object
#define JSON_INTROSORT_THRESH 16       // insertion sort threshold
#define JSON_ARENA_ALIGN      8        // allocation alignment
```

## Path syntax

```c
json_path(root, "users[0].name",       NULL);
json_path(root, "config[\"host\"]",    NULL);
json_path(root, "[2][1]",              NULL);
```

Supported path forms:

| Syntax    | Meaning                           |
| --------- | --------------------------------- |
| `.key`    | object key lookup                 |
| `["key"]` | object key lookup with quoted key |
| `[N]`     | array index lookup                |

## Memory model

sJson uses an arena allocator:

* all parsed values live inside the arena;
* allocation is fast and compact;
* individual values are not freed separately;
* the whole parse tree is released with `json_arena_destroy`;
* repeated parses can reuse memory with `json_arena_reset`.

Use one arena per independent parse or per thread.

## Threading note

sJson does not require global mutable parser state. For concurrent use, give each thread its own `JsonArena` and parsed document. Do not mutate or reset the same arena from multiple threads at the same time.

## Build & test

```sh
make test          # basic build
make asan          # AddressSanitizer + UBSan
cmake -B build && cmake --build build && ctest --test-dir build
```

## License

sJson is released under the **GPL-3.0 license**.
