/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#include "sjson_ffi.h"
#define JSON_IMPLEMENTATION
#include "json_pal.h"
#include <stdlib.h>
struct SJsonDocument { JsonArena* arena; JsonValue* root; };
unsigned sjson_abi_version(void) { return 1U; }
const char* sjson_version(void) { return SJSON_VERSION; }
SJsonDocument* sjson_document_parse(const char* bytes, size_t length, int* error) {
    JsonError status = JSON_ERR_NULL_PARAM;
    SJsonDocument* doc;
    if (error) { *error = status; }
    if (!bytes) { return NULL; }
    doc = (SJsonDocument*)calloc(1, sizeof *doc);
    if (!doc) { if (error) { *error = JSON_ERR_OOM; } return NULL; }
    doc->arena = json_arena_create(NULL, 4096);
    if (doc->arena) { doc->root = json_parse(doc->arena, bytes, length, &status); }
    else { status = JSON_ERR_OOM; }
    if (error) { *error = status; }
    if (!doc->root) { sjson_document_free(doc); return NULL; }
    return doc;
}
void sjson_document_free(SJsonDocument* doc) {
    if (doc) { json_arena_destroy(doc->arena); free(doc); }
}
const SJsonValue* sjson_document_root(const SJsonDocument* doc) { return doc ? doc->root : NULL; }
const SJsonValue* sjson_value_member(const SJsonValue* value, const char* key, size_t length) {
    JsonValue* result = NULL;
    return json_obj_get_n(value, key, length, &result) == JSON_OK ? result : NULL;
}
const SJsonValue* sjson_value_at(const SJsonValue* value, uint32_t index) {
    JsonValue* result = NULL;
    return json_arr_get(value, index, &result) == JSON_OK ? result : NULL;
}
int sjson_value_kind(const SJsonValue* value) { return value ? (int)value->type : -1; }
size_t sjson_value_size(const SJsonValue* value) {
    if (!value) { return 0; }
    switch (value->type) {
    case JSON_ARRAY: return value->v.a.len;
    case JSON_OBJECT: return value->v.o.len;
    case JSON_STRING: return value->v.s.len;
    default: return 0;
    }
}
int sjson_value_bool(const SJsonValue* value, int* output) {
    bool result;
    JsonError status;
    if (!output) { return JSON_ERR_NULL_PARAM; }
    status = json_get_bool(value, &result);
    if (status == JSON_OK) { *output = result; }
    return status;
}
int sjson_value_int(const SJsonValue* value, int64_t* output) { return json_get_int(value, output); }
int sjson_value_number(const SJsonValue* value, double* output) { return json_get_number(value, output); }
int sjson_value_string(const SJsonValue* value, const char** output, size_t* length) {
    uint32_t size;
    JsonError status;
    if (!length) { return JSON_ERR_NULL_PARAM; }
    status = json_get_string(value, output, &size);
    if (status == JSON_OK) { *length = size; }
    return status;
}
int sjson_value_entry(const SJsonValue* value, uint32_t index, const char** key, size_t* length, const SJsonValue** child) {
    JsonValue* result;
    uint32_t size;
    JsonError status;
    if (!key || !length || !child) { return JSON_ERR_NULL_PARAM; }
    status = json_obj_iter(value, index, key, &size, &result);
    if (status == JSON_OK) { *length = size; *child = result; }
    return status;
}
int sjson_value_write(const SJsonValue* value, char* output, size_t capacity, size_t* length) {
    return json_write(value, output, capacity, length, NULL);
}
const char* sjson_error_message(int error) { return json_error_str((JsonError)error); }
