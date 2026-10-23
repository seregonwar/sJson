# Security Policy

Report security issues privately to the project maintainer before opening public issues.

Parser hardening currently includes:

- configurable depth, node and collection limits;
- integer overflow checks;
- malformed number rejection;
- UTF-8/string escape validation;
- JSONTestSuite validation;
- sanitizer-friendly build targets.

Recommended checks before release:

```sh
make check
make asan
make ubsan
```

See [validation](docs/testing.md) for the full C/C++/FFI suite, counted allocation
tests and sanitizer configuration. C values and string views expire on arena
reset, trim or destruction. Opaque-ABI pointers expire when their document is
freed; Rust and Python wrappers enforce their documented ownership boundaries.
