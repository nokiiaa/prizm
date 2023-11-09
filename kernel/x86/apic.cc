#include <x86/apic.hh>
#include <x86/i8259a.hh>
#include <acpi/madt.hh>
#include <x86/port.hh>

constexpr int ApicBaseMsr = 0x1B;
volatile u32 *LapicRegisters;
#define REG_ID               (0x20 / 4)
#define REG_VERSION          (0x30 / 4)
#define REG_TPR              (0x80 / 4)
#define REG_APR              (0x90 / 4)
#define REG_PPR              (0xA0 / 4)
#define REG_EOI              (0xB0 / 4)
#define REG_RRD              (0xC0 / 4)
#define REG_LOG_DEST         (0xD0 / 4)
#define REG_DEST_FORMAT      (0xE0 / 4)
#define REG_SPURIOUS         (0xF0 / 4)
#define REG_ISR              (0x100 / 4)
#define REG_TMR              (0x180 / 4)
#define REG_IRR              (0x200 / 4)
#define REG_ERROR            (0x280 / 4)
#define REG_LVT_CMCI         (0x2F0 / 4)
#define REG_ICR              (0x300 / 4)
#define REG_LVT_TIMER        (0x320 / 4)
#define REG_THERMAL_SENSOR   (0x330 / 4)
#define REG_PERFMON_COUNTERS (0x340 / 4)
#define REG_LINT0            (0x350 / 4)
#define REG_LINT1            (0x360 / 4)
#define REG_ERR_REG          (0x370 / 4)
#define REG_INIT_COUNT       (0x380 / 4)
#define REG_CURR_COUNT       (0x390 / 4)
#define REG_DIV_CONFIG       (0x3E0 / 4)

static bool Enabled = false;

u32 HalApicGetBase()
{
    u32 eax, edx;
    HalMsrRead(ApicBaseMsr, &eax, &edx);
    u32 base = eax & 0xFFFFF000;
    LapicRegisters = (volatile u32 *)base;
    return base;
}

void HalApicSetBase(u32 base)
{
    if (LapicRegisters)
        MmVirtualFreeMemory((void *)LapicRegisters, 1024);

    LapicRegisters = (volatile u32 *)MmVirtualMapPhysical(nullptr, base, 1024, PAGE_READWRITE);
    HalMsrWrite(ApicBaseMsr, base & 0xFFFFF000 | 1 << 11, 0);
}

void HalApicInitialize()
{
    //Hal8259ADisable();
    // Hardware enable APIC
    HalApicSetBase(HalApicGetBase());

    LapicRegisters[REG_DEST_FORMAT] = 0xFFFFFFFF;
    LapicRegisters[REG_LOG_DEST] =
        (LapicRegisters[REG_LOG_DEST] & 0xFFFFFF) | 1;
    LapicRegisters[REG_TPR] = 0;
    LapicRegisters[REG_PERFMON_COUNTERS] = 4 << 8;
    LapicRegisters[REG_LINT0] = 1 << 16;
    LapicRegisters[REG_LINT1] = 1 << 16;
    LapicRegisters[REG_LVT_TIMER] = 1 << 16;

    // Software enable APIC
    // Set bit 8 of spurious interrupt register
    LapicRegisters[REG_SPURIOUS] |= /*32 + 7 |*/ 1 << 8;
    PzSetInterruptController(INT_CONTROLLER_IO_APIC);
}

void HalApicMapIoApic()
{
    HalApicSelectIoApic(0);

    // Set I/O APIC redirections
    for (int i = 0; i < 24; i++) {
        IoApicRedirection redir = { 0, 0 };
        redir.IntVector = 32 + i;
        redir.DeliveryMode = 0;
        redir.DestinationMode = 0;
        redir.DeliveryStatus = 0;
        redir.Polarity = 0;
        redir.TriggerMode = 0;
        redir.Mask = 0;
        redir.Destination = 0;

        HalIoApicWriteRedirection(i, redir);
    }
}

volatile bool SleepFinished = false;

void SleepFinishedHandler(CpuInterruptState *state)
{
    SleepFinished = true;
}

void HalApicSendEoi()
{
    LapicRegisters[REG_EOI] = 0;
}

static MadtIoApicEntry *CurrentIoApic;
static u32 *CurrentIoApicVirtualBase;

bool HalApicSelectIoApic(int index)
{
    auto *node = AcpiMadtGetIoApics()->First;
    CurrentIoApic = &node->Value;

    for (int i = 0; i < index; i++) {
        if (!node)
            return false;

        CurrentIoApic = &node->Value;
        node = node->Next;
    }

    if (CurrentIoApicVirtualBase)
        MmVirtualFreeMemory(CurrentIoApicVirtualBase, 32);

    CurrentIoApicVirtualBase = (u32 *)MmVirtualMapPhysical(nullptr,
        CurrentIoApic->Base, 32, PAGE_READWRITE);

    return true;
}

IoApicRedirection HalIoApicReadRedirection(int index)
{
    IoApicRedirection redir;
    redir.LoWord = HalIoApicRead(0x10 + index * 2 + 0);
    redir.HiWord = HalIoApicRead(0x10 + index * 2 + 1);
    return redir;
}

void HalIoApicWriteRedirection(int index, IoApicRedirection redir)
{
    HalIoApicWrite(0x10 + index * 2 + 0, redir.LoWord);
    HalIoApicWrite(0x10 + index * 2 + 1, redir.HiWord);
}

void HalApicSetLvtMask(int entry)
{
    LapicRegisters[REG_LVT_TIMER + entry] |= 1u << 16;
}

void HalApicClearLvtMask(int entry)
{
    LapicRegisters[REG_LVT_TIMER + entry] &= ~(1u << 16);
}

u32 HalIoApicRead(int reg)
{
    *(volatile u32 *)(CurrentIoApicVirtualBase + 0x00) = reg;
    return *(volatile u32 *)(CurrentIoApicVirtualBase + 0x10);
}

void HalIoApicWrite(int reg, u32 value)
{
    *(volatile u32 *)(CurrentIoApicVirtualBase + 0x00) = reg;
    *(volatile u32 *)(CurrentIoApicVirtualBase + 0x10) = value;
}

#include <x86/cmos.hh>
#include <serial.hh>

void HalApicStartTimer(int irq, float ms)
{
    const int RtcResolution = 1000;
    LapicRegisters[REG_LVT_TIMER] = 32 + irq;
    LapicRegisters[REG_DIV_CONFIG] = 3;

    CmosWaitForChange(0);
    LapicRegisters[REG_INIT_COUNT] = -1u;
    CmosWaitForChange(0);

    /* Mask timer IRQ */
    LapicRegisters[REG_LVT_TIMER] = 1 << 16;
    int ticks = ~LapicRegisters[REG_CURR_COUNT];

    /* Scale amount of ticks to needed value */
    ticks = ticks * ms / RtcResolution;

    /* Set up APIC timer (and unmask the IRQ) */
    LapicRegisters[REG_INIT_COUNT] = ticks;
    LapicRegisters[REG_LVT_TIMER] = 32 + irq | 1 << 17;
    LapicRegisters[REG_DIV_CONFIG] = 3;
}