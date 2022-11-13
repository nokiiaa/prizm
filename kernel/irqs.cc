#include <lib/list.hh>
#include <core.hh>
#include <x86/apic.hh>
#include <x86/i8259a.hh>
#include <processor.hh>
#include <debug.hh>
#include <serial.hh>

#define IRQ_QUEUE_SIZE 256

LinkedList<IrqHandlerFunc> IrqHandlers[16];
volatile int IrqQueue[IRQ_QUEUE_SIZE], IrqQueuePtr = 0;

void PzHandleQueuedInterrupts(CpuInterruptState *state)
{
    int irq;
    while (IrqQueuePtr && PzGetCurrentIrql() < (irq = IrqQueue[IrqQueuePtr - 1]) + 1) {
        IrqQueuePtr--;
        for (auto node = IrqHandlers[irq].First; node; node = node->Next)
            if (node->Value)
                node->Value(state);
    }
}

extern "C" void IrqHandler(CpuInterruptState *state)
{
    PzHandleQueuedInterrupts(state);

    if (state->InterruptNumber == 16)
        return;

    int irq = state->InterruptNumber;

    if (irq + 1 > PzGetCurrentIrql()) {
        for (auto node = IrqHandlers[irq].First; node; node = node->Next)
            if (node->Value)
                node->Value(state);
    }
    else {
        IrqQueue[IrqQueuePtr++] = irq;
        IrqQueuePtr %= IRQ_QUEUE_SIZE;
    }

    switch (PzGetInterruptController()) {
    case INT_CONTROLLER_8259A:
        Hal8259ASendEoi(state->InterruptNumber >= 8);
        break;

    case INT_CONTROLLER_IO_APIC:
        HalApicSendEoi();
        break;
    }
}