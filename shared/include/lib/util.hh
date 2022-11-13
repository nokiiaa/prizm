#pragma once

#include <defs.hh>

#define PAGE_SHIFT 12
#define PAGE_SIZE (1u << (PAGE_SHIFT))
#define ALIGN(bytes, align) ((bytes) + (align) - 1 & -(align))
#define PAGES_IN(bytes) (ALIGN(bytes, PAGE_SIZE) / (PAGE_SIZE))
#define NON_OVERLAPPING(a1, a2, b1, b2) ((a2) < (b1) || (a1) > (b2))
#define OVERLAPPING(a1, a2, b1, b2) (!NON_OVERLAPPING(a1, a2, b1, b2))

template<class T> struct RemoveRef { typedef T Type; };
template<class T> struct RemoveRef<T &> { typedef T Type; };
template<class T> struct RemoveRef<T &&> { typedef T Type; };

#define RELOCATE(addr, old_base, new_base) \
    ((RemoveRef<decltype(addr)>::Type)(uptr(addr) - (old_base) + (new_base)))

template<typename T> inline T Min(T a, T b) { return a > b ? b : a; }
template<typename T> inline T Max(T a, T b) { return a > b ? a : b; }
template<typename T> inline T Clamp(T x, T a, T b)
{
    return Max(a, Min(x, b));
}

template<typename T> inline void Swap(T &a, T &b)
{
    T t = a;
    a = b;
    b = t;
}

PZ_KERNEL_EXPORT void MemCopy(void *dest, const void *src, int count);
PZ_KERNEL_EXPORT void MemSet(void *dest, int value, int count);

inline int HighestSetBit(int value)
{
#if __GNUC__
    return 32 - __builtin_clz(value);
#else
    int i;
    for (i = -1; value; i++, value >>= 1);
    return i;
#endif
}