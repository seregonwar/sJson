/*
 * sJson v1.1.0 — safe, fast, single-header JSON library in C99.
 * SPDX-License-Identifier: GPL-3.0-only OR MIT
 *
 * This file is dual-licensed under GPL-3.0-only OR MIT.
 * You may choose either license.
 */

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

static char* read_file(const char* path, size_t* len_out) {
    FILE* f = fopen(path, "rb");
    long n;
    char* data;
    if (f == NULL) { return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    n = ftell(f);
    if (n < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    data = (char*)malloc((size_t)n + 1U);
    if (data == NULL) { fclose(f); return NULL; }
    if (fread(data, 1U, (size_t)n, f) != (size_t)n) { free(data); fclose(f); return NULL; }
    fclose(f);
    data[n] = '\0';
    *len_out = (size_t)n;
    return data;
}

static const char* base_name(const char* path) {
    const char* a = strrchr(path, '/');
    const char* b = strrchr(path, '\\');
    const char* p = a > b ? a : b;
    return p ? p + 1 : path;
}

static int bench_sjson(const char* src, size_t len, int iterations, double* seconds_out) {
    JsonArena* arena = json_arena_create(NULL, len * 3U + 65536U);
    JsonError err = JSON_OK;
    volatile unsigned long long sink = 0U;
    double t0, t1;
    int i;
    if (arena == NULL) { return 1; }
    for (i = 0; i < 3; i++) {
        JsonValue* root;
        json_arena_reset(arena);
        root = json_parse(arena, src, len, &err);
        if (root == NULL) { fprintf(stderr, "sJson parse error: %s\n", json_error_str(err)); json_arena_destroy(arena); return 1; }
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
    for (i = 0; i < 2; i++) {
        cJSON* root = cJSON_ParseWithLength(src, len);
        if (root == NULL) { return 1; }
        sink += cJSON_IsArray(root) || cJSON_IsObject(root);
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

static void report(const char* lib, const char* file, size_t len, int iterations, double seconds, double sjson_seconds) {
    double mbps = ((double)len * (double)iterations) / (1024.0 * 1024.0) / seconds;
    double ns_byte = seconds * 1e9 / ((double)len * (double)iterations);
    double ratio = sjson_seconds > 0.0 ? sjson_seconds / seconds : 1.0;
    printf("%-8s %-22s %10llu B %5d it %9.2f MB/s %7.3f ns/B %6.2fx vs sJson\n",
           lib, file, (unsigned long long)len, iterations, mbps, ns_byte, ratio);
}

static int run_asset(const char* path, int iterations) {
    size_t len = 0U;
    char* data = read_file(path, &len);
    double sjson_sec = 0.0;
    double sec = 0.0;
    const char* file = base_name(path);
    int failed = 0;

    if (data == NULL) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }

    if (bench_sjson(data, len, iterations, &sjson_sec) != 0) {
        fprintf(stderr, "sJson failed on %s\n", path);
        free(data);
        return 1;
    }
    report("sJson", file, len, iterations, sjson_sec, sjson_sec);

#if defined(SJSON_BENCH_WITH_CJSON)
    if (bench_cjson(data, len, iterations, &sec) == 0) { report("cJSON", file, len, iterations, sec, sjson_sec); }
    else { fprintf(stderr, "cJSON failed on %s\n", path); failed = 1; }
#endif

    free(data);
    return failed;
}

int main(int argc, char** argv) {
    int iterations = 50;
    int first_file = 1;
    int failed = 0;
    int i;

    if (argc > 2 && strcmp(argv[1], "--iterations") == 0) {
        iterations = atoi(argv[2]);
        if (iterations <= 0) { iterations = 50; }
        first_file = 3;
    }

    if (argc <= first_file) {
        fprintf(stderr, "usage: %s [--iterations N] file.json [file.json ...]\n", argv[0]);
        return 2;
    }

    printf("sJson official-asset benchmark (iterations=%d)\n", iterations);
    printf("Ratio column: below 1.0x means that row is slower than sJson; above 1.0x means faster.\n");
    for (i = first_file; i < argc; i++) {
        failed |= run_asset(argv[i], iterations);
    }
    return failed ? 1 : 0;
}
