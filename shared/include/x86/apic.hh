#pragma once

#include <defs.hh>
#include <x86/port.hh>
#include <core.hh>

union IoApicRedirection
{
    struct
    {
        u64 IntVector : 8;
        // 0 - Normal
        // 1 - Low priority
        // 2 - SM interrupt
        // 4 - NMI
        // 5 - INIT
        // 7 - External
        u64 DeliveryMode : 3;
        // 0 - Physical
        // 1 - Logical
        u64 DestinationMode : 1;
        u64 DeliveryStatus : 1;
        // 0 - Low is active
        // 1 - High is active
        u64 Polarity : 1;
        u64 Received : 1;
        // 0 - Edge sensitive
        // 1 - Level sensitive
        u64 TriggerMode : 1;
        u64 Mask : 1;
        u64 : 39;
        u64 Destination : 8;
    };
    struct
    {
        u32 LoWord;
        u32 HiWord;
    };
};

u32 HalApicGetBase();
void HalApicSetBase(u32 base);
void HalApicInitialize();
void HalApicMapIoApic();
void HalApicSendEoi();
bool HalApicSelectIoApic(int index);
IoApicRedirection HalIoApicReadRedirection(int index);
void HalIoApicWriteRedirection(int index, IoApicRedirection redir);
void HalApicSetLvtMask(int entry);
void HalApicClearLvtMask(int entry);
u32 HalIoApicRead(int reg);
void HalIoApicWrite(int reg, u32 value);
void HalApicStartTimer(int irq, float ms);