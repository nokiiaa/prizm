#include <mm/pool.hh>

#define HEAP_ZERO 1

PZ_KERNEL_EXPORT void PzHeapInitialize();
PZ_KERNEL_EXPORT void *PzHeapAllocate(u32 bytes, u32 flags);
PZ_KERNEL_EXPORT void *PzHeapReAllocate(void *mem, u32 bytes);
PZ_KERNEL_EXPORT void PzHeapFree(void *mem);
PZ_KERNEL_EXPORT_CPP void *operator new(size_t size);
PZ_KERNEL_EXPORT_CPP void *operator new(size_t size, void *ptr);
PZ_KERNEL_EXPORT_CPP void operator delete(void *ptr);
PZ_KERNEL_EXPORT_CPP void *operator new[](size_t size);
PZ_KERNEL_EXPORT_CPP void operator delete[](void *ptr);
PZ_KERNEL_EXPORT_CPP void operator delete(void *ptr, size_t s);