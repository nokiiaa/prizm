#pragma once

#include <defs.hh>
#include <boot.hh>

/* Function to initialize the physical page allocator's state. */
void MmPhysicalInitializeState(KernelBootInfo *info);

void MmiPhysicalPostVirtualInit();

/* Function to allocate a single physical page of a certain order. */
PZ_KERNEL_EXPORT uptr MmPhysicalAllocatePage(int order);

/* Function to allocate to allocate several physically contiguous pages of a certain order. */
PZ_KERNEL_EXPORT uptr MmPhysicalAllocateContiguousPages(u32 order, u32 count);

/* Function to mark at least one page of a certain order as free, given the physical address of the first page. */
PZ_KERNEL_EXPORT bool MmPhysicalFreePages(uptr address, u32 order, u32 number);