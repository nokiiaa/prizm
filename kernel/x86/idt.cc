#include <x86/idt.hh>
#include <x86/i8259a.hh>
#include <debug.hh>
#include <core.hh>

IdtEntry HalIdtEntries[256];
bool HalIsIdtInitialized;

extern "C"
{
    extern void *HalIdtHandlerArray[48+1];
    extern void HalSyscallEntry();
    extern void HalLoadIdt(void *ptr);
}

void HalIdtInitialize(void)
{
    // Load IDT
    IdtPointer.Limit = sizeof(IdtEntry) * 256 - 1;
    IdtPointer.Base = HalIdtEntries;

    Hal8259AInitialize(0x20, 0x28);
    PzSetInterruptController(INT_CONTROLLER_8259A);

    for (int i = 0; i < 48+1; i++) {
        HalIdtEntries[i].OffsetLow  = (u32)HalIdtHandlerArray[i] >> 0  & 0xFFFF;
        HalIdtEntries[i].OffsetHigh = (u32)HalIdtHandlerArray[i] >> 16 & 0xFFFF;
        HalIdtEntries[i].Selector   = 0x8;
        switch (i) {
        case 1:
        case 3:
        case 4:
            /* 32-bit trap gate, present */
            HalIdtEntries[i].Flags = 0xF | 0x80;
            break;
        default:
            /* 32-bit interrupt gate, present */
            HalIdtEntries[i].Flags = 0xE | 0x80;
            break;
        }
    }
    
    /* Set up IDT entry for syscall */
    HalIdtEntries[0x80].OffsetLow  = (u32)HalSyscallEntry >> 0  & 0xFFFF;
    HalIdtEntries[0x80].OffsetHigh = (u32)HalSyscallEntry >> 16 & 0xFFFF;
    HalIdtEntries[0x80].Selector   = 0x8;
    /* 32-bit interrupt gate, present, minimum CPL 3 */
    HalIdtEntries[0x80].Flags      = 0xE | 0x80 | 3 << 5;

    HalLoadIdt(&IdtPointer);
    DbgPrintStr("[HalIdtInitialize] IDT successfully initialized\r\n");
    HalIsIdtInitialized = true;
}