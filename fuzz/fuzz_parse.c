/* SPDX-License-Identifier: GPL-3.0-only OR MIT */

#define JSON_IMPLEMENTATION
#include "../src/json_pal.h"

#include <stdint.h>
#include <stddef.h>

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    JsonArena* arena;
    JsonError err = JSON_OK;
    JsonValue* root;
    char buf[4096];
    size_t written = 0U;

    if (size > ((size_t)-1 - 65536U) / 4U) { return 0; }
    arena = json_arena_create(NULL, size * 4U + 65536U);
    if (arena == NULL) { return 0; }

    root = json_parse(arena, (const char*)data, size, &err);
    if (root != NULL && err == JSON_OK) {
        if (json_measure(root, &written, NULL) != JSON_OK) { abort(); }
        if (written < sizeof(buf)) {
            JsonArena* second = json_arena_create(NULL, 65536);
            JsonValue* roundtrip;
            char result[4096];
            size_t result_size;
            if (!second) { json_arena_destroy(arena); return 0; }
            if (json_write(root, buf, sizeof(buf), &written, NULL) != JSON_OK) { abort(); }
            roundtrip = json_parse(second, buf, written, &err);
            if (!roundtrip || json_write(roundtrip, result, sizeof result, &result_size, NULL) != JSON_OK ||
                result_size != written || memcmp(buf, result, written) != 0) { abort(); }
            json_arena_destroy(second);
        }
    }

    json_arena_destroy(arena);
    return 0;
}

#ifndef SJSON_LIBFUZZER
int main(void) {
    const char* cases[] = {"{\"x\":[1,2,3],\"ok\":true}", "\"\\u0000\\ud834\\udd1e\"",
        "-9223372036854775808", "1.23456789e100", "[1,]", "\"\xc0\xaf\"", "{\"a\":1,\"a\":2}"};
    uint32_t state = 17U;
    size_t i, j;
    uint8_t bytes[256];
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        LLVMFuzzerTestOneInput((const uint8_t*)cases[i], strlen(cases[i]));
    }
    for (i = 0; i < 2000; i++) {
        for (j = 0; j < sizeof bytes; j++) { state = state * 1664525U + 1013904223U; bytes[j] = (uint8_t)(state >> 24U); }
        LLVMFuzzerTestOneInput(bytes, i % sizeof bytes);
    }
    return 0;
}
#endif
