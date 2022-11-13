#include <debug.hh>
#include <serial.hh>
#include <stdarg.h>
#include <core.hh>
#include <lib/strformat.hh>
#include <sched/scheduler.hh>
#include <obj/manager.hh>
#include <mm/virtual.hh>
#include <obj/directory.hh>

static PzSpinlock DbgSpinlockRead, DbgSpinlockWrite;

char DbgReadChar()
{
    return SerialReadChar();
}

void DbgWriteChar(char data)
{
    SerialPrintChar(data);
}

void DbgWrite8(u8 data)
{
    DbgWriteChar(data);
}

void DbgWrite16(u16 data)
{
    DbgWriteChar(data & 0xFF);
    DbgWriteChar(data >> 8);
}

void DbgWrite32(u32 data)
{
    DbgWriteChar(data & 0xFF);
    DbgWriteChar((u8)(data >> 8));
    DbgWriteChar((u8)(data >> 16));
    DbgWriteChar((u8)(data >> 24));
}

void DbgWritePtr(uptr data)
{
    DbgWrite32(data);
}

u8 DbgRead8()
{
    return (u8)DbgReadChar();
}

u16 DbgRead16()
{
    u16 ret = 0;
    ret |= (u16)DbgRead8() << 0;
    ret |= (u16)DbgRead8() << 8;
    return ret;
}

u32 DbgRead32()
{
    u32 ret = 0;
    ret |= (u32)DbgRead8() << 0;
    ret |= (u32)DbgRead8() << 8;
    ret |= (u32)DbgRead8() << 16;
    ret |= (u32)DbgRead8() << 24;
    return ret;
}

uptr DbgReadPtr()
{
    return DbgRead32();
}

void DbgEnumerateObjects(PzDirectoryObject *dir)
{
    ENUM_LIST(ob, dir->Objects) {
        DbgWritePtr((uptr)ob->Value - sizeof(ObObjectHeader));

        if (OBJECT_HEADER(ob->Value)->Type == PZ_OBJECT_DIRECTORY)
            DbgEnumerateObjects((PzDirectoryObject *)ob->Value);
    }
}

void DbgEnterBreakpointMode(CpuInterruptState *state)
{
    PzDisableInterrupts();
    DbgWrite32(EVENT_BREAKPOINT);
    DbgWriteStackTraceAndRegs(state);

    /* Restart debug listener */
    DbgListener((void *)state);
}

void DbgWriteStackTraceAndRegs(CpuInterruptState *state)
{
    /* IA-32 stack trace dumper */
    PzThreadObject *thread = PsGetCurrentThread();
    uptr stack_top = -1;
    uptr stack_bottom = 0;

    if (thread) {
        uptr user_stack_start = (uptr)thread->UserStack;
        uptr user_stack_end = user_stack_start + thread->UserStackSize;
        uptr kernel_stack_start = (uptr)thread->KernelStack;
        uptr kernel_stack_end = kernel_stack_start + thread->KernelStackSize;

        stack_top =
            (user_stack_start <= state->Ebp && state->Ebp < user_stack_end) ? user_stack_end :
            kernel_stack_end;

        stack_bottom = stack_top == kernel_stack_end ? kernel_stack_start : user_stack_start;
    }

    DbgWritePtr(state->Eip);

    for (uptr ebp = state->Ebp;
        stack_bottom < ebp && ebp < stack_top &&
        MmVirtualProbeMemory(false, ebp, sizeof(uptr) * 2, false);
        ebp = *(uptr *)ebp) {

        /* Return address is *(ebp + 4) */
        DbgWritePtr(((uptr *)ebp)[1]);
    }

    /* Terminate stack trace */
    DbgWritePtr(-1);

    /* Output saved CPU state */
    for (int i = 0; i < sizeof(CpuInterruptState); i++)
        DbgWrite8(((u8 *)state)[i]);
}

