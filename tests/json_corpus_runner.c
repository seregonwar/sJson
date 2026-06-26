#define JSON_IMPLEMENTATION
#include "../src/json_pal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

typedef struct {
    unsigned accepted_valid;
    unsigned rejected_valid;
    unsigned rejected_invalid;
    unsigned accepted_invalid;
    unsigned accepted_impl;
    unsigned rejected_impl;
    unsigned skipped;
} CorpusStats;

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

static int run_one(const char* path, CorpusStats* st) {
    const char* name = base_name(path);
    char prefix = name[0];
    size_t len = 0U;
    char* data = read_file(path, &len);
    JsonArena* arena;
    JsonError err = JSON_OK;
    JsonValue* root;
    int accepted;

    if (data == NULL) {
        fprintf(stderr, "SKIP unreadable %s\n", path);
        st->skipped++;
        return 0;
    }

    arena = json_arena_create(NULL, len * 3U + 65536U);
    if (arena == NULL) {
        free(data);
        fprintf(stderr, "OOM creating arena for %s\n", path);
        return 1;
    }

    root = json_parse(arena, data, len, &err);
    accepted = (root != NULL && err == JSON_OK);

    if (prefix == 'y') {
        if (accepted) { st->accepted_valid++; }
        else {
            st->rejected_valid++;
            fprintf(stderr, "FAIL valid rejected: %s (%s)\n", path, json_error_str(err));
        }
    } else if (prefix == 'n') {
        if (!accepted) { st->rejected_invalid++; }
        else {
            st->accepted_invalid++;
            fprintf(stderr, "FAIL invalid accepted: %s\n", path);
        }
    } else if (prefix == 'i') {
        if (accepted) { st->accepted_impl++; }
        else { st->rejected_impl++; }
    } else {
        st->skipped++;
    }

    json_arena_destroy(arena);
    free(data);
    return 0;
}

static int is_regular_file(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) { return 0; }
    return (st.st_mode & S_IFREG) != 0;
}

static int run_dir(const char* dir, CorpusStats* st) {
    DIR* d = opendir(dir);
    struct dirent* ent;
    if (d == NULL) {
        fprintf(stderr, "cannot open corpus directory: %s\n", dir);
        return 1;
    }
    while ((ent = readdir(d)) != NULL) {
        char path[4096];
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }
        snprintf(path, sizeof(path), "%s%s%s", dir, PATH_SEP, ent->d_name);
        if (is_regular_file(path)) {
            if (run_one(path, st) != 0) { closedir(d); return 1; }
        }
    }
    closedir(d);
    return 0;
}

int main(int argc, char** argv) {
    const char* dir = argc > 1 ? argv[1] : "tests/JSONTestSuite/test_parsing";
    CorpusStats st;
    memset(&st, 0, sizeof(st));

    if (run_dir(dir, &st) != 0) { return 2; }

    printf("JSONTestSuite corpus: %s\n", dir);
    printf("  valid accepted:      %u\n", st.accepted_valid);
    printf("  valid rejected:      %u\n", st.rejected_valid);
    printf("  invalid rejected:    %u\n", st.rejected_invalid);
    printf("  invalid accepted:    %u\n", st.accepted_invalid);
    printf("  impl accepted:       %u\n", st.accepted_impl);
    printf("  impl rejected:       %u\n", st.rejected_impl);
    printf("  skipped:             %u\n", st.skipped);

    return (st.rejected_valid == 0U && st.accepted_invalid == 0U) ? 0 : 1;
}
