#include <lib/string.hh>
#include <lib/util.hh>
#include <lib/malloc.hh>

char *Utf8EncodeNext(char *buffer, int codepoint)
{
    int lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 2,
        2, 2, 2, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    switch (lengths[HighestSetBit(codepoint)])
    {
    case 1:
        *buffer++ = codepoint;
        return buffer;

    case 2:
        *buffer++ = 0b11000000 | codepoint >> 6;
        *buffer++ = 0b10000000 | codepoint >> 0 & 0b111111;
        return buffer;

    case 3:
        *buffer++ = 0b11100000 | codepoint >> 12 & 0b1111;
        *buffer++ = 0b10000000 | codepoint >> 6 & 0b111111;
        *buffer++ = 0b10000000 | codepoint >> 0 & 0b111111;
        return buffer;

    case 4:
        *buffer++ = 0b11110000 | codepoint >> 18 & 0b111;
        *buffer++ = 0b10000000 | codepoint >> 12 & 0b111111;
        *buffer++ = 0b10000000 | codepoint >> 6 & 0b111111;
        *buffer++ = 0b10000000 | codepoint >> 0 & 0b111111;
        return buffer;

    default:
        return 0;
    }
}

const char *Utf8DecodeNext(const char *str, int &value)
{
    int lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        2, 2, 2, 2, 3, 3, 4, 0
    },
        masks[] = { 0, 0b1111111, 0b11111, 0b1111, 0b111 },
        shifts[] = { 0, 18, 12, 6, 0 };

    int len = lengths[(u8)str[0] >> 3];

    value =
        (str[0] & masks[len]) << 18 |
        (str[1] & 0b111111) << 12 |
        (str[2] & 0b111111) << 6 |
        (str[3] & 0b111111) << 0;
    value >>= shifts[len];

    return str + len;
}

int Utf8CompareRawStrings(
    const char *a, int len1,
    const char *b, int len2)
{
    int v1, v2;

    do {
        a = Utf8DecodeNext(a, v1);
        b = Utf8DecodeNext(b, v2);
        len1--, len2--;
    } while (len1 && len2 && v1 && v1 == v2);

    return v1 - v2;
}

int Utf8CompareRawStringsCaseIns(
    const char *a, int len1,
    const char *b, int len2)
{
    int v1, v2;

    do {
        a = Utf8DecodeNext(a, v1);
        b = Utf8DecodeNext(b, v2);
        len1--, len2--;
    } while (len1 && len2 && v1 && CharToLower(v1) == CharToLower(v2));

    return CharToLower(v1) - CharToLower(v2);
}

int Utf8CharAtRaw(const char *str, int index)
{
    int value;
    for (int i = 0; i <= index; i++)
        str = Utf8DecodeNext(str, value);
    return value;
}

char *Utf8DuplicateRawString(const char *str)
{
    int length = Utf8RawStringSize(str);
    char *dup = new char[length + 1];
    Utf8RawStringCopy(dup, str);
    return dup;
}

int Utf8RawStringLength(const char *str)
{
    int length = 0, value = -1;
    while (value) {
        str = Utf8DecodeNext(str, value);
        length++;
    }
    return length;
}

int Utf8RawStringSize(const char *str)
{
    int length = 0;
    while (*str) str++, length++;
    return length;
}

void Utf8RawStringCopy(char *dest, const char *src)
{
    while (*src)
        *dest++ = *src++;
    *dest++ = '\0';
}

PzString *PzAllocateString()
{
    PzString *str = (PzString *)PzHeapAllocate(sizeof(PzString), 0);

    if (!str)
        return nullptr;

    str->Buffer = nullptr;
    str->Size = 0;
    return str;
}

void PzInitStringViewAscii(PzString *dest, char *src)
{
    dest->Buffer = src;
    dest->Size = Utf8RawStringSize(src);
}

void PzInitStringViewUtf8(PzString *dest, char *src)
{
    int length = Utf8RawStringSize(src);
    dest->Buffer = src;
    dest->Size = length;
}

void PzInitStringAscii(PzString *dest, const char *src)
{
    int length = Utf8RawStringSize(src);
    dest->Buffer = new char[length + 1];
    dest->Size = length;
    Utf8RawStringCopy(dest->Buffer, src);
}

void PzInitStringUtf8(PzString *dest, const char *src)
{
    int length = Utf8RawStringSize(src);
    dest->Buffer = new char[length + 1];
    dest->Size = length;
    Utf8RawStringCopy(dest->Buffer, src);
}

#include <debug.hh>

PzString *PzDuplicateString(const PzString *src)
{
    PzString *dest = PzAllocateString();

    if (!dest)
        return nullptr;

    PzInitStringUtf8(dest, src->Buffer);
    return dest;
}

void PzFreeString(PzString *str)
{
    delete str->Buffer;
    delete str;
}

void PzPrependString(PzString *dest, const PzString *src)
{
    int new_size = src->Size + dest->Size;
    char *new_buffer = new char[new_size + 1];

    if (src->Buffer)  Utf8RawStringCopy(new_buffer, src->Buffer);
    if (dest->Buffer) { Utf8RawStringCopy(new_buffer + src->Size, dest->Buffer); delete dest->Buffer; }

    dest->Buffer = new_buffer;
    dest->Size = new_size;
}

void PzAppendString(PzString *dest, const PzString *src)
{
    int new_size = src->Size + dest->Size;
    char *new_buffer = new char[new_size + 1];

    if (dest->Buffer) { Utf8RawStringCopy(new_buffer, dest->Buffer); delete dest->Buffer; }
    if (src->Buffer)  Utf8RawStringCopy(new_buffer + dest->Size, src->Buffer);

    dest->Buffer = new_buffer;
    dest->Size = new_size;
}

bool PzValidateString(bool as_usermode, const PzString *str, bool write)
{
    return MmVirtualProbeMemory(as_usermode, (uptr)str, sizeof(PzString), write) &&
        MmVirtualProbeMemory(as_usermode, (uptr)str->Buffer, str->Size + 1, write);
}