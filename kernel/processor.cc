#include <processor.hh>
#include <panic.hh>
#include <x86/i8259a.hh>
#include <debug.hh>
#include <x86/idt.hh>

PzProcessor Placeholder = { 0 };

PzProcessor *PzGetCurrentProcessor()
{
    return &Placeholder;
}

int PzGetCurrentIrql()
{
    return PzGetCurrentProcessor()->IntLevel;
}

int PzRaiseIrql(int new_irql)
{
    PzProcessor *processor = PzGetCurrentProcessor();
    int irql = processor->IntLevel;

    if (new_irql < irql)
        PzPanic(nullptr, PANIC_REASON_INVALID_IRQL, "PzRaiseIrql has been called with new_irql < old_irql");

    processor->IntLevel = new_irql;
    return irql;
}

int PzLowerIrql(int new_irql)
{
    PzProcessor *processor = PzGetCurrentProcessor();
    int irql = processor->IntLevel;

    if (new_irql > irql)
        PzPanic(nullptr, PANIC_REASON_INVALID_IRQL, "PzLowerIrql has been called with new_irql > old_irql");

    processor->IntLevel = new_irql;

    if (HalIsIdtInitialized) {
#ifdef __GNUC__
        asm("int $0x30");
#else
        #error TODO: msvc inline assembly for this function
#endif
    }

    return irql;
}