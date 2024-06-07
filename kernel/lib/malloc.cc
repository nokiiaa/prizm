#include <lib/malloc.hh>
#include <lib/util.hh>

PzKernelPool KernelPool;

void PzHeapInitialize()
{
    PlInitializeKernelPool(&KernelPool, 1024 * 1024 * 2);
}

void *PzHeapAllocate(u32 bytes, u32 flags)
{
    u8 *mem = (u8 *)PlAllocateMemory(&KernelPool, bytes, flags);

    if (mem && (flags & HEAP_ZERO))
        MemSet(mem, 0, bytes);

    return (void *)mem;
}

void *PzHeapReAllocate(void *mem, u32 bytes)
{
    return PlReAllocateMemory(&KernelPool, mem, bytes);
}

void PzHeapFree(void *mem)
{
    PlFreeMemory(&KernelPool, mem);
}

void *operator new(size_t size)
{
    return PzHeapAllocate(size, 0);
}

void *operator new(
    size_t size,
    void *ptr)
{
    return ptr;
}

void operator delete(void *ptr)
{
    PzHeapFree(ptr);
}

void *operator new[](size_t size)
{
    return PzHeapAllocate(size, 0);
}

void operator delete[](void *ptr)
{
    PzHeapFree(ptr);
}

void operator delete(
    void *ptr,
    size_t s)
{
    PzHeapFree(ptr);
}