int DbgListener(void *param)
{
    for (CpuInterruptState *state = (CpuInterruptState *)param ;;) {
        u32 command = DbgRead32();

        switch (command) {
            case COMMAND_OBJECT_LIST: {
                PzAcquireSpinlock(&DbgSpinlockWrite);

                DbgWrite32(COMMAND_OBJECT_LIST | FLAG_RESPONSE);
                /* Enumerate all system objects, starting from the root directory */
                DbgEnumerateObjects(ObRootDirectory);
                DbgWrite32(0);

                PzReleaseSpinlock(&DbgSpinlockWrite);
                break;
            }

            case COMMAND_READ_VMEM: {
                PzAcquireSpinlock(&DbgSpinlockWrite);

                u32 start = DbgRead32();
                u32 size = DbgRead32();

                /* Check if provided address range is valid */
                if (!MmVirtualProbeKernelMemory(start, size, false)) {
                    DbgWrite32(ERROR_ACCESS_VIOLATION);
                    PzReleaseSpinlock(&DbgSpinlockWrite);
                    break;
                }

                DbgWrite32(COMMAND_READ_VMEM | FLAG_RESPONSE);

                for (u32 i = 0; i < size; i++)
                    DbgWrite8(((u8*)start)[i]);

                PzReleaseSpinlock(&DbgSpinlockWrite);
                break;
            }

            case COMMAND_WRITE_VMEM: {
                PzAcquireSpinlock(&DbgSpinlockWrite);

                u32 start = DbgRead32();
                u32 size = DbgRead32();

                /* Check if provided address range is valid */
                if (!MmVirtualProbeKernelMemory(start, size, true)) {
                    DbgWrite32(ERROR_ACCESS_VIOLATION); 

                    for (int i = 0; i < size; i++) DbgRead8();

                    PzReleaseSpinlock(&DbgSpinlockWrite);
                    break;
                }

                DbgWrite32(COMMAND_WRITE_VMEM | FLAG_RESPONSE);

                for (u32 i = 0; i < size; i++)
                    ((u8 *)start)[i] = DbgRead8();

                PzReleaseSpinlock(&DbgSpinlockWrite);
                break;
            }

            case COMMAND_OBJ_ROOT: {
                PzAcquireSpinlock(&DbgSpinlockWrite);

                DbgWrite32(COMMAND_OBJ_ROOT | FLAG_RESPONSE);
                DbgWritePtr((uptr)ObRootDirectory);

                PzReleaseSpinlock(&DbgSpinlockWrite);
                break;
            }

            case COMMAND_READ_LINKED_LIST: {
                PzAcquireSpinlock(&DbgSpinlockWrite);

                uptr start = DbgReadPtr();
                u32 element_size = DbgRead32();
                DbgWrite32(COMMAND_READ_LINKED_LIST | FLAG_RESPONSE);
                auto *node = (LLNode<u8>*)start;
                int length = 0;

                for (auto *cnt = node; cnt; cnt = cnt->Next, length++);
                DbgWrite32(length);

                for (; node; node = node->Next)
                    for (int i = 0; i < element_size; i++)
                        DbgWrite8(node->ValueStart[i]);

                PzReleaseSpinlock(&DbgSpinlockWrite);
                break;
            }

            case COMMAND_READ_STRING: {
                PzAcquireSpinlock(&DbgSpinlockWrite);
                uptr start = DbgReadPtr();

                /* Check if provided address range is valid */
                if (!MmVirtualProbeKernelMemory(start, sizeof(PzString), false)) {
                    DbgWrite32(ERROR_ACCESS_VIOLATION);
                    PzReleaseSpinlock(&DbgSpinlockWrite);
                    break;
                }

                const PzString *str = (const PzString *)start;
                DbgWrite32(COMMAND_READ_STRING | FLAG_RESPONSE);
                DbgWrite32(str->Size + 1);

                for (u32 i = 0; i < str->Size + 1; i++)
                    DbgWriteChar(str->Buffer[i]);

                PzReleaseSpinlock(&DbgSpinlockWrite);
                break;
            }

            case COMMAND_BREAK:
                PzDisableInterrupts();
                DbgWrite32(COMMAND_BREAK | FLAG_RESPONSE);
                break;

            case COMMAND_CONTINUE:
                DbgWrite32(COMMAND_CONTINUE | FLAG_RESPONSE);
                if (state) return 0;
                else PzEnableInterrupts();
                break;
        }
    }
    return -1;
}

PzHandle DbgStartListener()
{
    PzHandle handle = INVALID_HANDLE_VALUE;
    PsCreateThread(&handle, false, 0, DbgListener, nullptr, 0, THREAD_PRIORITY_LOW);
    return handle;
}

extern "C" int HalReadIfFlag();

bool DbgSchedulerEnabled = false;

void DbgPrintStrUnformatted(const char *str, int size)
{
    PzAcquireSpinlock(&DbgSpinlockWrite);
    DbgWrite32(EVENT_LOG_STRING);

    //DbgWrite32(DbgSchedulerEnabled ? PsGetCurrentThread()->Id : 0);
    for (int i = 0; i < size; i++)
        DbgWriteChar(str[i]);

    DbgWriteChar('\0');
    PzReleaseSpinlock(&DbgSpinlockWrite);
}

void DbgPrintStr(const char *str, ...)
{
    PzAcquireSpinlock(&DbgSpinlockWrite);
    DbgWrite32(EVENT_LOG_STRING);
    //DbgWrite32(DbgSchedulerEnabled ? PsGetCurrentThread()->Id : 0);

    va_list params;
    va_start(params, str);
    PrintfWithCallbackV(DbgWriteChar, str, params);
    DbgWriteChar('\0');
    va_end(params);

    PzReleaseSpinlock(&DbgSpinlockWrite);
}