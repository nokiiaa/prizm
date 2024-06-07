#include <x86/i8259a.hh>
#include <x86/port.hh>

#define PIC_MASTER_BASE 0x20
#define PIC_SLAVE_BASE 0xA0
#define PIC_MASTER_COMMAND (PIC_MASTER_BASE + 0)
#define PIC_MASTER_DATA    (PIC_MASTER_BASE + 1)
#define PIC_SLAVE_COMMAND  (PIC_SLAVE_BASE  + 0)
#define PIC_SLAVE_DATA     (PIC_SLAVE_BASE  + 1)
#define PIC_EOI 0x20
#define PIC_READ_IRR 0xA
#define PIC_READ_ISR 0xB

u8 Hal8259ARead(bool slave)
{
    return HalPortIn8(slave ? PIC_SLAVE_DATA : PIC_MASTER_DATA);
}

u8 Hal8259AReadCommand(bool slave)
{
    return HalPortIn8(slave ? PIC_SLAVE_COMMAND : PIC_MASTER_COMMAND);
}

void Hal8259AWrite(u8 data, bool slave)
{
    HalPortOut8(slave ? PIC_SLAVE_DATA : PIC_MASTER_DATA, data);
}

void Hal8259AWriteCommand(u8 data, bool slave)
{
    HalPortOut8(slave
        ? PIC_SLAVE_COMMAND
        : PIC_MASTER_COMMAND, data);
}

void Hal8259AMaskAssign(u16 mask)
{
    Hal8259AWrite(mask & 0xFF, false);
    Hal8259AWrite(mask >> 8,   true);
}

void Hal8259AMaskSingleSet(int irq)
{
    bool slave = irq & 8;
    irq %= 8;
    Hal8259AWrite(Hal8259ARead(slave) | (1 << irq), slave);
}

void Hal8259AMaskSingleClear(int irq)
{
    bool slave = irq & 8;
    irq %= 8;
    Hal8259AWrite(Hal8259ARead(slave) & ~(1 << irq), slave);
}

void Hal8259ASendEoi(bool to_slave)
{
    // Send EOI to slave PIC
    if (to_slave)
        Hal8259AWriteCommand(PIC_EOI, true);

    // Send EOI to master PIC
    Hal8259AWriteCommand(PIC_EOI, false);
}

bool Hal8259AIsSpuriousIrq(int irq)
{
    Hal8259AWriteCommand(PIC_READ_ISR, false);
    Hal8259AWriteCommand(PIC_READ_ISR, true);

    u16 comb = Hal8259ARead(true) << 8 | Hal8259ARead(false);

    // If the IRQ's bit is not set in the In-Service Register,
    // this is a spurious IRQ.
    return !(comb & (1 << irq));
}

void Hal8259ADisable()
{
    // Mask all IRQs
    Hal8259AWrite(0xFF, false);
    Hal8259AWrite(0xFF, true);
}

void Hal8259AInitialize(int remap_master, int remap_slave)
{
    // Initialize PICs
    Hal8259AWriteCommand(0x11, false);
    Hal8259AWriteCommand(0x11, true);

    // Remap IRQs to the offsets
    Hal8259AWrite(remap_master, false);
    Hal8259AWrite(remap_slave, true);

    // IR line 2
    Hal8259AWrite(0x04, false);
    Hal8259AWrite(0x02, true);

    // Set x86 mode
    Hal8259AWrite(1, false);
    Hal8259AWrite(1, true);

    // Null the data registers
    Hal8259AWrite(0, false);
    Hal8259AWrite(0, true);
}