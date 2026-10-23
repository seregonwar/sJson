/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#define JSON_IMPLEMENTATION
#include "json_pal.h"
#include <stdlib.h>
#include <stdio.h>
static size_t calls, fail_at = (size_t)-1, live;
static void* counted_alloc(size_t size) {
    void* result;
    if (calls++ == fail_at) { return NULL; }
    result = malloc(size);
    if (result) { live++; }
    return result;
}
static void counted_free(void* value) { if (value) { live--; free(value); } }
#define SJSON_CJSON_MALLOC counted_alloc
#define SJSON_CJSON_FREE counted_free
#include "sjson_cjson_compat.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    cJSON *root, *child, *number;
    char *first, *printed;
    size_t bytes, i;
    for (i = 0; i < 2; i++) {
        calls = 0; fail_at = i;
        CHECK(cJSON_Parse("{\"x\":1}") == NULL && live == 0);
    }
    fail_at = (size_t)-1;
    root = cJSON_ParseWithLength("{\"s\":\"hello\",\"n\":1e100}", sizeof("{\"s\":\"hello\",\"n\":1e100}"));
    CHECK(root);
    child = cJSON_GetObjectItem(root, "s");
    number = cJSON_GetObjectItem(root, "n");
    CHECK(child && number && number->valueint == INT_MAX);
    first = cJSON_GetStringValue(child);
    CHECK(first && strcmp(first, "hello") == 0);
    bytes = child->arena->total_bytes;
    for (i = 0; i < 1000; i++) { CHECK(cJSON_GetStringValue(child) == first); }
    CHECK(child->arena->total_bytes == bytes);
    cJSON_Delete(root);
    CHECK(strcmp(cJSON_GetStringValue(child), "hello") == 0);
    printed = cJSON_PrintUnformatted(number);
    CHECK(printed);
    cJSON_free(printed);
    cJSON_Delete(number);
    cJSON_Delete(child);
    CHECK(live == 0);
    return 0;
}
