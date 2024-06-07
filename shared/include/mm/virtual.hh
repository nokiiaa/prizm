#pragma once

#include <defs.hh>
#include <boot.hh>

#define PAGE_READ 1
#define PAGE_WRITE 2
#define PAGE_EXECUTE 4
#define PAGE_READWRITE (PAGE_READ | PAGE_WRITE)
#define PAGE_EXECUTE_READWRITE (PAGE_READWRITE | PAGE_EXECUTE)

#define KERNEL_SPACE_START 0x8000'0000u
#define KERNEL_SPACE_SIZE  0x8000'0000u

struct PzProcessObject;

extern "C" void HalSwitchPageTable(uptr dir_pointer);
extern "C" uptr HalReadCr3();
extern "C" void HalFlushCacheForPage(void *page);
PZ_KERNEL_EXPORT void MmVirtualInitBootPageTable(KernelBootInfo *info);
PZ_KERNEL_EXPORT void *MmVirtualMapPhysical(void *start, uptr physical_addr, u32 bytes, u32 flags);
PZ_KERNEL_EXPORT bool MmVirtualProtectMemory(void *start, u32 bytes, u32 flags);
PZ_KERNEL_EXPORT void *MmVirtualAllocateMemory(
    void *start, u32 bytes, u32 flags, uptr *last_page_physical);
PZ_KERNEL_EXPORT bool MmVirtualFreeMemory(void *start, u32 bytes);
uptr MmiVirtualToPhysical(void *page, PzProcessObject *process);
PZ_KERNEL_EXPORT uptr MmVirtualToPhysical(void *page, PzHandle process);
uptr **MmiAllocateProcessPageDirectory(PzProcessObject *process);
void MmiFreeProcessPageDirectory(PzProcessObject *process);
void *MmiVirtualAllocateUserMemory(
    PzProcessObject *process, void *start, u32 bytes, u32 flags);
bool MmiVirtualFreeUserMemory(
    PzProcessObject *process, void *start, u32 flags);
bool MmiVirtualProtectUserMemory(
    PzProcessObject *process, void *start, u32 bytes, u32 flags);
PZ_KERNEL_EXPORT void *MmAllocateDmaMemory(u32 bytes, u32 alignment, uptr *physical, u32 flags);
PZ_KERNEL_EXPORT bool MmFreeDmaMemory(void *addr, u32 alignment, u32 bytes);
PZ_KERNEL_EXPORT void *MmVirtualAllocateUserMemory(PzHandle process, void *start, u32 bytes, u32 flags);
PZ_KERNEL_EXPORT bool MmVirtualFreeUserMemory(PzHandle process, void *start, u32 flags);
PZ_KERNEL_EXPORT bool MmVirtualProtectUserMemory(PzHandle process, void *start, u32 bytes, u32 flags);
PZ_KERNEL_EXPORT bool MmVirtualProbeMemory(bool as_user, uptr start, usize size, bool write);
PZ_KERNEL_EXPORT bool MmVirtualProbeKernelMemory(uptr start, usize size, bool write);