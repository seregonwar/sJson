#define _POSIX_C_SOURCE 200809L
#define JSON_IMPLEMENTATION
#include "../src/json_pal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(SJSON_BENCH_WITH_CJSON)
#include <cjson/cJSON.h>
#endif

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

typedef int (*BenchFn)(const char* src, size_t len, int iterations, double* seconds_out);

static char* xmalloc(size_t n) {
    char* p = (char*)malloc(n);
    if (p == NULL) {
        fprintf(stderr, "allocation failed (%llu bytes)\n", (unsigned long long)n);
        exit(2);
    }
    return p;
}

static char* make_number_array(size_t count, size_t* len_out) {
    size_t cap = count * 12U + 2U;
    char* s = xmalloc(cap);
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
    char* s = xmalloc(cap);
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
    char* s = xmalloc(cap);
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

static int bench_sjson(const char* src, size_t len, int iterations, double* seconds_out) {
    JsonArena* arena = json_arena_create(NULL, len * 3U + 65536U);
    JsonError err = JSON_OK;
    volatile unsigned long long sink = 0U;
    double t0, t1;
    int i;
    if (arena == NULL) { return 1; }
    for (i = 0; i < 8; i++) {
        JsonValue* root;
        json_arena_reset(arena);
        root = json_parse(arena, src, len, &err);
        if (root == NULL) { json_arena_destroy(arena); return 1; }
        sink += (unsigned long long)root->type;
    }
    t0 = bench_now();
    for (i = 0; i < iterations; i++) {
        JsonValue* root;
        json_arena_reset(arena);
        root = json_parse(arena, src, len, &err);
        if (root == NULL) { json_arena_destroy(arena); return 1; }
        sink += (unsigned long long)root->type;
    }
    t1 = bench_now();
    if (sink == 0xFFFFFFFFULL) { printf("%llu\n", sink); }
    json_arena_destroy(arena);
    *seconds_out = t1 - t0;
    return 0;
}

#if defined(SJSON_BENCH_WITH_CJSON)
static int bench_cjson(const char* src, size_t len, int iterations, double* seconds_out) {
    volatile int sink = 0;
    double t0, t1;
    int i;
    (void)len;
    for (i = 0; i < 4; i++) {
        cJSON* root = cJSON_ParseWithLength(src, len);
        if (root == NULL) { return 1; }
        sink += cJSON_GetArraySize(root);
        cJSON_Delete(root);
    }
    t0 = bench_now();
    for (i = 0; i < iterations; i++) {
        cJSON* root = cJSON_ParseWithLength(src, len);
        if (root == NULL) { return 1; }
        sink += cJSON_IsArray(root) || cJSON_IsObject(root);
        cJSON_Delete(root);
    }
    t1 = bench_now();
    if (sink == -1) { printf("%d\n", sink); }
    *seconds_out = t1 - t0;
    return 0;
}
#endif

static void report(const char* lib, const char* dataset, size_t len, int iterations, double seconds, double sjson_seconds) {
    double mbps = ((double)len * (double)iterations) / (1024.0 * 1024.0) / seconds;
    double ns_byte = seconds * 1e9 / ((double)len * (double)iterations);
    double speedup = sjson_seconds > 0.0 ? sjson_seconds / seconds : 1.0;
    printf("%-8s %-10s %8llu B %6d it %9.2f MB/s %7.3f ns/B %7.2fx vs sJson\n",
           lib, dataset, (unsigned long long)len, iterations, mbps, ns_byte, speedup);
}

static int run_dataset(const char* name, const char* src, size_t len, int iterations) {
    double sjson_sec = 0.0;
    double sec = 0.0;
    int failed = 0;

    if (bench_sjson(src, len, iterations, &sjson_sec) != 0) {
        fprintf(stderr, "sJson failed on %s\n", name);
        return 1;
    }
    report("sJson", name, len, iterations, sjson_sec, sjson_sec);

#if defined(SJSON_BENCH_WITH_CJSON)
    if (bench_cjson(src, len, iterations, &sec) == 0) { report("cJSON", name, len, iterations, sec, sjson_sec); }
    else { fprintf(stderr, "cJSON failed on %s\n", name); failed = 1; }
#endif

    return failed;
}

int main(int argc, char** argv) {
    int iterations = 300;
    size_t n_len, o_len, s_len;
    char* numbers;
    char* object;
    char* strings;
    int failed = 0;

    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations <= 0) { iterations = 300; }
    }

    numbers = make_number_array(10000U, &n_len);
    object = make_object(2000U, &o_len);
    strings = make_string_array(4000U, &s_len);

    printf("sJson comparative benchmark (iterations=%d)\n", iterations);
    printf("Ratio column: below 1.0x means that row is slower than sJson; above 1.0x means faster.\n");
    failed |= run_dataset("numbers", numbers, n_len, iterations);
    failed |= run_dataset("object", object, o_len, iterations);
    failed |= run_dataset("strings", strings, s_len, iterations);

    free(numbers);
    free(object);
    free(strings);
    return failed ? 1 : 0;
}
