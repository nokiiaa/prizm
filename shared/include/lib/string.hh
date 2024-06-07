#pragma once

#include <defs.hh>

#define PZ_CONST_STRING(x) (PzString {sizeof(x) - 1, (char*)(x)})

/* A structure describing the codepoint count, size and location of a UTF-8 encoded string. */
struct PzString
{
    u32 Size;
#ifdef DEBUGGER_INCLUDE
    DEBUGGER_TARGET_PTR Buffer;
#else
    char *Buffer;
#endif
};

/* Calculates the length of a null-terminated integral sequence (most commonly a string). */
template<typename T = char> inline int StringLength(T *data)
{
    T *ptr = data;
    while (*ptr)
        ptr++;
    return ptr - data;
}

/* Reverses a null-terminated integral sequence (most commonly a string). */
template<typename T = char> inline void ReverseString(T *s)
{
    T *end = s + StringLength(s) - 1;
    while (s < end) {
        *s ^= *end; *end ^= *s; *s ^= *end;
        s++;
        end--;
    }
}

/* Outputs the string representation of an integer to the given buffer. */
template<int base = 10, typename T = i32>
inline int IntToString(T value, char *buffer,
    int padding = 0, char padder = 0,
    bool left_justify = false,
    bool uppercase = false)
{
    static_assert(base >= 2 && base <= 36, "Base must be in the range of [2, 36]");

    const wchar_t chars[] = L"0123456789abcdefghijklmnopqrstuvwxyz";
    int i = 0;
    bool negative = false;
    if (!padder)
        padder = ' ';
    if (T(0) > T(-1) && value < 0) {
        negative = true;
        value = -value;
    }
    do {
        char ch = chars[value % base];
        if (ch >= 'a' && uppercase)
            ch -= 0x20;
        buffer[i++] = ch;
    } while (value /= base);
    int pad = padding - i;
    if (negative)
        buffer[i++] = '-';
    for (int j = 0; !left_justify && j < pad; j++)
        buffer[i++] = padder;
    buffer[i] = 0;
    ReverseString(buffer);
    for (int j = 0; left_justify && j < pad; j++)
        buffer[i++] = padder;
    buffer[i] = 0;
    return i;
}

/* Outputs the string representation of a floating-point number to the given buffer. */
template<int base = 10>
inline int FloatToString(long double value, char *buffer, int precision = 4,
    int padding = 0, char padder = 0,
    bool left_justify = false,
    bool uppercase = false)
{
    const char chars[] = "0123456789abcdef";
    int i = 0;
    bool negative = false;
    if (!padder)
        padder = ' ';
    if (precision <= 0)
        precision = 4;
    i = IntToString<base, i64>(i64(value), buffer, 0, 0, 0, uppercase);
    buffer[i++] = '.';
    long double fract = value - (i64)value;
    while (precision > 0) {
        wchar_t ch = chars[(i64)(fract *= base) % base];
        if (uppercase && ch >= 'a')
            ch -= 0x20;
        buffer[i++] = ch;
        precision--;
    }
    int pad = padding - i;
    for (int j = 0; pad > j; j++)
        buffer[i++] = padder;
    if (!left_justify && pad > 0) {
        ReverseString(buffer);
        ReverseString(buffer + pad);
    }
    buffer[i] = 0;
    return i;
}

/* Converts an ASCII string to a 32-bit unsigned integer. */
inline const char *ParseUint(const char *buffer, int *out, int *length)
{
    *out = 0;
    *length = 0;
    while (*buffer >= '0' && *buffer <= '9') {
        *out *= 10;
        *out += *buffer - '0';
        buffer++;
        (*length)++;
    }
    return buffer;
}

/* Converts an ASCII character to its lowercase counterpart. */
inline char CharToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    return c;
}

/* Allocates a string object of size 0 on the heap. */
PZ_KERNEL_EXPORT PzString *PzAllocateString();

/* Initializes a string object from the given ASCII string pointer without cloning it. */
PZ_KERNEL_EXPORT void PzInitStringViewAscii(PzString *dest, char *src);

/* Initializes a string object from the given UTF-8 string pointer without cloning it. */
PZ_KERNEL_EXPORT void PzInitStringViewUtf8(PzString *dest, char *src);

/* Initializes a string object from the given ASCII string pointer, cloning the string at that pointer. */
PZ_KERNEL_EXPORT void PzInitStringAscii(PzString *dest, const char *src);

/* Initializes a string object from the given UTF-8 string pointer, cloning the string at that pointer. */
PZ_KERNEL_EXPORT void PzInitStringUtf8(PzString *dest, const char *src);

/* Creates a new string object with identical contents to the given string object. */
PZ_KERNEL_EXPORT PzString *PzDuplicateString(const PzString *src);

/* Validates a string object and its character buffer for accessibility,
   with a flag to check if it is accessible in usermode. */
PZ_KERNEL_EXPORT bool PzValidateString(bool as_usermode, const PzString *str, bool write);

/* Frees a string object and its character buffer. */
PZ_KERNEL_EXPORT void PzFreeString(PzString *str);

/* Given a UTF-8 string pointer, gets the value of a codepoint at a certain index in the string. */
PZ_KERNEL_EXPORT int Utf8CharAtRaw(const char *str, int index);

/* Prepends the source string to the destination string, modifiying the destination string. */
PZ_KERNEL_EXPORT void PzPrependString(PzString *dest, const PzString *src);
/* Appends the source string to the destination string, modifiying the destination string. */
PZ_KERNEL_EXPORT void PzAppendString(PzString *dest, const PzString *src);

/* Given two UTF-8 string pointers, returns 0 if all of their codepoints are
   identical, returning other values otherwise. */
PZ_KERNEL_EXPORT int Utf8CompareRawStrings(
    const char *a, int len1,
    const char *b, int len2);

/* Given two UTF-8 string pointers, returns 0 if all of their codepoints are
   identical by their lowercase counterparts, returning other values otherwise. */
PZ_KERNEL_EXPORT int Utf8CompareRawStringsCaseIns(
    const char *a, int len1,
    const char *b, int len2);

/* Duplicates the contents of a UTF-8 string pointer. */
PZ_KERNEL_EXPORT char *Utf8DuplicateRawString(const char *str);

/* Returns the number of codepoints that the given UTF-8 string
   consists of up until its null terminator. */
PZ_KERNEL_EXPORT int Utf8RawStringLength(const char *str);

/* Returns the number of bytes that the given UTF-8 string takes
   to encode up until its null terminator. */
PZ_KERNEL_EXPORT int Utf8RawStringSize(const char *str);

/* Copies the contents of the source UTF-8 string pointer up until its null
   terminator to the destination UTF-8 string pointer. */
PZ_KERNEL_EXPORT void Utf8RawStringCopy(char *dest, const char *src);

/* Encodes a given UTF-8 codepoint as a sequence of bytes, writing it
   to the destination buffer and returning the incremented destination pointer. */
PZ_KERNEL_EXPORT char *Utf8EncodeNext(char *buffer, int codepoint);

/* Decodes a sequence of bytes at the source buffer as a UTF-8 codepoint, returning
   the incremented source buffer and the decoded codepoint. */
PZ_KERNEL_EXPORT const char *Utf8DecodeNext(const char *str, int &value);