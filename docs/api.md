# sJson API Notes

## Stability

The C functions documented in `README.md` and declared in `src/sJson.c` are the public source API for the current single-header distribution. Struct layout is currently visible for speed and direct integration, but consumers that need long-term ABI stability should prefer an opaque wrapper once it is introduced.

## Ownership

- `JsonArena*` owns every parsed or constructed `JsonValue`.
- Individual values are never freed directly.
- `json_arena_destroy` invalidates every value, string and object/array pointer allocated in the arena.
- `json_arena_reset` invalidates previous values but reuses memory blocks.

## Threading

There is no global mutable parser state. Use one arena per thread/document. Do not mutate or reset the same arena concurrently.

## Numeric behavior

- Integers are parsed into `int64_t` with explicit overflow checks.
- Floating-point numbers use `JSON_FAST_FLOAT` for common decimals and fall back to `strtod` for hard cases.
- Define `JSON_FAST_FLOAT 0` before including the header to force the libc path for floats.

## Errors

APIs return `JsonError`. Use `json_error_str` for human-readable diagnostics.
