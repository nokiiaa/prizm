#pragma once

#include <defs.hh>

#pragma pack(push, 1)
struct IdtEntry
{
    u16 OffsetLow;
    u16 Selector;
    u8 Zero;
    u8 Flags;
    u16 OffsetHigh;
};

struct
{
    u16 Limit;
    void *Base;
} IdtPointer;
#pragma pack(pop)

extern IdtEntry HalIdtEntries[256];
extern bool HalIsIdtInitialized;
void HalIdtInitialize();