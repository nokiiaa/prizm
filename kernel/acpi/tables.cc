#include <acpi/tables.hh>
#include <serial.hh>
#include <core.hh>
#include <mm/virtual.hh>
#include <lib/util.hh>
#include <panic.hh>

uptr BdaEbdaBaseAddr = 0x40Eu;
// 'RSD PTR '
constexpr u64 RsdpSignature = 0x2052545020445352ull;

RsdpDescriptor *RsdpPointer = nullptr;
IsdtHeader *RsdtPointer = nullptr;
bool Version2 = false;

IsdtHeader *AcpiFindTable(u32 signature)
{
    if (!RsdtPointer)
        return nullptr;

    int headers = RsdtPointer->Length - sizeof(IsdtHeader);

    if (Version2) {
        headers /= sizeof(u64);
        auto start = (u64 *)&RsdtPointer[1];

        for (int i = 0; i < headers; i++) {
            IsdtHeader *hdr = AcpiMapTable((uptr)start[i]);

            if (hdr->Signature == signature)
                return hdr;
            else
                AcpiUnmapTable(hdr);
        }
    }
    else {
        headers /= sizeof(u32);
        auto start = (IsdtHeader **)&RsdtPointer[1];

        for (int i = 0; i < headers; i++) {
            IsdtHeader *hdr = AcpiMapTable((uptr)start[i]);

            if (hdr->Signature == signature)
                return hdr;
            else
                AcpiUnmapTable(hdr);
        }
    }
    return nullptr;
}

IsdtHeader *AcpiMapTable(uptr isdt)
{
    auto *hdr = (IsdtHeader *)(
        (u8 *)MmVirtualMapPhysical(nullptr,
            isdt & -PAGE_SIZE,
            sizeof(IsdtHeader) + PAGE_SIZE, PAGE_READWRITE)
        + isdt % PAGE_SIZE
        );

    u32 map_length = hdr->Length;
    MmVirtualFreeMemory(hdr, sizeof(IsdtHeader));

    return (IsdtHeader *)(
        (u8 *)MmVirtualMapPhysical(nullptr,
            isdt & -PAGE_SIZE, map_length + PAGE_SIZE, PAGE_READWRITE)
        + isdt % PAGE_SIZE);
}

bool AcpiUnmapTable(IsdtHeader *isdt)
{
    return MmVirtualFreeMemory(isdt, isdt->Length);
}

void AcpiInitialize()
{
    // Search EBDA for the RSDP first
    uptr ebda_map = (uptr)MmVirtualMapPhysical(
        nullptr, 0x0000'0000, 4096, PAGE_READWRITE);
    volatile u64 *ebda = (u64 *)(*(u16 *)(ebda_map + BdaEbdaBaseAddr) << 4);

    for (int i = 0; i < 1024 / sizeof(u64); i++)
        if (ebda[i] == RsdpSignature)
            RsdpPointer = (RsdpDescriptor *)(ebda + i);

    MmVirtualFreeMemory((void *)ebda_map, 4096);

    // Search main BIOS area for the RSDP if not found
    if (!RsdpPointer)
        for (int i = 0xE0000; i < 0x100000; i += sizeof(u64))
            if (*(volatile u64 *)i == RsdpSignature)
                RsdpPointer = (RsdpDescriptor *)i;

    if (!RsdpPointer)
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_ACPI, "Could not locate RSDP pointer!");

    RsdtPointer = (IsdtHeader *)(
        (u8 *)MmVirtualMapPhysical(nullptr,
            (uptr)RsdpPointer & -PAGE_SIZE,
            sizeof(IsdtHeader) + PAGE_SIZE,
            PAGE_READWRITE) +
        (uptr)RsdpPointer % PAGE_SIZE);

    RsdtPointer = AcpiMapTable((uptr)((RsdpDescriptor *)RsdtPointer)->RsdtAddress);
}

#include <acpi/madt.hh>

void AcpiInitializeTables()
{
    AcpiMadtInitialize();
}