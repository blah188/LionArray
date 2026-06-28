# LionArray

Lean dynamic **`Array<T>`**, **`Queue<T>`** and **`Stack<T>`** for embedded C++
(ESP8266 / ESP32 / Arduino). Header-only, no dependencies, allocation-frugal.

This is **not** a drop-in `std::vector`. It is deliberately minimal and trades
generality for a small footprint and predictable behavior on microcontrollers.
Read the contract below before using it.

## Features

- Single header (`src/Array.h`), no dependencies.
- `Array<T>`: append (`+=`), random access, `Insert`, `Remove`, `Reserve`,
  `Find`, `Sort` (custom comparator, no allocation).
- `Queue<T>` (FIFO) and `Stack<T>` (LIFO) built on top, with unambiguous
  `TryPop` / `TryPeek`.
- Geometric growth (doubling) from a configurable initial capacity.

## Contract — read this

**1. `T` must be trivially copyable.**
Primitives, PODs, structs built only from such fields (fixed `char[]` buffers are
fine), and raw pointers. The container relocates elements with
`memcpy`/`memmove` on growth, so types with a non-trivial copy-ctor/destructor
(e.g. `String`, `std::function`) are **not** supported and would corrupt the heap.
This is enforced at compile time via `static_assert` wherever `<type_traits>` is
available (all modern 32-bit cores; classic AVR may skip the check).

**2. Pointer elements are stored non-owning.**
`Array<Foo*>` never deletes the pointees — the destructor frees only its own
buffer. Manage pointee lifetime yourself.

**3. An `Array` object is non-copyable (no value semantics).**
Copy constructor and copy-assignment are `= delete` (move is suppressed too).
A shallow copy would double-free the buffer. Pass/return by reference
(`Array<T>&`) or pointer — never by value. (`Array<T> a = Array<T>(N);` still
compiles via C++17 guaranteed copy-elision.)

**4. `operator[]` behavior:**
- non-const, **non-negative** index past the end **grows** the array (sparse
  fill, by design);
- a **negative** index, or any out-of-range **read** on a `const` array, returns
  a safe default (a fresh `T{}`) instead of touching memory.

## Install

**PlatformIO** — `platformio.ini`:

```ini
lib_deps = leva/LionArray
```

**Arduino IDE** — Library Manager → search "LionArray" (once published), or
clone into your `libraries/` folder.

## Usage

```cpp
#include <Array.h>

Array<int> a;
a += 10;                 // append
a += 20;
a.Insert(15, 1);         // 10, 15, 20
a.Remove(0);             // 15, 20

for (int i = 0; i < a.Length(); i++)
    Serial.println(a[i]);

a.Sort([](const int &x, const int &y) { return x - y; });
int idx = a.Find(15);    // -1 if not found

Queue<int> q;            // FIFO
q.Push(1); q.Push(2);
int v;
while (q.TryPop(v)) { /* ... */ }

Stack<int> s;            // LIFO
s.Push(1); s.Push(2);
while (s.TryPop(v)) { /* ... */ }
```

See [`examples/BasicUsage`](examples/BasicUsage/BasicUsage.ino).

## API (summary)

`Array<T>`
- `explicit Array(int capacity = 8)`
- `int Length()` / `int BufferLength()`
- `T& operator[](int)` (read/write, grows) and `const` read variant
- `void operator+=(T)` — append
- `T& Last()`
- `void Insert(T value, int index)`
- `void Remove(int index)` / `void Remove(int index, int len)`
- `void Reserve(int len)` / `void Clear(int capacity = 1)`
- `int Find(const T&)`
- `template<class Cmp> void Sort(Cmp)` — `cmp(a,b)` returns `<0 / 0 / >0`
- `T* GetData()`

`Queue<T>` / `Stack<T>`
- `void Push(T)`, `T Pop()`, `T Peek()`
- `bool TryPop(T& out)`, `bool TryPeek(T& out)` — `false` when empty
- `int Count()`, `bool Any()`

## License

[0BSD](LICENSE) (Zero-Clause BSD) — public-domain-equivalent, no attribution
required.
