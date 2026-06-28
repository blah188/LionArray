# Changelog

All notable changes to LionArray are documented here.
This project adheres to [Semantic Versioning](https://semver.org/).

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
