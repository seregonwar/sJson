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

Returned child wrappers from lookup/access functions should be released with `cJSON_Delete`; they do not own the arena.
