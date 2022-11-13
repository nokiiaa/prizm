#pragma once

#include <defs.hh>
#include <core.hh>

#define FLAG_RESPONSE 0x80000000
#define FLAG_ERROR    0x40000000
#define CONNECT_SEQUENCE 0x12345678
#define COMMAND_OBJECT_LIST      1
#define COMMAND_READ_VMEM        2
#define COMMAND_WRITE_VMEM       3
#define COMMAND_OBJ_ROOT         4
#define COMMAND_READ_LINKED_LIST 5
#define COMMAND_READ_STRING      6
#define COMMAND_BREAK            7
#define COMMAND_CONTINUE         8
#define EVENT_LOG_STRING 1
#define EVENT_CREATE_OBJECT 2
#define EVENT_KERNEL_PANIC 3
#define EVENT_BREAKPOINT 4
#define ERROR_ACCESS_VIOLATION (FLAG_ERROR | 1)
#define ERROR_COMMAND_ABORTED  (FLAG_ERROR | 2)

extern bool DbgSchedulerEnabled;

void DbgWriteChar(char data);
void DbgWrite8(u8 data);
void DbgWrite16(u16 data);
void DbgWrite32(u32 data);
void DbgWritePtr(uptr data);
char DbgReadChar();
u8 DbgRead8();
u16 DbgRead16();
u32 DbgRead32();
uptr DbgReadPtr();
PzHandle DbgStartListener();
int DbgListener(void *param);
void DbgEnterBreakpointMode(CpuInterruptState *state);
void DbgWriteStackTraceAndRegs(CpuInterruptState *state);
PZ_KERNEL_EXPORT void DbgPrintStrUnformatted(const char *str, int size);
PZ_KERNEL_EXPORT void DbgPrintStr(const char *str, ...);