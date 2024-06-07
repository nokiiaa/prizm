#include <except/except.hh>
#include <sched/scheduler.hh>
#include <obj/process.hh>
#include <processor.hh>
#include <debug.hh>
#include <core.hh>

void ExCallNextHandler(CpuInterruptState *user_state)
{
    auto &info = PsGetCurrentThread()->CurrentException;

    if (PzExceptionRegistration *handler = PsGetCurrentThread()->CurrentExHandler) {
        *(PzExceptionInfo *)(user_state->EspU -= sizeof(PzExceptionInfo)) = info;
        user_state->Eip = (u32)handler->Handler;
    }
    else
        PsTerminateProcess(CPROC_HANDLE, info.Code);
}

void ExContinueExecution()
{
    PzGetCurrentProcessor()->Queue.FsSpace =
        PsGetCurrentThread()->CurrentException.SavedContext;
    PzDisableInterrupts();
    HalSwitchContextUser();
}

void ExPushExceptionHandler(PzExceptionHandler handler, uptr stack_context)
{
    auto &current = PsGetCurrentThread()->CurrentExHandler;
    auto *reg = new PzExceptionRegistration;
    reg->Handler = handler;
    reg->StackContext = stack_context;
    reg->Previous = current;
    current = reg;
}

void ExPopExceptionHandler()
{
    auto &handler = PsGetCurrentThread()->CurrentExHandler;

    if (handler)
        handler = handler->Previous;
}

extern "C" u32 HalReadCr2();

void ExHandleUserCpuException(CpuInterruptState *state)
{
    u32 code = EXCEPTION_OTHER;
    switch (state->InterruptNumber) {
    case 0:  code = EXCEPTION_DIVIDE_BY_ZERO; break;
    case 6:  code = EXCEPTION_ILLEGAL_INSTRUCTION; break;
    case 13: code = EXCEPTION_PRIV_INSTRUCTION; break;
    case 14: code = EXCEPTION_ACCESS_VIOLATION; break;
    default: return;
    }
    auto *reg = PsGetCurrentThread()->CurrentExHandler;
    if (/* If there are no available handlers, just terminate the process. */
        !reg ||
        /* Or if we can't even write the structure to the stack
           for some reason, I guess we shouldn't bother either? */
        !MmVirtualProbeMemory(true,
            state->EspU - sizeof(PzExceptionInfo),
            sizeof(PzExceptionInfo), true)) {

        DbgPrintStr("\r\nAn unhandled exception has occurred. Terminating process:\r\n"
            "eax=0x%p ecx=0x%p edx=0x%p ebx=0x%p\r\n"
            "esp=0x%p ebp=0x%p esi=0x%p edi=0x%p\r\n"
            "eip=0x%p err=0x%p efl=0x%p cr2=0x%p\r\n"
            "cs=0x%04x ds=0x%04x es=0x%04x fs=0x%04x gs=0x%04x\r\n"
            "code=%i",
            state->Eax, state->Ecx, state->Edx, state->Ebx,
            state->Esp, state->Ebp, state->Esi, state->Edi,
            state->Eip, state->ErrorCode, state->Eflags,
            HalReadCr2(),
            state->Cs & 0xFFFF, state->Ds & 0xFFFF, state->Es & 0xFFFF,
            state->Fs & 0xFFFF, state->Gs & 0xFFFF, code);

        PsTerminateProcess(CPROC_HANDLE, code);
    }

    /* Save thread context */
    PzThreadContext context;
    context.Eax = state->Eax;
    context.Ecx = state->Ecx;
    context.Edx = state->Edx;
    context.Ebx = state->Ebx;
    context.Ebp = state->Ebp;
    context.Esi = state->Esi;
    context.Edi = state->Edi;
    context.Eip = state->Eip;
    context.Eflags = state->Eflags | 1 << 9 | 1 << 1;
    context.Cs = state->Cs;
    context.Ds = state->Ds;
    context.Es = state->Es;
    context.Fs = state->Fs;
    context.Gs = state->Gs;
    context.Esp = state->EspU;
    context.Ss = state->SsU;

    /* Push exception info structure on stack */
    *(PzExceptionInfo *)(state->EspU -= sizeof(PzExceptionInfo)) =
        PsGetCurrentThread()->CurrentException = {
        code,
        0,
        state->Eip,
        0,
        {},
        context,
        reg->StackContext
    };

    /* Push dummy return address */
    *(u32*)(state->EspU -= sizeof(u32)) = 0;

    /* Transfer control to handler */
    state->Eip = (u32)reg->Handler;
}