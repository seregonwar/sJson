/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#include "sjson_ffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    const char input[] = "{\"s\":\"a\\u0000b\",\"a\":[null,17]}";
    int error = 0;
    SJsonDocument* doc = sjson_document_parse(input, sizeof input - 1, &error);
    const SJsonValue *root, *value, *child;
    const char *bytes, *key;
    size_t size, length;
    int64_t number;
    char* output;
    CHECK(sjson_abi_version() == 1 && strcmp(sjson_version(), "2.0.0") == 0);
    CHECK(doc && error == 0);
    root = sjson_document_root(doc);
    CHECK(sjson_value_kind(root) == 6 && sjson_value_size(root) == 2);
    value = sjson_value_member(root, "s", 1);
    CHECK(value && sjson_value_string(value, &bytes, &size) == 0);
    CHECK(size == 3 && memcmp(bytes, "a\0b", 3) == 0);
    CHECK(sjson_value_entry(root, 1, &key, &length, &child) == 0 && length == 1 && key[0] == 'a');
    CHECK(sjson_value_kind(sjson_value_at(child, 0)) == 0);
    CHECK(sjson_value_int(sjson_value_at(child, 1), &number) == 0 && number == 17);
    CHECK(sjson_value_member(root, "missing", 7) == NULL);
    CHECK(sjson_value_write(root, NULL, 0, &size) == 0);
    output = (char*)malloc(size + 1);
    CHECK(output && sjson_value_write(root, output, size + 1, &length) == 0 && length == size);
    CHECK(strcmp(output, input) == 0);
    free(output);
    sjson_document_free(doc);
    CHECK(sjson_document_parse("{", 1, &error) == NULL && error != 0);
    sjson_document_free(NULL);
    CHECK(sjson_value_kind(NULL) == -1);
    return 0;
}
