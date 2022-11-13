#pragma once

#include <defs.hh>
#include <spinlock.hh>
#include <mm/virtual.hh>

#define PL_ALLOC_FLAGS_ZERO 1
#define PL_DEFAULT_GRANULARITY 64

struct PzPoolBlockHeader
{
    PzPoolBlockHeader *Previous, *Next;
    u32 Granularity;
    u32 LastIndex;
    u32 Used, Size;
    u32 BitmapSize;
    alignas(64) u8 BitmapStart[0];
};

struct PzKernelPool
{
    PzSpinlock Lock;
    PzPoolBlockHeader *FirstBlock, *LastBlock;
};

void PlInitializeKernelPool(PzKernelPool *pool, int init_size);
PzPoolBlockHeader *PlCreateBlock(PzKernelPool *pool, int size, int granularity);
void PlUnlinkBlock(PzKernelPool *pool, PzPoolBlockHeader *header);
void *PlAllocateMemory(PzKernelPool *pool, int bytes, u32 flags);
void *PlReAllocateMemory(PzKernelPool *pool, void *ptr, int bytes);
int PlFreeMemory(PzKernelPool *pool, void *ptr);