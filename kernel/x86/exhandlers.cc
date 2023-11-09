#include <core.hh>
#include <mm/virtual.hh>
#include <debug.hh>
#include <panic.hh>
#include <processor.hh>
#include <sched/scheduler.hh>

extern "C" void HandleNmi(void *state)
{

}

extern "C" u32 HalReadCr2();

extern "C" void CpuExceptionHandler(CpuInterruptState * state)
{
    bool usermode = state->Cs != 0x8;

    if (usermode)
        ExHandleUserCpuException(state);
    else {
        const char *message;
        switch (state->InterruptNumber) {
            /* Divide-by-zero (#DE) */
        case 0:
            message = "Divide-by-zero error. System halted.";
            break;

            /* Debug (#DB) */
        case 1:
            message = "Debug.";
            break;

            /* NMI */
        case 2:
            HandleNmi(state);
            return;

            /* Breakpoint (#BP) */
        case 3:
            DbgEnterBreakpointMode(state);

            /* Fix instruction pointer to point to the instruction
               that replaced the breakpoint */
            if (MmVirtualProbeMemory(false, state->Eip - sizeof(u16), 2, false) &&
                ((u16 *)state->Eip)[-1] == 0x03CD)
                /* Breakpoint was caused by CD 03 (int 0x3) */
                state->Eip -= 2;
            else
                /* Breakpoint was caused by CC (int3) */
                state->Eip--;

            return;

            /* Overflow (#OF) */
        case 4:
            message = "Array out of bounds. System halted.";
            break;

        case 5:
            return;

            /* Invalid opcode (#UD) */
        case 6:
            message = "Invalid opcode. System halted.";
            break;

            /* Device not available (#NM) */
        case 7:
            message = "FPU not available. System halted.";
            break;

            /* Double fault (#DF) */
        case 8:
            message = "Double fault. System halted.";
            break;

            /* Coprocessor segment overrun (?) */
        case 9:
            return;

            /* Invalid TSS (#TS) */
        case 10:
            message = "Invalid TSS. System halted.";
            break;

            /* Segment not present (#NP) */
        case 11:
            message = "Segment not present. System halted.";
            break;

            /* Stack-segment fault (#SS) */
        case 12:
            message = "Stack-segment fault. System halted.";
            break;

            /* General Protection Fault (#GP) */
        case 13:
            message = "General protection fault. System halted.";
            break;

            /* Page fault (#PF) */
        case 14:
            message = "Page fault. System halted.";
            break;

            /* x86 floating-point exception (#MF) */
        case 16:
            message = "x87 exception. System halted.";
            break;

            /* Alignment check (#AC) */
        case 17:
            message = "Alignment check exception. System halted.";
            break;

            /* Machine check (#MC) */
        case 18:
            message = "Machine check exception. System halted.";
            break;

            /* SIMD floating-point exception (#XM/#XF) */
        case 19:
            message = "SIMD floating-point exception. System halted.";
            break;

            /* Virtualization exception (#VE). Surely this will happen someday. */
        case 20:
            message = "Virtualization exception. System halted.";
            break;

            /* Security exception (#SX) */
        case 30:
            message = "Security exception. System halted.";
            break;

        default:
            return;
        }

        PzPanic(state, PANIC_REASON_CPU_EXCEPTION, "%s\r\n"
            "eax=0x%p ecx=0x%p edx=0x%p ebx=0x%p\r\n"
            "esp=0x%p ebp=0x%p esi=0x%p edi=0x%p\r\n"
            "eip=0x%p err=0x%p efl=0x%p cr2=0x%p\r\n"
            "cs=0x%04x ds=0x%04x es=0x%04x fs=0x%04x gs=0x%04x", message,
            state->Eax, state->Ecx, state->Edx, state->Ebx,
            state->Esp, state->Ebp, state->Esi, state->Edi,
            state->Eip, state->ErrorCode, state->Eflags,
            HalReadCr2(),
            state->Cs & 0xFFFF, state->Ds & 0xFFFF, state->Es & 0xFFFF,
            state->Fs & 0xFFFF, state->Gs & 0xFFFF);
    }
}