#pragma once

#include <defs.hh>
#include <core.hh>
#include <syscall/handle.hh>

struct UmAllocateVirtualMemoryParams
{
    PzHandle ProcessHandle;
    void **BaseAddress;
    u32 Size;
    u32 Protection;
};

struct UmProtectVirtualMemoryParams
{
    PzHandle ProcessHandle;
    void **BaseAddress;
    u32 Size;
    u32 Protection;
};

struct UmFreeVirtualMemoryParams
{
    PzHandle ProcessHandle;
    void *BaseAddress;
    u32 *Size;
};

DECL_SYSCALL(UmAllocateVirtualMemory);
DECL_SYSCALL(UmProtectVirtualMemory);
DECL_SYSCALL(UmFreeVirtualMemory);