#include <acpi/tables.hh>
#include <lib/list.hh>

#pragma pack(push, 1)
struct MadTable
{
    IsdtHeader Header;
    u32 LocalApicBase;
    u32 Flags;
    u8 Entries[0];
};

struct MadtCpuEntry
{
    u8 AcpiCpuId;
    u8 ApicId;
    int Flags;
};

struct MadtIoApicEntry
{
    u8 IoApicId;
    u8 Reserved;
    u32 Base;
    u32 GlobalSysIntBase;
};

struct MadtIsoEntry
{
    u8 BusSource;
    u8 IrqSource;
    u32 GlobalSysInt;
    u16 Flags;
};


struct MadtNmiEntry
{
    u8 AcpiCpuId;
    u16 Flags;
    u8 Lint;
};

struct MadtLocalApicOverride
{
    u16 Reserved;
    u64 Base;
};
#pragma pack(pop)

MadTable *AcpiMadtGetBase();
void AcpiMadtInitialize();
u32 AcpiMadtGetLocalApicBase();
LinkedList<MadtCpuEntry> *AcpiMadtGetPhysicalCpus();
LinkedList<MadtIoApicEntry> *AcpiMadtGetIoApics();
LinkedList<MadtIsoEntry> *AcpiMadtGetIsos();
LinkedList<MadtNmiEntry> *AcpiMadtGetNmis();