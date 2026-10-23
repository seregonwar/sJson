#ifndef SJSON_CJSON_COMPAT_H
#define SJSON_CJSON_COMPAT_H

/* SPDX-License-Identifier: GPL-3.0-only OR MIT */

#include <stdlib.h>
#include <string.h>
#ifndef JSON_PAL_H
#include <json_pal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SJSON_CJSON_API
#define SJSON_CJSON_API static inline
#endif
#ifndef SJSON_CJSON_MALLOC
#define SJSON_CJSON_MALLOC malloc
#endif
#ifndef SJSON_CJSON_FREE
#define SJSON_CJSON_FREE free
#endif

#define cJSON_False  0
#define cJSON_True   1
#define cJSON_NULL   2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array  5
#define cJSON_Object 6

typedef struct sjson_cjson_owner { JsonArena* arena; size_t references; } sjson_cjson_owner;
typedef struct cJSON {
    JsonArena* arena;
    JsonValue* value;
    int owns_arena;
    int type;
    int valueint;
    double valuedouble;
    char* valuestring;
    sjson_cjson_owner* owner;
} cJSON;

SJSON_CJSON_API cJSON* sjson_cjson_wrap(sjson_cjson_owner* owner, JsonValue* value, int owns_arena) {
    cJSON* item = (cJSON*)SJSON_CJSON_MALLOC(sizeof(cJSON));
    if (item == NULL) { return NULL; }
    memset(item, 0, sizeof(*item));
    item->arena = owner->arena;
    item->owner = owner;
    owner->references++;
    item->value = value;
    item->owns_arena = owns_arena;
    if (value == NULL) { return item; }

    switch (value->type) {
    case JSON_NULL:
        item->type = cJSON_NULL;
        break;
    case JSON_BOOL:
        item->type = value->v.b ? cJSON_True : cJSON_False;
        item->valueint = value->v.b ? 1 : 0;
        item->valuedouble = value->v.b ? 1.0 : 0.0;
        break;
    case JSON_INTEGER:
        item->type = cJSON_Number;
        item->valueint = value->v.i > INT_MAX ? INT_MAX : value->v.i < INT_MIN ? INT_MIN : (int)value->v.i;
        item->valuedouble = (double)value->v.i;
        break;
    case JSON_FLOAT:
        item->type = cJSON_Number;
        item->valueint = value->v.f >= INT_MAX ? INT_MAX : value->v.f <= INT_MIN ? INT_MIN : (int)value->v.f;
        item->valuedouble = value->v.f;
        break;
    case JSON_STRING:
        item->type = cJSON_String;
        break;
    case JSON_ARRAY:
        item->type = cJSON_Array;
        break;
    case JSON_OBJECT:
        item->type = cJSON_Object;
        break;
    default:
        break;
    }
    return item;
}

SJSON_CJSON_API cJSON* cJSON_ParseWithLength(const char* value, size_t buffer_length) {
    JsonArena* arena;
    JsonError err = JSON_OK;
    JsonValue* root;
    sjson_cjson_owner* owner;
    cJSON* result;
    if (value == NULL) { return NULL; }
    if (buffer_length && value[buffer_length - 1U] == '\0') { buffer_length--; }
    if (buffer_length > ((size_t)-1 - 65536U) / 3U) { return NULL; }
    arena = json_arena_create(NULL, buffer_length * 3U + 65536U);
    if (arena == NULL) { return NULL; }
    root = json_parse(arena, value, buffer_length, &err);
    if (root == NULL || err != JSON_OK) {
        json_arena_destroy(arena);
        return NULL;
    }
    owner = (sjson_cjson_owner*)SJSON_CJSON_MALLOC(sizeof *owner);
    if (!owner) { json_arena_destroy(arena); return NULL; }
    owner->arena = arena; owner->references = 0;
    result = sjson_cjson_wrap(owner, root, 1);
    if (!result) { json_arena_destroy(arena); SJSON_CJSON_FREE(owner); }
    return result;
}

SJSON_CJSON_API cJSON* cJSON_Parse(const char* value) {
    return value ? cJSON_ParseWithLength(value, strlen(value)) : NULL;
}

SJSON_CJSON_API void cJSON_Delete(cJSON* item) {
    if (item == NULL) { return; }
    if (--item->owner->references == 0U) {
        json_arena_destroy(item->arena);
        SJSON_CJSON_FREE(item->owner);
    }
    SJSON_CJSON_FREE(item);
}

