#pragma once

#include <defs.hh>
#include <x86/e820.hh>

struct KernelBootInfo
{
    uptr KernelVirtualBase, KernelSize;
    uptr KernelPhysicalStart, KernelPhysicalEnd;
    E820MemRegion *MemMapPointer;
    int BootstrapDriverCount;
    void *BootstrapDriverBases[0];
};