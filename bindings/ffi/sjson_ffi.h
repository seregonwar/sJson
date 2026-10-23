/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#ifndef SJSON_FFI_H
#define SJSON_FFI_H
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32) && defined(SJSON_FFI_SHARED)
# ifdef SJSON_FFI_BUILD
#  define SJSON_API __declspec(dllexport)
# else
#  define SJSON_API __declspec(dllimport)
# endif
#else
# define SJSON_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SJsonDocument SJsonDocument;
typedef struct JsonValue SJsonValue;
SJSON_API unsigned sjson_abi_version(void);
SJSON_API const char* sjson_version(void);

/* Documents own their values. Freeing a document invalidates all borrowed
 * values/string pointers. Lengths are byte lengths and may include NUL. */
SJSON_API SJsonDocument* sjson_document_parse(const char*, size_t, int* error);
SJSON_API void sjson_document_free(SJsonDocument*);
SJSON_API const SJsonValue* sjson_document_root(const SJsonDocument*);
SJSON_API const SJsonValue* sjson_value_member(const SJsonValue*, const char*, size_t);
SJSON_API const SJsonValue* sjson_value_at(const SJsonValue*, uint32_t);
/* Kinds: null=0, bool=1, int64=2, double=3, string=4, array=5, object=6. */
SJSON_API int sjson_value_kind(const SJsonValue*);
SJSON_API size_t sjson_value_size(const SJsonValue*);
SJSON_API int sjson_value_bool(const SJsonValue*, int*);
SJSON_API int sjson_value_int(const SJsonValue*, int64_t*);
SJSON_API int sjson_value_number(const SJsonValue*, double*);
SJSON_API int sjson_value_string(const SJsonValue*, const char**, size_t*);
SJSON_API int sjson_value_entry(const SJsonValue*, uint32_t, const char**, size_t*, const SJsonValue**);
/* With buffer=NULL and capacity=0, return the required byte count (no NUL).
 * Actual output needs length+1 bytes. Status codes match JsonError. */
SJSON_API int sjson_value_write(const SJsonValue*, char*, size_t, size_t*);
SJSON_API const char* sjson_error_message(int);
#ifdef __cplusplus
}
#endif
#endif
