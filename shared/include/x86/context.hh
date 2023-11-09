#pragma once

#include <defs.hh>

struct PzThreadContext
{
    u32 Eflags;
    u32 Eax, Ecx, Edx, Ebx;
    u32 Esp, Ebp, Esi, Edi;
    u32 Eip;
    u32 Cs, Ds, Es, Fs, Gs, Ss;
    u32 Cr3;
#ifdef DEBUGGER_INCLUDE
    uptr FxSaveRegion;
#else
    void *FxSaveRegion;
#endif
};