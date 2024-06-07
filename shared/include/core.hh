#pragma once

#include <defs.hh>
#include <lib/string.hh>

#define INT_CONTROLLER_8259A 1
#define INT_CONTROLLER_IO_APIC 2

struct CpuInterruptState
{
    u32 Gs, Fs, Es, Ds;
    u32 Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    u32 InterruptNumber, ErrorCode;
    u32 Eip, Cs, Eflags, EspU, SsU;
};

typedef void (*IrqHandlerFunc)(CpuInterruptState *state);

PZ_KERNEL_EXPORT int PzReadIfFlag();
PZ_KERNEL_EXPORT void PzDisableInterrupts();
PZ_KERNEL_EXPORT void PzEnableInterrupts();
PZ_KERNEL_EXPORT void PzHaltCpu();
PZ_KERNEL_EXPORT void PzInstallIrqHandler(int irq, IrqHandlerFunc handler);
PZ_KERNEL_EXPORT void PzUninstallIrqHandler(int irq, IrqHandlerFunc handler);
PZ_KERNEL_EXPORT void PzAssert(bool condition, const char *error, ...);
PZ_KERNEL_EXPORT void PzPushAddressSpace(uptr addr_space_desc);
PZ_KERNEL_EXPORT void PzPopAddressSpace();
PZ_KERNEL_EXPORT void PzSetInterruptController(int controller);
PZ_KERNEL_EXPORT int PzGetInterruptController();
PZ_KERNEL_EXPORT void PzEnterCriticalRegion();
PZ_KERNEL_EXPORT void PzLeaveCriticalRegion();