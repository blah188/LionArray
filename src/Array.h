#pragma once
#include <string.h>
#if __has_include(<type_traits>)
#include <type_traits>
#define LIONARRAY_HAS_TYPE_TRAITS 1
#endif

// =============================================================================
//  Array<T> / Queue<T> / Stack<T> — deliberately minimal dynamic container
//  for embedded (ESP8266) use. Two contracts MUST be respected:
//
//  1) T MUST be trivially copyable.
//     * OK:  primitives (int, float, uint32_t, ...), PODs, and structs built
//            ONLY from such fields (fixed char[] buffers are fine), and raw
//            pointers. A struct may have user-defined constructors and still
//            be trivially copyable (e.g. SceneAction) — that is fine.
//     * Pointer elements are stored NON-OWNING: ~Array() frees only its own
//            buffer, never the pointed-to objects. Delete those yourself.
//     * NOT OK: String, std::function, or any type with a non-trivial
//            copy-ctor/destructor. Growth, Insert() and Remove() relocate
//            elements with memcpy/memmove/memset (no per-element copy-ctor),
//            which would corrupt such types and double-free their resources.
//
//  2) An Array object itself is NOT copyable — it has NO value semantics.
//     The copy ctor and copy-assignment are = delete (enforced at compile
//     time; move is suppressed too). A compiler-generated copy would
//     shallow-copy _data, and then two destructors would free the same buffer
//     (double-free). Therefore:
//        * do NOT pass an Array by value,
//        * do NOT assign one Array to another (a = b),
//        * do NOT return an Array by value.
//     Pass/return by reference (Array<T>&) or by pointer instead.
//     NOTE: this class was meant to stay simple — it was never designed to be
//     returned/copied. Any return-by-value usage (added later, not original
//     intent) survives ONLY on the compiler's optional NRVO and is unsupported.
// =============================================================================

template <typename T>
class Array
{
#ifdef LIONARRAY_HAS_TYPE_TRAITS
    // Enforce contract #1 at compile time (where <type_traits> is available,
    // i.e. all modern 32-bit cores; classic AVR may lack it). Turns a silent
    // heap corruption into a clear compile error.
    static_assert(std::is_trivially_copyable<T>::value,
        "Array<T> requires a trivially-copyable T (primitives, PODs, structs "
        "built from such fields, or raw pointers). Types with a non-trivial "
        "copy-ctor/destructor such as String or std::function are NOT supported "
        "— growth relocates elements with memcpy/memmove and would corrupt them.");
#endif

protected:
    T *_data;
    int _size, _realSize;

public:
    // explicit: forbid implicit int -> Array conversion (e.g. `Array<int> a = 5;`
    // or passing an int where an Array& is expected). Explicit `Array<T>(N)`
    // construction is unaffected.
    explicit Array(int size = 8)
    {
        _size = size;
        _data = size > 0 ? new T[_size] : nullptr;
        _realSize = 0;
    }

    ~Array()
    {
         delete[] _data;
    }

    // Non-copyable / no value semantics (see contract #2 at top of file).
    // The implicit copy would shallow-copy _data and the two dtors would
    // double-free it. Deleting these turns ANY copy/assignment/by-value
    // pass/return into a compile error (also makes any class holding an Array
    // member non-copyable, surfacing hidden double-frees). Declaring the
    // deleted copy ctor also suppresses the implicit move — intended.
    // NOTE: `Array<T> a = Array<T>(N);` still compiles (C++17 guaranteed
    // copy-elision constructs in place, no copy ctor involved).
    Array(const Array &) = delete;
    Array &operator=(const Array &) = delete;

    void Clear(int size = 1)
    {
        _realSize = 0;
        if (size > _size)
        {
            delete[] _data;
            _size = size;
            _data = new T[_size];
        }
    }

    T* GetData() const {return _data;}

    // Read-only access. Out-of-range (incl. negative) returns a safe default
    // instead of touching the buffer — see Default() below.
    T &operator[](int index) const
    {
        if (index < 0 || index >= _realSize)
            return Default();
        return *(_data + index);
    }

    // Read/write access. A non-negative index past the end grows the array (by
    // design — sparse fill). A negative index is invalid and returns the safe
    // default, never *( _data - n ).
    T &operator[](int index)
    {
        if (index < 0)
            return Default();
        Realloc(index);
        if (index >= _realSize)
            _realSize = index + 1;
        return *(_data + index);
    }

    T& Last () const
    {
        if (_realSize <= 0)
            return Default(); // empty: avoid *(_data - 1) UB
        return *(_data + _realSize - 1);
    }

    void operator+=(T data)
    {
        Realloc(_realSize);
        _data[_realSize++] = data;
    }

    inline int Length () const
    {
        return _realSize;
    }

    inline int BufferLength () const
    {
        return _size;
    }

    void Remove(int index)
    {
        if (index < 0 || index >= _realSize)
            return;
        if (index < _realSize - 1)
            memmove(_data + index, _data + (index+1), (_realSize-index-1) * sizeof(T));
        --_realSize;
    }

    void Remove(int index, int len)
    {
        if (index < 0 || len <= 0 || index+len > _realSize)
            return;
        if (index < _realSize - len)
            memmove(_data + index, _data + (index+len), (_realSize-index-len) * sizeof(T));
        _realSize -= len;
    }

