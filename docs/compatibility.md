# Compatibility Wrappers

Compatibility wrappers are migration aids. They are source-level adapters, not ABI-compatible replacements for third-party shared libraries.

## cJSON subset

Header:

```c
#include "compat/cjson/sjson_cjson_compat.h"
```

Supported common read-path APIs include:

- `cJSON_Parse`, `cJSON_ParseWithLength`;
- `cJSON_Delete`;
- type predicates such as `cJSON_IsObject`, `cJSON_IsArray`, `cJSON_IsNumber`;
- `cJSON_GetObjectItem`, `cJSON_GetObjectItemCaseSensitive`;
- `cJSON_GetArrayItem`, `cJSON_GetArraySize`;
- `cJSON_GetStringValue`, `cJSON_GetNumberValue`;
- `cJSON_Print`, `cJSON_PrintUnformatted`, `cJSON_free`.

Each returned child wrapper must be released with `cJSON_Delete`. Wrappers share
ownership of the arena, so a retained child stays valid after the root wrapper
is deleted. This differs from real cJSON's borrowed-child convention.

`cJSON_GetStringValue` caches one NUL-terminated view per wrapper; repeated calls
do not grow the arena. Do not free that pointer. Printed strings are separate
allocations and must be paired with `cJSON_free`. The optional
`SJSON_CJSON_MALLOC`/`SJSON_CJSON_FREE` macros must be a matching allocator pair.

The header is installed under `sjson/compat/cjson`; use the include directory
provided by the `sJson::sjson` CMake target. Numeric `valueint` fields saturate to
the native `int` range, avoiding undefined floating-point casts.
