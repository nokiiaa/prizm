#include <core.hh>
#include <lib/list.hh>
#include <lib/string.hh>
#include <obj/process.hh>
#include <debug.hh>

extern "C" void HalDisableInterrupts();
extern "C" void HalEnableInterrupts();
extern "C" void HalHaltCpu();
extern "C" int  HalReadIfFlag();
extern "C" void HalWriteIfFlag(int flag);
extern "C" uptr HalReadCr3();
extern "C" void HalSwitchPageTable(uptr cr3);

void PzDisableInterrupts()
{
    HalDisableInterrupts();
}

void PzEnableInterrupts()
{
    HalEnableInterrupts();
}

void PzHaltCpu()
{
    HalHaltCpu();
}

int PzReadIfFlag()
{
    return HalReadIfFlag();
}

extern LinkedList<IrqHandlerFunc> IrqHandlers[16];

void PzInstallIrqHandler(int irq, IrqHandlerFunc handler)
{
    PzAcquireSpinlock(&IrqHandlers[irq].Spinlock);
    IrqHandlers[irq].Add(handler);
    PzReleaseSpinlock(&IrqHandlers[irq].Spinlock);
}

void PzUninstallIrqHandler(int irq, IrqHandlerFunc handler)
{
    PzAcquireSpinlock(&IrqHandlers[irq].Spinlock);
    IrqHandlers[irq].RemoveValue(handler);
    PzReleaseSpinlock(&IrqHandlers[irq].Spinlock);
}

void PzAssert(bool condition, const char *error, ...)
{

}

static int PzInterruptController = 0;

void PzEnterCriticalRegion()
{
    PzDisableInterrupts();
}

void PzLeaveCriticalRegion()
{
    PzEnableInterrupts();
}

void PzSetInterruptController(int controller)
{
    PzInterruptController = controller;
}

int PzGetInterruptController()
{
    return PzInterruptController;
}

#include <processor.hh>

void PzPushAddressSpace(uptr addr_space_desc)
{
    if (PzGetCurrentIrql() < DISPATCH_LEVEL)
        PzRaiseIrql(DISPATCH_LEVEL);

    PzGetCurrentProcessor()->AddressSpaceStack.Add(HalReadCr3());
    HalSwitchPageTable(addr_space_desc);
}

void PzPopAddressSpace()
{
    auto *stack = &PzGetCurrentProcessor()->AddressSpaceStack;
    uptr old = stack->Last->Value;
    stack->Remove(stack->Last);

    HalSwitchPageTable(old);

    if (PzGetCurrentIrql() == DISPATCH_LEVEL)
        PzLowerIrql(PASSIVE_LEVEL);
}