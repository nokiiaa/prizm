#include <x86/gdt.hh>
#include <debug.hh>

TssStructure HalTss;
GdtDescriptor HalGdtDesc;
u64 HalGdtTable[64];

extern "C" void HalLoadGdt(void *desc, int tss_descriptor);

u64 MakeEntry(u32 base, u32 limit,
    u8 rw, u8 dc, u8 ex, u8 s, u8 privl,
    u8 sz, u8 gr)
{
    return
        u64(limit & 0x00FFFF) << 0 |
        u64(base & 0xFFFFFF) << 16 |
        u64(rw) << 41 |
        u64(dc) << 42 |
        u64(ex) << 43 |
        u64(s) << 44 |
        u64(privl) << 45 |
        1ull << 47 |
        u64(limit >> 16 & 0xF) << 48 |
        u64(sz) << 54 |
        u64(gr) << 55 |
        u64(base >> 24 & 0xFF) << 56;
}

void HalGdtInitialize()
{
    HalGdtTable[0] = 0;

    /* Kernelmode segments: code (cs), data (ds/es/ss), kernel system data (fs) */
    HalGdtSetCodeSegment(1, 0, 0xFFFFF, 1, 0);
    HalGdtSetDataSegment(2, 0, 0xFFFFF, 1, 0);
    HalGdtSetDataSegment(3, 0, 0xFFFFF, 1, 0);

    /* Usermode segments:   code (cs), data (ds/es/ss), user system data (fs) */
    HalGdtSetCodeSegment(4, 0, 0xFFFFF, 1, 3);
    HalGdtSetDataSegment(5, 0, 0xFFFFF, 1, 3);
    HalGdtSetDataSegment(6, 0, 0xFFFFF, 1, 3);
    HalGdtSetTssSegment(7, (u32)&HalTss, sizeof(HalTss) - 1);

    HalGdtReload();

    DbgPrintStr("[HalGdtInitialize] GDT successfully initialized\r\n");
}

void HalGdtReload()
{
    HalGdtDesc.Limit = sizeof(HalGdtTable) - 1;
    HalGdtDesc.Base = &HalGdtTable;
    HalLoadGdt(&HalGdtDesc, 7 * 8 | 3);
}

void HalGdtSetCodeSegment(int index, u32 base,
    int size, int gr, int privl)
{
    HalGdtTable[index] = MakeEntry(base, size, 0, 0, 1, 1, privl, 1, gr);
}

void HalGdtSetTssSegment(int index, u32 base, int size)
{
    HalGdtTable[index] = MakeEntry(base, size, 0, 0, 1, 0, 3, 1, 0) | 1ull << 40;
}

void HalGdtSetDataSegment(int index, u32 base,
    int size, int gr, int privl)
{
    HalGdtTable[index] = MakeEntry(base, size, 1, 0, 0, 1, privl, 1, gr);
}