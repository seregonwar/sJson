#define _POSIX_C_SOURCE 200809L
#define JSON_IMPLEMENTATION
#include "../src/json_pal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
static double bench_now(void) {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
}
#else
#  include <time.h>
static double bench_now(void) {
    struct timespec ts;
#  if defined(CLOCK_MONOTONIC_RAW)
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#  else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#  endif
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
#endif

static char* bench_xmalloc(size_t n) {
    char* p = (char*)malloc(n);
    if (p == NULL) {
        fprintf(stderr, "allocation failed (%llu bytes)\n", (unsigned long long)n);
        exit(2);
    }
    return p;
}

static char* make_number_array(size_t count, size_t* len_out) {
    size_t cap = count * 12U + 2U;
    char* s = bench_xmalloc(cap);
    size_t pos = 0U;
    size_t i;
    s[pos++] = '[';
    for (i = 0U; i < count; i++) {
        int n = snprintf(s + pos, cap - pos, "%s%u", i ? "," : "", (unsigned)i);
        pos += (size_t)n;
    }
    s[pos++] = ']';
    s[pos] = '\0';
    *len_out = pos;
    return s;
}

static char* make_object(size_t count, size_t* len_out) {
    size_t cap = count * 32U + 2U;
    char* s = bench_xmalloc(cap);
    size_t pos = 0U;
    size_t i;
    s[pos++] = '{';
    for (i = 0U; i < count; i++) {
        int n = snprintf(s + pos, cap - pos, "%s\"k%05u\":%u", i ? "," : "", (unsigned)i, (unsigned)i);
        pos += (size_t)n;
    }
    s[pos++] = '}';
    s[pos] = '\0';
    *len_out = pos;
    return s;
}

static char* make_string_array(size_t count, size_t* len_out) {
    static const char item[] = "\"alpha beta gamma delta epsilon\"";
    size_t item_len = strlen(item);
    size_t cap = 2U + count * (item_len + 1U);
    char* s = bench_xmalloc(cap);
    size_t pos = 0U;
    size_t i;
    s[pos++] = '[';
    for (i = 0U; i < count; i++) {
        if (i != 0U) { s[pos++] = ','; }
        memcpy(s + pos, item, item_len);
        pos += item_len;
    }
    s[pos++] = ']';
    s[pos] = '\0';
    *len_out = pos;
    return s;
}

static int bench_parse_case(const char* name, const char* src, size_t len, int iterations) {
    JsonArena* arena = json_arena_create(NULL, len * 3U + 65536U);
    JsonError err = JSON_OK;
    volatile unsigned long long sink = 0U;
    double t0, t1, sec, mbps, ns_byte;
    int i;

    if (arena == NULL) { return 1; }
    for (i = 0; i < 8; i++) {
        JsonValue* root;
        json_arena_reset(arena);
        root = json_parse(arena, src, len, &err);
        if (root == NULL) {
            fprintf(stderr, "%s warmup failed: %s\n", name, json_error_str(err));
            json_arena_destroy(arena);
            return 1;
        }
        sink += (unsigned long long)root->type;
    }

    t0 = bench_now();
    for (i = 0; i < iterations; i++) {
        JsonValue* root;
        json_arena_reset(arena);
        root = json_parse(arena, src, len, &err);
        if (root == NULL) {
            fprintf(stderr, "%s failed: %s\n", name, json_error_str(err));
            json_arena_destroy(arena);
            return 1;
        }
        sink += (unsigned long long)root->type;
    }
    t1 = bench_now();

    sec = t1 - t0;
    mbps = ((double)len * (double)iterations) / (1024.0 * 1024.0) / sec;
    ns_byte = sec * 1e9 / ((double)len * (double)iterations);
    printf("parse %-18s %8llu B  %6d it  %9.2f MB/s  %7.3f ns/B\n",
           name, (unsigned long long)len, iterations, mbps, ns_byte);

    if (sink == 0xFFFFFFFFULL) { printf("%llu\n", sink); }
    json_arena_destroy(arena);
    return 0;
}

static int bench_lookup_case(size_t count, int iterations) {
    size_t len;
    char* src = make_object(count, &len);
    JsonArena* arena = json_arena_create(NULL, len * 3U + 65536U);
    JsonError err = JSON_OK;
    JsonValue* root;
    volatile unsigned long long sink = 0U;
    double t0, t1, sec, ns_lookup;
    int i;

    if (arena == NULL) { free(src); return 1; }
    root = json_parse(arena, src, len, &err);
    if (root == NULL || json_obj_finalize(arena, root) != JSON_OK) {
        fprintf(stderr, "lookup setup failed: %s\n", json_error_str(err));
        json_arena_destroy(arena);
        free(src);
        return 1;
    }

    t0 = bench_now();
    for (i = 0; i < iterations; i++) {
        size_t k;
        for (k = 0U; k < count; k++) {
            char key[16];
            JsonValue* v = NULL;
            int n = snprintf(key, sizeof(key), "k%05u", (unsigned)k);
            if (json_obj_get_n(root, key, (size_t)n, &v) != JSON_OK) {
                fprintf(stderr, "lookup failed for %s\n", key);
                json_arena_destroy(arena);
                free(src);
                return 1;
            }
            sink += (unsigned long long)v->type;
        }
    }
    t1 = bench_now();

    sec = t1 - t0;
    ns_lookup = sec * 1e9 / ((double)count * (double)iterations);
    printf("lookup finalized object %8llu keys %6d it  %9.2f ns/lookup\n",
           (unsigned long long)count, iterations, ns_lookup);

    if (sink == 0xFFFFFFFFULL) { printf("%llu\n", sink); }
    json_arena_destroy(arena);
    free(src);
    return 0;
}

int main(int argc, char** argv) {
    int iterations = 500;
    size_t len_numbers, len_object, len_strings;
    char* numbers;
    char* object;
    char* strings;
    int failed = 0;

    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations <= 0) { iterations = 500; }
    }

    numbers = make_number_array(10000U, &len_numbers);
    object = make_object(2000U, &len_object);
    strings = make_string_array(4000U, &len_strings);

    printf("sJson benchmark (iterations=%d)\n", iterations);
    failed |= bench_parse_case("numbers", numbers, len_numbers, iterations);
    failed |= bench_parse_case("object", object, len_object, iterations);
    failed |= bench_parse_case("strings", strings, len_strings, iterations);
    failed |= bench_lookup_case(2000U, iterations);

    free(numbers);
    free(object);
    free(strings);
    return failed ? 1 : 0;
}
