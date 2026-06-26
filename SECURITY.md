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
