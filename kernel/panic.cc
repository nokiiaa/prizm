#include <panic.hh>
#include <serial.hh>
#include <stdarg.h>
#include <lib/strformat.hh>
#include <lib/list.hh>
#include <lib/string.hh>
#include <obj/process.hh>
#include <debug.hh>
#include <sched/scheduler.hh>

void PzPanic(CpuInterruptState *state, u32 reason, const char *error, ...)
{
    PzDisableInterrupts();
    DbgWrite32(EVENT_KERNEL_PANIC);
    DbgWrite32(reason);
    DbgWrite32(!!state);

    if (state)
        DbgWriteStackTraceAndRegs(state);

    /* Print message string */
    va_list params;
    va_start(params, error);
    PrintfWithCallbackV(DbgWriteChar, error, params);
    va_end(params);

    DbgWriteChar(0);

    /* Restart debug listener as post-panic */
    DbgListener((void*)state);
}