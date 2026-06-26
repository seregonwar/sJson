/*
 * sJson v1.1.0 — safe, fast, single-header JSON library in C99.
 * SPDX-License-Identifier: GPL-3.0-only OR MIT
 *
 * This file is dual-licensed under GPL-3.0-only OR MIT.
 * You may choose either license.
 */

#define JSON_IMPLEMENTATION
#include "json_pal.h"

#include <stdio.h>
#include <string.h>

#define T(name) printf("[TEST] %-58s", name)
#define PASS()  do { printf("PASS\n"); } while (0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return 1; } while (0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); } } while (0)
#define CHECK_OK(expr) do { JsonError e__ = (expr); if (e__ != JSON_OK) { FAIL(json_error_str(e__)); } } while (0)

static int test_primitives(void) {
    JsonArena* a = json_arena_create(NULL, 4096);
    JsonError err = JSON_OK;
    JsonValue* v = NULL;
    bool b = false;
    int64_t i = 0;
    double f = 0.0;
    const char* s = NULL;
    uint32_t sl = 0;

    CHECK(a != NULL, "arena");

    T("null");
    v = json_parse_cstr(a, "null", &err);
    CHECK(v != NULL && err == JSON_OK && v->type == JSON_NULL, "null");
    PASS();

    T("true/false");
    v = json_parse_cstr(a, "true", &err);
    CHECK(v != NULL && json_get_bool(v, &b) == JSON_OK && b, "true");
    v = json_parse_cstr(a, "false", &err);
    CHECK(v != NULL && json_get_bool(v, &b) == JSON_OK && !b, "false");
    PASS();

    T("int64 boundaries");
    v = json_parse_cstr(a, "9223372036854775807", &err);
    CHECK(v != NULL && json_get_int(v, &i) == JSON_OK && i == 9223372036854775807LL, "int64 max");
    v = json_parse_cstr(a, "-9223372036854775807", &err);
    CHECK(v != NULL && json_get_int(v, &i) == JSON_OK && i == -9223372036854775807LL, "int64 negative");
    PASS();

    T("float");
    v = json_parse_cstr(a, "3.14159265358979", &err);
    CHECK(v != NULL && json_get_float(v, &f) == JSON_OK && f > 3.14 && f < 3.15, "float");
    PASS();

    T("fast decimal float path");
    v = json_parse_cstr(a, "-65.613616999999977", &err);
    CHECK(v != NULL && json_get_float(v, &f) == JSON_OK, "fast float parse");
    CHECK(f < -65.61 && f > -65.62, "fast float value");
    PASS();

    T("string escapes");
    v = json_parse_cstr(a, "\"hello\\nworld\\t!\"", &err);
    CHECK(v != NULL && json_get_string(v, &s, &sl) == JSON_OK, "string parse");
    CHECK(sl == 13U && memcmp(s, "hello\nworld\t!", 13U) == 0, "escape content");
    PASS();

    T("unicode BMP byte lengths");
    v = json_parse_cstr(a, "\"\\u00e9\\u4e2d\"", &err);
    CHECK(v != NULL && json_get_string(v, &s, &sl) == JSON_OK, "unicode parse");
    CHECK(sl == 5U, "uXXXX length");
    PASS();

    T("unicode surrogate pair U+1D11E");
    v = json_parse_cstr(a, "\"\\uD834\\uDD1E\"", &err);
    CHECK(v != NULL && json_get_string(v, &s, &sl) == JSON_OK, "surrogate parse");
    CHECK(sl == 4U && memcmp(s, "\xF0\x9D\x84\x9E", 4U) == 0, "surrogate bytes");
    PASS();

    T("embedded nul string");
    v = json_parse_cstr(a, "\"a\\u0000b\"", &err);
    CHECK(v != NULL && json_get_string(v, &s, &sl) == JSON_OK, "nul parse");
    CHECK(sl == 3U && s[0] == 'a' && s[1] == '\0' && s[2] == 'b', "nul content");
    PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_objects_arrays(void) {
    JsonArena* a = json_arena_create(NULL, 32768);
    JsonError err = JSON_OK;
    JsonValue *v = NULL, *item = NULL;
    int64_t iv = 0;
    uint32_t len = 0;
    const char* key = NULL;
    uint32_t klen = 0;

    CHECK(a != NULL, "arena");

    T("simple object lookup after finalize");
    v = json_parse_cstr(a, "{\"a\":1,\"b\":2}", &err);
    CHECK(v != NULL && err == JSON_OK, "parse failed");
    CHECK_OK(json_obj_finalize(a, v));
    CHECK_OK(json_obj_get(v, "a", &item));
    CHECK_OK(json_get_int(item, &iv));
    CHECK(iv == 1, "a!=1");
    CHECK_OK(json_obj_get(v, "b", &item));
    CHECK_OK(json_get_int(item, &iv));
    CHECK(iv == 2, "b!=2");
    PASS();

    T("empty object and array lengths");
    v = json_parse_cstr(a, "{\"o\":{},\"a\":[]}", &err);
    CHECK(v != NULL && err == JSON_OK, "parse");
    CHECK_OK(json_obj_finalize(a, v));
    CHECK_OK(json_obj_get(v, "o", &item));
    CHECK_OK(json_get_obj_len(item, &len));
    CHECK(len == 0U, "object len");
    CHECK_OK(json_obj_get(v, "a", &item));
    CHECK_OK(json_get_arr_len(item, &len));
    CHECK(len == 0U, "array len");
    PASS();

    T("nested object/array access");
    v = json_parse_cstr(a, "{\"users\":[{\"name\":\"Alice\",\"age\":30}]}", &err);
    CHECK(v != NULL && err == JSON_OK, "nested parse");
    CHECK_OK(json_obj_finalize(a, v));
    CHECK_OK(json_obj_get(v, "users", &item));
    CHECK_OK(json_arr_get(item, 0U, &item));
    CHECK_OK(json_obj_finalize(a, item));
    CHECK_OK(json_obj_get(item, "age", &item));
    CHECK_OK(json_get_int(item, &iv));
    CHECK(iv == 30, "age");
    PASS();

    T("object iteration preserves insertion order");
    v = json_parse_cstr(a, "{\"first\":1,\"second\":2}", &err);
    CHECK(v != NULL && err == JSON_OK, "iter parse");
    CHECK_OK(json_obj_iter(v, 0U, &key, &klen, &item));
    CHECK(klen == 5U && memcmp(key, "first", 5U) == 0, "first key");
    CHECK_OK(json_obj_iter(v, 1U, &key, &klen, &item));
    CHECK(klen == 6U && memcmp(key, "second", 6U) == 0, "second key");
    PASS();

    T("array bounds and object not found");
    v = json_parse_cstr(a, "[1,2,3]", &err);
    CHECK(v != NULL && err == JSON_OK, "array parse");
    CHECK(json_arr_get(v, 3U, &item) == JSON_ERR_NOT_FOUND, "array bounds");
    v = json_parse_cstr(a, "{\"x\":1}", &err);
    CHECK(v != NULL && err == JSON_OK, "object parse");
    CHECK(json_obj_get(v, "missing", &item) == JSON_ERR_NOT_FOUND, "missing key");
    PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_path(void) {
    JsonArena* a = json_arena_create(NULL, 8192);
    JsonError err = JSON_OK;
    JsonValue *root = NULL, *v = NULL;
    int64_t iv = 0;
    const char* s = NULL;
    uint32_t sl = 0;

    CHECK(a != NULL, "arena");
    root = json_parse_cstr(a, "{\"config\":{\"host\":\"localhost\",\"port\":8080},\"data\":[10,20,30]}", &err);
    CHECK(root != NULL && err == JSON_OK, "setup parse");
    CHECK_OK(json_obj_finalize(a, root));

    T("path data[2]");
    v = json_path(root, "data[2]", &err);
    CHECK(v != NULL && json_get_int(v, &iv) == JSON_OK && iv == 30, "data[2]");
    PASS();

    T("path config.port");
    v = json_path(root, "config.port", &err);
    CHECK(v != NULL && json_get_int(v, &iv) == JSON_OK && iv == 8080, "port");
    PASS();

    T("path config[\"host\"]");
    v = json_path(root, "config[\"host\"]", &err);
    CHECK(v != NULL && json_get_string(v, &s, &sl) == JSON_OK, "host get");
    CHECK(sl == 9U && memcmp(s, "localhost", 9U) == 0, "host value");
    PASS();

    T("path missing reports error");
    v = json_path(root, "data[99]", &err);
    CHECK(v == NULL && err == JSON_ERR_NOT_FOUND, "missing path");
    PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_serialize(void) {
    JsonArena* a = json_arena_create(NULL, 16384);
    JsonError err = JSON_OK;
    JsonValue* root = NULL;
    char buf[1024];
    size_t written = 0U;
    size_t measured = 0U;
    JsonWriteOpts opts;

    CHECK(a != NULL, "arena");

    T("round-trip compact write");
    root = json_parse_cstr(a, "{\"x\":42,\"y\":[1,2,3],\"z\":true}", &err);
    CHECK(root != NULL && err == JSON_OK, "rt parse");
    CHECK_OK(json_write(root, buf, sizeof(buf), &written, NULL));
    CHECK(written > 0U, "written");
    {
        JsonArena* a2 = json_arena_create(NULL, 4096);
        JsonValue* r2 = json_parse(a2, buf, written, &err);
        CHECK(r2 != NULL && err == JSON_OK, "re-parse");
        json_arena_destroy(a2);
    }
    PASS();

    T("buffer too small returns correct error");
    root = json_parse_cstr(a, "{\"key\":\"value\"}", &err);
    CHECK(root != NULL, "parse for small buffer");
    CHECK(json_write(root, buf, 5U, NULL, NULL) == JSON_ERR_BUFFER_TOO_SMALL, "wrong error code");
    PASS();

    T("measure equals write length");
    CHECK_OK(json_measure(root, &measured, NULL));
    CHECK_OK(json_write(root, buf, measured + 1U, &written, NULL));
    CHECK(written == measured, "measure mismatch");
    PASS();

    T("pretty print writes newlines");
    memset(&opts, 0, sizeof(opts));
    opts.pretty = true;
    opts.indent = 2U;
    CHECK_OK(json_write(root, buf, sizeof(buf), &written, &opts));
    CHECK(memchr(buf, '\n', written) != NULL, "no newline");
    PASS();

    T("sorted key output");
    root = json_parse_cstr(a, "{\"b\":2,\"a\":1}", &err);
    CHECK(root != NULL && err == JSON_OK, "sort parse");
    CHECK_OK(json_obj_finalize(a, root));
    memset(&opts, 0, sizeof(opts));
    opts.sort_keys = true;
    CHECK_OK(json_write(root, buf, sizeof(buf), &written, &opts));
    CHECK(strcmp(buf, "{\"a\":1,\"b\":2}") == 0, "sort output");
    PASS();

    T("arena allocated write");
    {
        char* out = NULL;
        size_t out_len = 0U;
        CHECK_OK(json_write_arena(root, a, &out, &out_len, NULL));
        CHECK(out != NULL && out_len == strlen(out), "write arena");
    }
    PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_construction(void) {
    JsonArena* a = json_arena_create(NULL, 16384);
    JsonValue* arr = NULL;
    JsonValue* obj = NULL;
    JsonValue* v = NULL;
    uint32_t len = 0U;
    int64_t iv = 0;
    int i;

    CHECK(a != NULL, "arena");
    arr = json_make_array(a);
    obj = json_make_object(a);
    CHECK(arr != NULL && obj != NULL, "construct roots");

    T("build 100-element array");
    for (i = 0; i < 100; i++) {
        CHECK_OK(json_arr_push(arr, a, json_make_int(a, i)));
    }
    CHECK_OK(json_get_arr_len(arr, &len));
    CHECK(len == 100U, "arr len");
    PASS();

    T("object set, update and finalize");
    CHECK_OK(json_obj_setz(obj, a, "x", json_make_int(a, 1)));
    CHECK_OK(json_obj_setz(obj, a, "x", json_make_int(a, 2)));
    CHECK_OK(json_obj_finalize(a, obj));
    CHECK_OK(json_obj_get(obj, "x", &v));
    CHECK_OK(json_get_int(v, &iv));
    CHECK(iv == 2, "updated value");
    PASS();

    T("deep clone equals original");
    {
        JsonArena* a2 = json_arena_create(NULL, 8192);
        JsonValue* cl = json_clone(a2, obj);
        CHECK(cl != NULL && json_equal(obj, cl), "clone");
        json_arena_destroy(a2);
    }
    PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_errors(void) {
    JsonArena* a = json_arena_create(NULL, 4096);
    JsonError err = JSON_OK;
    JsonValue* v = NULL;
    bool b = false;
    char deep[256];
    size_t pos = 0U;
    int i;

    CHECK(a != NULL, "arena");

    T("reject malformed JSON cases");
    CHECK(json_parse_cstr(a, "{bad}", &err) == NULL, "accepted bad object");
    CHECK(json_parse_cstr(a, "[01]", &err) == NULL, "leading zero");
    CHECK(json_parse_cstr(a, "\"\\q\"", &err) == NULL, "bad escape");
    CHECK(json_parse_cstr(a, "42 garbage", &err) == NULL, "trailing");
    CHECK(json_parse_cstr(a, "\"\\uD800\"", &err) == NULL, "lone surrogate");
    PASS();

    T("reject int64 overflow");
    CHECK(json_parse_cstr(a, "9223372036854775808", &err) == NULL, "accepted int64 max+1");
    CHECK(err == JSON_ERR_OVERFLOW, "wrong positive overflow error");
    CHECK(json_parse_cstr(a, "-9223372036854775809", &err) == NULL, "accepted int64 min-1");
    CHECK(err == JSON_ERR_OVERFLOW, "wrong negative overflow error");
    PASS();

    T("reject excessive depth");
    for (i = 0; i < 80; i++) { deep[pos++] = '['; }
    deep[pos++] = '0';
    for (i = 0; i < 80; i++) { deep[pos++] = ']'; }
    deep[pos] = '\0';
    CHECK(json_parse_cstr(a, deep, &err) == NULL, "accepted too deep");
    CHECK(err == JSON_ERR_DEPTH_EXCEEDED, "wrong depth error");
    PASS();

    T("type mismatch");
    v = json_parse_cstr(a, "42", &err);
    CHECK(v != NULL && json_get_bool(v, &b) == JSON_ERR_TYPE_MISMATCH, "mismatch");
    PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_introsort(void) {
    JsonArena* a = json_arena_create(NULL, 512U * 1024U);
    JsonValue* obj = NULL;
    char kb[32];
    int n = 200;
    int i;
    JsonValue* v = NULL;
    int64_t iv = 0;

    CHECK(a != NULL, "arena");
    obj = json_make_object(a);
    CHECK(obj != NULL, "object");

    T("introsort 200 keys + binary search");
    for (i = n - 1; i >= 0; i--) {
        snprintf(kb, sizeof(kb), "key_%04d", i);
        CHECK_OK(json_obj_setz(obj, a, kb, json_make_int(a, (int64_t)i)));
    }
    CHECK_OK(json_obj_finalize(a, obj));
    snprintf(kb, sizeof(kb), "key_%04d", 99);
    CHECK_OK(json_obj_get(obj, kb, &v));
    CHECK_OK(json_get_int(v, &iv));
    CHECK(iv == 99, "value 99");
    snprintf(kb, sizeof(kb), "key_%04d", 0);
    CHECK_OK(json_obj_get(obj, kb, &v));
    snprintf(kb, sizeof(kb), "key_%04d", n - 1);
    CHECK_OK(json_obj_get(obj, kb, &v));
    PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_arena_reset(void) {
    JsonArena* a = json_arena_create(NULL, 4096);
    JsonError err = JSON_OK;
    int i;

    CHECK(a != NULL, "arena");

    T("arena reset x5 reuse");
    for (i = 0; i < 5; i++) {
        JsonValue* v;
        json_arena_reset(a);
        v = json_parse_cstr(a, "[1,2,3,{\"ok\":true}]", &err);
        CHECK(v != NULL && err == JSON_OK, "reset round");
    }
    PASS();

    json_arena_destroy(a);
    return 0;
}

int main(void) {
    int f = 0;
    printf("=== sJson v1.1 regression suite ===\n\n");
    f += test_primitives();
    f += test_objects_arrays();
    f += test_path();
    f += test_serialize();
    f += test_construction();
    f += test_errors();
    f += test_introsort();
    f += test_arena_reset();
    printf("\n%s — %d failure(s)\n", f ? "TESTS FAILED" : "ALL TESTS PASSED", f);
    return f ? 1 : 0;
}
