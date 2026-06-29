# Changelog

All notable changes to LionArray are documented here.
This project adheres to [Semantic Versioning](https://semver.org/).

## [1.0.2] - 2026-06-29

### Fixed
- Silence `-Wclass-memaccess` in `Default()`: the zeroing `memset` now casts the
  pointer to `void*` (same as `Realloc()`), avoiding the warning for
  trivially-copyable types that still have a non-trivial default constructor
  (e.g. `DirEntry`). No behavioural change.

## [1.0.1] - 2026-06-29

### Fixed
- `Array<T>` now compiles for C-array element types (e.g. `uint8_t[8]`): the
  internal `safe_default` slot is zero-initialised with `memset` instead of a
  `T()` assignment, which is invalid for array types. Zeroing stays within the
  trivially-copyable contract and matches the relocation done elsewhere.

## [1.0.0] - 2026-06-28

First public release.

### Added
- `Array<T>`, `Queue<T>`, `Stack<T>` containers (header-only).
- `Queue`/`Stack` `TryPop` / `TryPeek` returning `false` on empty.
- Compile-time `static_assert` enforcing trivially-copyable `T` (where
  `<type_traits>` is available).
- `explicit` constructor (no implicit `int -> Array` conversion).
- Deleted copy-ctor / copy-assignment (`Array` is non-copyable).
- Library packaging: `library.properties`, `library.json`, `keywords.txt`,
  example sketch, 0BSD license.

### Changed
- `Sort` now takes a templated comparator (lambda / function pointer / functor)
  instead of `nonstd::function`, removing the external dependency.

### Fixed
- `Reserve` / sparse `operator[]` no longer expose uninitialized or stale slots
  (the newly exposed range is zeroed).
- `Last()` on an empty array no longer dereferences `_data[-1]`.
- Negative-index guards in `Remove`, `Remove(index, len)`, `Insert` and
  `operator[]`.
