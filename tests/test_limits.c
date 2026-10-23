/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#define JSON_MAX_ARRAY_LEN 3U
#define JSON_MAX_NODES 128U
#define JSON_MAX_STRING_LEN 8U
#define JSON_IMPLEMENTATION
#include "json_pal.h"
#include <stdio.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    JsonArena* arena = json_arena_create(NULL, 4096);
    JsonValue *arr, *obj;
    JsonError error;
    unsigned i;
    CHECK(arena);
    arr = json_make_array(arena); obj = json_make_object(arena);
    CHECK(arr && obj);
    for (i = 0; i < 3; i++) {
        char key[2] = {(char)('a' + i), 0};
        CHECK(json_arr_push(arr, arena, json_make_int(arena, i)) == JSON_OK);
        CHECK(json_obj_setz(obj, arena, key, json_make_int(arena, i)) == JSON_OK);
    }
    CHECK(json_arr_push(arr, arena, json_make_int(arena, 4)) == JSON_ERR_OVERFLOW);
    CHECK(json_obj_setz(obj, arena, "d", json_make_int(arena, 4)) == JSON_ERR_OVERFLOW);
    CHECK(json_parse_cstr(arena, "[1,2,3,4]", &error) == NULL && error == JSON_ERR_OVERFLOW);
    CHECK(json_parse_cstr(arena, "{\"a\":1,\"b\":2,\"c\":3,\"d\":4}", &error) == NULL && error == JSON_ERR_OVERFLOW);
    CHECK(json_make_stringz(arena, "123456789") == NULL);
    while (arena->node_count < JSON_MAX_NODES) { CHECK(json_make_null(arena)); }
    CHECK(json_parse_cstr(arena, "null", &error) == NULL && error == JSON_ERR_NODE_LIMIT);
    json_arena_destroy(arena);
    return 0;
}
