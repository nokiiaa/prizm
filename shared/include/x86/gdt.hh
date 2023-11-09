#pragma once

#include <defs.hh>

#pragma pack(push, 1)
extern struct TssStructure {
    u32 Link;
    u32 Esp0, Ss0;
    u32 Esp1, Ss1;
    u32 Esp2, Ss2;
    u32 Cr3, Eip, Eflags, Eax, Ecx, Edx, Ebx, Esp, Ebp, Esi, Edi;
    u32 Es, Cs, Ss, Ds, Fs, Gs, Ldtr;
    u16 _, IopbOffset;
} HalTss;

extern struct GdtDescriptor {
    u16 Limit;
    void *Base;
} HalGdtDesc;

extern u64 HalGdtTable[64];
#pragma pack(pop)

void HalGdtInitialize();
void HalGdtReload();
void HalGdtSetCodeSegment(int index, u32 base, int size, int gr, int privl);
void HalGdtSetDataSegment(int index, u32 base, int size, int gr, int privl);
void HalGdtSetTssSegment(int index, u32 base, int size);