#include <syscall/mem.hh>
#include <mm/virtual.hh>
#include <lib/util.hh>
#include <sched/scheduler.hh>
#include <serial.hh>

PzStatus UmAllocateVirtualMemory(CpuInterruptState *state, void *params)
{
    auto prm = (UmAllocateVirtualMemoryParams*)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmAllocateVirtualMemoryParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->BaseAddress, sizeof(void **), true))
        return STATUS_INVALID_ARGUMENT;

    if (void *allocated =
        MmVirtualAllocateUserMemory(prm->ProcessHandle, *prm->BaseAddress, prm->Size, prm->Protection)) {
        *prm->BaseAddress = allocated;
        return STATUS_SUCCESS;
    }

    return STATUS_FAILED;
}

PzStatus UmProtectVirtualMemory(CpuInterruptState *state, void *params)
{
    auto prm = (UmAllocateVirtualMemoryParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmAllocateVirtualMemoryParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->BaseAddress, sizeof(void **), true))
        return STATUS_INVALID_ARGUMENT;

    if (bool allocated =
        MmVirtualProtectUserMemory(prm->ProcessHandle, *prm->BaseAddress, prm->Size, prm->Protection)) {
        *(uptr*)*prm->BaseAddress &= -PAGE_SIZE;
        return STATUS_SUCCESS;
    }

    return STATUS_FAILED;
}

PzStatus UmFreeVirtualMemory(CpuInterruptState *state, void *params)
{
    auto prm = (UmFreeVirtualMemoryParams*)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmFreeVirtualMemoryParams), false))
        return STATUS_INVALID_ARGUMENT;

    if (MmVirtualFreeUserMemory(prm->ProcessHandle, prm->BaseAddress, 0))
        return STATUS_SUCCESS;

    return STATUS_FAILED;
}