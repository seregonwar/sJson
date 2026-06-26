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

    arena = json_arena_create(NULL, size * 4U + 65536U);
    if (arena == NULL) { return 0; }

    root = json_parse(arena, (const char*)data, size, &err);
    if (root != NULL && err == JSON_OK) {
        (void)json_measure(root, &written, NULL);
        if (written < sizeof(buf)) {
            (void)json_write(root, buf, sizeof(buf), NULL, NULL);
        }
    }

    json_arena_destroy(arena);
    return 0;
}

#ifndef SJSON_LIBFUZZER
int main(void) {
    static const uint8_t smoke[] = "{\"x\":[1,2,3],\"ok\":true}";
    return LLVMFuzzerTestOneInput(smoke, sizeof(smoke) - 1U);
}
#endif
