/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#define JSON_IMPLEMENTATION
#include "json_pal.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
typedef union Header { size_t size; long double align; void* ptr; } Header;
typedef struct { size_t calls, blocks, bytes, fail_at; } Allocations;
static void* tracked_alloc(void* opaque, size_t size) {
    Allocations* stats = (Allocations*)opaque;
    Header* header;
    size_t call = stats->calls++;
    if (call == stats->fail_at || size > (size_t)-1 - sizeof(Header)) { return NULL; }
    header = (Header*)malloc(sizeof(Header) + size);
    if (!header) { return NULL; }
    header->size = size;
    stats->blocks++; stats->bytes += size;
    memset(header + 1, 0xA5, size);
    return header + 1;
}
static void tracked_free(void* opaque, void* ptr) {
    Allocations* stats = (Allocations*)opaque;
    Header* header;
    if (!ptr) { return; }
    header = (Header*)ptr - 1;
    stats->blocks--; stats->bytes -= header->size;
    free(header);
}
static int arena_reuse(void) {
    Allocations stats = {0, 0, 0, (size_t)-1};
    JsonAllocator allocator = {tracked_alloc, tracked_free, NULL, &stats};
    JsonArena* arena = json_arena_create(&allocator, 4096);
    size_t cycle, calls = 0, i;
    const size_t sizes[] = {2048, 8192, 64, 16384, 4096};
    CHECK(arena);
    for (cycle = 0; cycle < 200; cycle++) {
        json_arena_reset(arena);
        for (i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
            unsigned char* block = (unsigned char*)json_arena_alloc(arena, sizes[i]);
            CHECK(block && block[0] == 0 && block[sizes[i]-1] == 0);
            block[sizes[i]-1] = 42;
        }
        if (cycle == 0) { calls = stats.calls; }
        CHECK(stats.calls == calls);
    }
    calls = stats.calls;
    CHECK(json_arena_alloc(arena, (size_t)-1) == NULL && stats.calls == calls);
    CHECK(json_arena_alloc(arena, (size_t)-16) == NULL && stats.calls == calls);
    json_arena_trim(arena);
    CHECK(stats.blocks == 2 && arena->total_bytes == 0 && arena->node_count == 0);
    json_arena_destroy(arena);
    CHECK(stats.blocks == 0 && stats.bytes == 0);
    return 0;
}
static int allocation_failures(void) {
    size_t failure;
    bool succeeded = false;
    for (failure = 0; failure < 16; failure++) {
        Allocations stats = {0, 0, 0, failure};
        JsonAllocator allocator = {tracked_alloc, tracked_free, NULL, &stats};
        JsonArena* arena = json_arena_create(&allocator, 4096);
        if (arena) {
            size_t i;
            JsonValue* array = json_make_array(arena);
            for (i = 0; array && i < 300; i++) {
                JsonValue* item = json_make_int(arena, (int64_t)i);
                if (!item || json_arr_push(array, arena, item) != JSON_OK) { break; }
            }
            if (array && i == 300) { succeeded = true; }
            json_arena_destroy(arena);
        }
        CHECK(stats.blocks == 0 && stats.bytes == 0);
    }
    CHECK(succeeded);
    return 0;
}
static int unicode_and_writer(void) {
    JsonArena* arena = json_arena_create(NULL, 4096);
    JsonError error;
    const char* malformed[] = {"\"\xc0\xaf\"", "\"\xed\xa0\x80\"", "\"\xf4\x90\x80\x80\"",
        "\"\x80\"", "\"\xe2\x82\"", "\"\\n\xc0\x80\"", "{\"\xc0\xaf\":1}"};
    const char* valid = "\"\xc3\xa9\xe4\xb8\xad\xf0\x9d\x84\x9e\"";
    JsonValue *value, *roundtrip;
    JsonWriteOpts options = {false, 2, false, true, false};
    char output[128], sentinel = 'x';
    size_t i, measured, written;
    CHECK(arena);
    for (i = 0; i < sizeof(malformed)/sizeof(malformed[0]); i++) {
        CHECK(json_parse_cstr(arena, malformed[i], &error) == NULL && error == JSON_ERR_INVALID_UTF8);
    }
    CHECK(json_make_string(arena, "\xc0\xaf", 2) == NULL);
    value = json_parse_cstr(arena, valid, &error);
    CHECK(value && error == JSON_OK);
    CHECK(json_write(value, output, sizeof output, &written, &options) == JSON_OK);
    CHECK(strcmp(output, "\"\\u00e9\\u4e2d\\ud834\\udd1e\"") == 0);
    roundtrip = json_parse_cstr(arena, output, &error);
    CHECK(roundtrip && json_equal(value, roundtrip));
    CHECK(json_write(value, &sentinel, 0, NULL, NULL) == JSON_ERR_BUFFER_TOO_SMALL && sentinel == 'x');
    options.trailing_nl = true;
    CHECK(json_measure(value, &measured, &options) == JSON_OK);
    CHECK(json_write(value, output, measured + 1, &written, &options) == JSON_OK);
    CHECK(written == measured && output[written-1] == '\n' && output[written] == '\0');
    CHECK(json_write(value, output, measured, &written, &options) == JSON_ERR_BUFFER_TOO_SMALL);
    {
        JsonValue* object = json_parse_cstr(arena, "{\"z\":0,\"q\":1,\"a\":2}", &error);
        JsonWriteOpts sorted = {false, 2, true, false, false};
        CHECK(object && json_write(object, output, sizeof output, NULL, &sorted) == JSON_OK);
        CHECK(strcmp(output, "{\"a\":2,\"q\":1,\"z\":0}") == 0);
        CHECK(json_obj_finalize(arena, object) == JSON_OK);
        CHECK(json_write(object, output, sizeof output, NULL, &sorted) == JSON_OK);
        CHECK(strcmp(output, "{\"a\":2,\"q\":1,\"z\":0}") == 0);
        CHECK(json_measure(object, &measured, &sorted) == JSON_OK);
        for (i = 0; i <= measured; i++) {
            CHECK(json_write(object, output, i, NULL, &sorted) == JSON_ERR_BUFFER_TOO_SMALL);
        }
    }
    json_arena_destroy(arena);
    return 0;
}
static int numeric_and_depth(void) {
    JsonArena* arena = json_arena_create(NULL, 4096);
    JsonValue* number = json_make_float(arena, 1e100);
    JsonValue* array = json_make_array(arena);
    JsonError error;
    int64_t output = 123;
    size_t size;
    CHECK(number && array);
    CHECK(json_get_int(number, &output) == JSON_ERR_OVERFLOW && output == 123);
    CHECK(json_path(array, "[8589934591]", &error) == NULL && error == JSON_ERR_OVERFLOW);
    CHECK(json_arr_push(array, arena, array) == JSON_OK);
    CHECK(json_measure(array, &size, NULL) == JSON_ERR_DEPTH_EXCEEDED);
    CHECK(json_clone(arena, array) == NULL);
    {
        JsonValue* second = json_make_array(arena);
        CHECK(second && json_arr_push(second, arena, second) == JSON_OK);
        CHECK(!json_equal(array, second));
    }
    {
        JsonValue* empty_key = json_make_object(arena);
        JsonValue* clone;
        JsonValue* found = NULL;
        CHECK(empty_key && json_obj_set(empty_key, arena, NULL, 0, json_make_int(arena, 3)) == JSON_OK);
        CHECK(json_obj_get(empty_key, "", &found) == JSON_OK && found->v.i == 3);
        clone = json_clone(arena, empty_key);
        CHECK(clone && json_equal(empty_key, clone));
        CHECK(json_obj_finalize(arena, empty_key) == JSON_OK);
        CHECK(json_obj_get(empty_key, "", &found) == JSON_OK);
        CHECK(json_equal(json_make_stringz(arena, ""), json_parse_cstr(arena, "\"\"", &error)));
    }
    json_arena_destroy(arena);
    return 0;
}
int main(void) {
    int failed = arena_reuse() + allocation_failures() + unicode_and_writer() + numeric_and_depth();
    printf("Reliability: %s\n", failed ? "FAIL" : "PASS");
    return failed != 0;
}