    void Insert(T value, int index)
    {
        if (index < 0)
            return;
        if (index >= _realSize)
        {
            (*this) += value;
            return;
        }
        Realloc(_realSize + 1);
        memmove(_data + (index+1), _data + index, (_realSize-index) * sizeof(T));
        ++_realSize;
        _data[index] = value;
    }

    void Reserve(int len)
    {
        Realloc(len-1);
        if (len > _realSize)
            _realSize = len;
    }

    int Find(const T &item) const
    {
        for (int i = 0; i < _realSize; i++)
            if (_data[i] == item)
                return i;
        return -1;
    }

    // Sort the array (bubble sort) with a custom comparator.
    // comparator(a, b) returns <0 if a<b, 0 if a==b, >0 if a>b.
    // Cmp may be a lambda, function pointer or functor — no allocation, no deps.
    template <typename Cmp>
    void Sort(Cmp comparator)
    {
        for (int i = 0; i < _realSize - 1; i++)
        {
            for (int j = 0; j < _realSize - i - 1; j++)
            {
                if (comparator(_data[j], _data[j + 1]) > 0)
                {
                    // Swap
                    T temp = _data[j];
                    _data[j] = _data[j + 1];
                    _data[j + 1] = temp;
                }
            }
        }
    }

private:
    // Shared scratch element returned for any invalid index (out-of-range or
    // negative) and for Last() on an empty array. Reset to a clean T() on every
    // access so a stray write through a bad-index reference can never leak into
    // a later read. Single-threaded firmware: no reentrancy concern. T is
    // trivially copyable per the class contract, so T() assignment is cheap.
    static T &Default()
    {
        static T safe_default;
        // memset (not `safe_default = T()`) so this also compiles when T is a
        // C array type (e.g. uint8_t[8]): arrays are neither functional-cast-
        // initializable nor assignable. Zeroing is valid per the trivially-
        // copyable contract and matches the memset/memcpy relocation elsewhere.
        // The (void*) cast suppresses -Wclass-memaccess for trivially-copyable
        // types that still have a non-trivial default ctor (e.g. DirEntry) — the
        // same cast used in Realloc().
        memset((void *)&safe_default, 0, sizeof(safe_default));
        return safe_default;
    }

    // Ensure slot `index` is addressable, growing the buffer if needed.
    // NOTE: `index` is the highest slot the CALLER intends to touch — NOT a
    // count and NOT the new length. This method does NOT change _realSize;
    // callers adjust it themselves afterwards (operator[]: index; operator+=:
    // _realSize; Insert: _realSize+1; Reserve: len-1). Newly exposed slots are
    // zeroed (see below) so callers never observe garbage/stale data.
    void Realloc(int index)
    {
        if (index < _size)
        {
            // Capacity is already sufficient. But the caller may be about to
            // expose slots [_realSize .. index] as live elements WITHOUT writing
            // them (Reserve(), or sparse operator[]). Zero that gap so callers
            // never read uninitialized garbage, nor stale data left over by a
            // previous Clear()/Remove(). This matters most for Array<T*>, where
            // a stale pointer could later be delete'd. Slots the caller does
            // write are simply overwritten. (The realloc branch below zeroes its
            // own tail, so it already covers the grow-the-buffer case.)
            if (index >= _realSize)
                memset((void *)(_data + _realSize), 0, (index + 1 - _realSize) * sizeof(T));
            return;
        }
        int newSize = _size > 0 ? _size * 2 : 8;
        if (index >= newSize)
            newSize = index+1;
        T *newData = new T[newSize];
        if (_data)
            memcpy(newData, _data, _realSize * sizeof(T));
        memset((void *)(newData + _realSize), 0, (newSize - _realSize) * sizeof(T));
        delete[] _data;
        _data = newData;
        _size = newSize;
    }
};

template <typename T>
class Queue: private Array<T>
{
public:
    void Push(T val)
    {
        *this += val;
    }

    T Peek()
    {
        if (this->Length() == 0)
            return T();
        return (*this)[0];
    }

    T Pop()
    {
        if (this->Length() == 0)
            return T();
        T ret = this->_data[0];
        this->Remove(0);
        return ret;
    }

    // Unambiguous variants: return false on empty (so a popped/peeked default
    // value, e.g. 0 or nullptr, is not mistaken for "queue was empty").
    bool TryPop(T &out)
    {
        if (this->Length() == 0)
            return false;
        out = Pop();
        return true;
    }

    bool TryPeek(T &out)
    {
        if (this->Length() == 0)
            return false;
        out = Peek();
        return true;
    }

    int Count()
    {
        return this->Length();
    }

    bool Any()
    {
        return this->Length() > 0;
    }

    T* GetQueueRawData()
    {
        return this->GetData();
    }
};

template <typename T>
class Stack: private Array<T>
{
public:
    void Push(T val)
    {
        *this += val;
    }

    T Peek()
    {
        if (this->Length() == 0)
            return T();
        return this->Last();
    }

    T Pop()
    {
        if (this->Length() == 0)
            return T();
        T ret = this->Last();
        this->Remove(this->Length()-1);
        return ret;
    }

    // Unambiguous variants: return false on empty (so a popped/peeked default
    // value, e.g. 0 or nullptr, is not mistaken for "stack was empty").
    bool TryPop(T &out)
    {
        if (this->Length() == 0)
            return false;
        out = Pop();
        return true;
    }

    bool TryPeek(T &out)
    {
        if (this->Length() == 0)
            return false;
        out = Peek();
        return true;
    }

    int Count()
    {
        return this->Length();
    }

    bool Any()
    {
        return this->Length() > 0;
    }
};

