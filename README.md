# sJson — Safe single-header JSON library in C
sJson (also known as Secure JSON) is a thread-safe, dynamic-allocation-free, cross-compilable JSON parser based on a PAL (Platform Abstraction Layer) architecture writed in c99

Version **1.1.0** ·  GPL-3.0 license

### Features

| Category | Detail |
|---|---|
| **Parsing** | RFC 8259, iterative (no C recursion), depth-limited, node-limited |
| **Numbers** | Integer → `int64_t` exact; Float → `double` (NaN/Inf rejected) |
| **Strings** | Two-pass, exact UTF-8 sizing, full `\uXXXX` + surrogate pairs |
| **Objects** | Insertion-order storage + on-demand introsort index (O(n log n)) |
| **Lookup** | O(log n) binary search after `json_obj_finalize`; O(n) before |
| **Memory** | Arena allocator, zero fragmentation, pluggable hooks |
| **Platform** | C99, freestanding (`JSON_NO_STDLIB`), MSVC/GCC/Clang |

### Quick start

```c
// In ONE translation unit:
#define JSON_IMPLEMENTATION
#include "json_pal.h"

JsonArena* arena = json_arena_create(NULL, 64 * 1024);
JsonError  err;
JsonValue* root  = json_parse_cstr(arena, "{\"x\":1}", &err);

json_obj_finalize(arena, root);       // build sorted index

JsonValue* v;
json_obj_get(root, "x", &v);          // O(log n) lookup
int64_t x; json_get_int(v, &x);       // x == 1

json_arena_destroy(arena);
```

### PAL overrides

```c
#define JSON_MEMCPY   my_memcpy
#define JSON_MEMSET   my_memset
#define JSON_MEMCMP   my_memcmp
#define JSON_STRLEN   my_strlen
#define JSON_ASSERT   my_assert
#define JSON_STRTOD   my_strtod
#define JSON_STRTOLL  my_strtoll
#define JSON_NO_STDLIB          // exclude all stdlib headers
```

### Configuration

```c
#define JSON_MAX_DEPTH        64       // max nesting depth
#define JSON_MAX_STRING_LEN   1048576  // max decoded string length
#define JSON_MAX_NODES        1048576  // max value nodes per parse
#define JSON_MAX_ARRAY_LEN    1048576  // max items per array/object
#define JSON_INTROSORT_THRESH 16       // insertion sort threshold
#define JSON_ARENA_ALIGN      8        // allocation alignment
```

### Path syntax

```c
json_path(root, "users[0].name",       NULL);
json_path(root, "config[\"host\"]",    NULL);
json_path(root, "[2][1]",              NULL);
```

### Build & test

```sh
make test          # basic build
make asan          # AddressSanitizer + UBSan
cmake -B build && cmake --build build && ctest --test-dir build
```

### Bug fixes in v1.1

| # | Description |
|---|---|
| 1 | Parser: spurious extra `PS_OBJ_KEY` frame push caused all non-empty objects to fail |
| 2 | String decoder: `\uXXXX` BMP size estimated as constant 3; now uses exact byte count |
| 3 | Missing `JSON_MAX_ARRAY_LEN` macro definition (compile error) |
| 4 | Missing `#include <stdio.h>` for `snprintf` in implementation block |
| 5 | Removed unused internal `json__utf8_advance` function |
