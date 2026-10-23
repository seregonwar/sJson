# sJson API Notes

## Stability

The C functions declared in `src/sJson.c` form the source API. Native struct
layouts remain visible; foreign-language code should use the opaque ABI in
`bindings/ffi/sjson_ffi.h` rather than duplicate those layouts. Its ABI version is
reported separately from the package version. See [bindings](bindings.md).

## Ownership

- `JsonArena*` owns every parsed or constructed `JsonValue`.
- Individual values are never freed directly.
- `json_arena_destroy` invalidates every value, string and object/array pointer allocated in the arena.
- `json_arena_reset` invalidates previous values but reuses memory blocks.
- `json_arena_trim` invalidates previous values and frees every extra block.
- Parsing appends allocations, including partial allocations on failure. Reset
  before replacing a document, or destroy the arena when finished.

## Threading

There is no global mutable parser state. Use one arena per thread/document. Do not mutate or reset the same arena concurrently.

## Numeric behavior

- Integers are parsed into `int64_t` with explicit overflow checks.
- Floating-point numbers use `JSON_FAST_FLOAT` for common decimals and fall back to `strtod` for hard cases.
- Define `JSON_FAST_FLOAT 0` before including the header to force the libc path for floats.
- `json_get_int` rejects non-finite or out-of-range doubles without changing the output.

## Writing

`json_measure` counts output bytes, including an optional trailing newline and
excluding the terminating NUL. A real write therefore needs `measured + 1` bytes.
Zero-capacity writes never touch the output pointer. `ascii_only` writes Unicode
code points, using surrogate pairs for supplementary characters.

`sort_keys` writes lexical key order without mutating the input or changing its
hash-based lookup index. Sorted writes use a temporary arena released before
returning; measurement itself needs no sorting allocation. Serialization,
cloning and equality traversal are depth-limited for constructed values too.

## Allocation

Custom allocators must provide matching `alloc` and `free` callbacks. The
`realloc` callback is optional. Backing allocations must support the configured
`JSON_ARENA_ALIGN`, which must be a nonzero power of two.

## Errors

APIs return `JsonError`. Use `json_error_str` for human-readable diagnostics.
