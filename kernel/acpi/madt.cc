#include <acpi/madt.hh>
#include <serial.hh>

MadTable *AcpiMadtGetBase()
{
    // 'APIC'
    return (MadTable *)AcpiFindTable(0x43495041u);
}

LinkedList<MadtCpuEntry> PhysicalCpus;
LinkedList<MadtIoApicEntry> IoApics;
LinkedList<MadtIsoEntry> Isos;
LinkedList<MadtNmiEntry> Nmis;
MadTable *Table;

u32 AcpiMadtGetLocalApicBase()
{
    return Table->LocalApicBase;
}

void AcpiMadtInitialize()
{
    Table = AcpiMadtGetBase();

    /*SerialPrintStr("LAPIC base=%p\r\n", Table->LocalApicBase); */

    u8 *ptr = Table->Entries;
    u8 *ptr_org = ptr;
    int table_size = Table->Header.Length - sizeof(MadTable);

    /* Parse all entries in the MADT */
    /*SerialPrintStr("MADT: table_size=%i table=%p\r\n", table_size, Table); */

    MadtCpuEntry cpu;
    MadtIoApicEntry ioapic;
    MadtIsoEntry iso;
    MadtNmiEntry nmi;

    while (ptr < ptr_org + table_size) {
        int ent_type = *ptr++;
        int ent_reclen = *ptr++;
        switch (ent_type) {
            /* Type 0: processor local APIC */
        case 0:
            cpu = *(MadtCpuEntry *)ptr;
            /*SerialPrintStr("LAPIC: AcpiCpuId=%i ApicId=%i Flags=%p\r\n", cpu.AcpiCpuId, cpu.ApicId, cpu.Flags);*/
            PhysicalCpus.Add(cpu);
            break;

            /* Type 1: I/O APIC */
        case 1:
            ioapic = *(MadtIoApicEntry *)ptr;
            /*SerialPrintStr("I/O APIC: IoApicId=%i Base=%p GSIBase=%p\r\n",
                ioapic.IoApicId, ioapic.Base, ioapic.GlobalSysIntBase); */
            IoApics.Add(ioapic);
            break;

            /* Type 2: Interrupt Source Override */
        case 2:
            iso = *(MadtIsoEntry *)ptr;
            /*SerialPrintStr("ISO: BusSource=%i IrqSource=%i GlobalSysInt=%i Flags=%p\r\n",
                iso.BusSource, iso.IrqSource, iso.GlobalSysInt, int(iso.Flags)); */
            Isos.Add(iso);
            break;

            /* Type 4: NMIs */
        case 4:
            nmi = *(MadtNmiEntry *)ptr;
            /*SerialPrintStr("NMI: IoApicId=%i Flags=%i Lint=%i\r\n",
                int(nmi.AcpiCpuId), int(nmi.Flags), int(nmi.Lint));*/
            Nmis.Add(nmi);
            break;

            /* Type 5: LAPIC Address Override (this is for x64, so ignore it) */
        case 5:
            break;
        }

        ptr += ent_reclen - 2;
    }
}

LinkedList<MadtCpuEntry> *AcpiMadtGetPhysicalCpus()
{
    return &PhysicalCpus;
}

LinkedList<MadtIoApicEntry> *AcpiMadtGetIoApics()
{
    return &IoApics;
}

LinkedList<MadtIsoEntry> *AcpiMadtGetIsos()
{
    return &Isos;
}

LinkedList<MadtNmiEntry> *AcpiMadtGetNmis()
{
    return &Nmis;
}