SJSON_CJSON_API int cJSON_IsInvalid(const cJSON* item) { return item == NULL || item->value == NULL; }
SJSON_CJSON_API int cJSON_IsFalse(const cJSON* item) { return item != NULL && item->value != NULL && item->value->type == JSON_BOOL && !item->value->v.b; }
SJSON_CJSON_API int cJSON_IsTrue(const cJSON* item) { return item != NULL && item->value != NULL && item->value->type == JSON_BOOL && item->value->v.b; }
SJSON_CJSON_API int cJSON_IsBool(const cJSON* item) { return item != NULL && item->value != NULL && item->value->type == JSON_BOOL; }
SJSON_CJSON_API int cJSON_IsNull(const cJSON* item) { return item != NULL && item->value != NULL && item->value->type == JSON_NULL; }
SJSON_CJSON_API int cJSON_IsNumber(const cJSON* item) { return item != NULL && item->value != NULL && (item->value->type == JSON_INTEGER || item->value->type == JSON_FLOAT); }
SJSON_CJSON_API int cJSON_IsString(const cJSON* item) { return item != NULL && item->value != NULL && item->value->type == JSON_STRING; }
SJSON_CJSON_API int cJSON_IsArray(const cJSON* item) { return item != NULL && item->value != NULL && item->value->type == JSON_ARRAY; }
SJSON_CJSON_API int cJSON_IsObject(const cJSON* item) { return item != NULL && item->value != NULL && item->value->type == JSON_OBJECT; }

SJSON_CJSON_API int cJSON_GetArraySize(const cJSON* array) {
    uint32_t len = 0U;
    if (array == NULL || json_get_arr_len(array->value, &len) != JSON_OK) { return 0; }
    return (int)len;
}

SJSON_CJSON_API cJSON* cJSON_GetArrayItem(const cJSON* array, int index) {
    JsonValue* out = NULL;
    if (array == NULL || index < 0) { return NULL; }
    if (json_arr_get(array->value, (uint32_t)index, &out) != JSON_OK) { return NULL; }
    return sjson_cjson_wrap(array->owner, out, 0);
}

SJSON_CJSON_API cJSON* cJSON_GetObjectItemCaseSensitive(const cJSON* object, const char* string) {
    JsonValue* out = NULL;
    if (object == NULL || string == NULL) { return NULL; }
    if (json_obj_get(object->value, string, &out) != JSON_OK) { return NULL; }
    return sjson_cjson_wrap(object->owner, out, 0);
}

SJSON_CJSON_API cJSON* cJSON_GetObjectItem(const cJSON* object, const char* string) {
    return cJSON_GetObjectItemCaseSensitive(object, string);
}

SJSON_CJSON_API char* cJSON_GetStringValue(const cJSON* item) {
    const char* data = NULL;
    uint32_t len = 0U;
    char* out;
    if (item == NULL || json_get_string(item->value, &data, &len) != JSON_OK) { return NULL; }
    if (item->valuestring) { return item->valuestring; }
    out = (char*)json_arena_alloc(item->arena, (size_t)len + 1U);
    if (out == NULL) { return NULL; }
    if (len > 0U) { memcpy(out, data, len); }
    out[len] = '\0';
    ((cJSON*)item)->valuestring = out;
    return out;
}

SJSON_CJSON_API double cJSON_GetNumberValue(const cJSON* item) {
    double out = 0.0;
    if (item == NULL) { return 0.0; }
    json_get_number(item->value, &out);
    return out;
}

SJSON_CJSON_API char* cJSON_PrintUnformatted(const cJSON* item) {
    size_t len = 0U;
    char* out;
    if (item == NULL || item->value == NULL) { return NULL; }
    if (json_measure(item->value, &len, NULL) != JSON_OK) { return NULL; }
    if (len == (size_t)-1) { return NULL; }
    out = (char*)SJSON_CJSON_MALLOC(len + 1U);
    if (out == NULL) { return NULL; }
    if (json_write(item->value, out, len + 1U, NULL, NULL) != JSON_OK) {
        SJSON_CJSON_FREE(out);
        return NULL;
    }
    return out;
}

SJSON_CJSON_API char* cJSON_Print(const cJSON* item) {
    JsonWriteOpts opts;
    size_t len = 0U;
    char* out;
    if (item == NULL || item->value == NULL) { return NULL; }
    memset(&opts, 0, sizeof(opts));
    opts.pretty = true;
    opts.indent = 2U;
    if (json_measure(item->value, &len, &opts) != JSON_OK) { return NULL; }
    if (len == (size_t)-1) { return NULL; }
    out = (char*)SJSON_CJSON_MALLOC(len + 1U);
    if (out == NULL) { return NULL; }
    if (json_write(item->value, out, len + 1U, NULL, &opts) != JSON_OK) {
        SJSON_CJSON_FREE(out);
        return NULL;
    }
    return out;
}

SJSON_CJSON_API void cJSON_free(void* object) {
    SJSON_CJSON_FREE(object);
}

#ifdef __cplusplus
}
#endif

#endif
