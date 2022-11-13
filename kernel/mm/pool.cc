#include <mm/pool.hh>
#include <lib/util.hh>
#include <debug.hh>

inline int RoundUp(int size, int granularity)
{
    return (size + granularity - 1) & -granularity;
}

/* Find an empty space of the specified size in the specified bitmap */
int SearchCave(void *bmp, int bmp_size, int granularity, int needed)
{
    u8 *bitmap = (u8 *)bmp;
    int streak = 0, streak_start = 0;

    for (int i = 0; i < bmp_size; i++) {
        if (bitmap[i] == 0) {
            if (streak == 0)
                streak_start = i;

            streak++;

            if (streak == needed)
                return streak_start;
        }
        else
            streak = 0;
    }

    return -1;
}

void FillEntries(void *bmp, int size, int value)
{
    MemSet(bmp, value, size);
}

/* Replace consecutive values in bitmap starting at provided index with the specified value */
int ReplaceEntries(void *bmp, int index, int value)
{
    char *ch = (char *)bmp + index;
    char first = ch[0];

    if (first == 0)
        return 0;

    int entries = 0;

    while (*ch == first) {
        *ch = value;
        entries++;
    }

    return entries;
}

PzPoolBlockHeader *PlCreateBlock(PzKernelPool *pool, int size, int granularity)
{
    PzAcquireSpinlock(&pool->Lock);

    int bmp_size;
    bmp_size = RoundUp(size, granularity) / granularity;
    bmp_size = RoundUp(bmp_size, 64);

    int block_space = sizeof(PzPoolBlockHeader) + bmp_size + size;

    auto *start = (PzPoolBlockHeader *)MmVirtualAllocateMemory(
        nullptr, block_space, PAGE_READWRITE, nullptr);

    if (!start)
        return nullptr;

    start->Previous = pool->LastBlock;
    start->Next = nullptr;
    start->Granularity = granularity;
    start->LastIndex = 0;
    start->Used = 0;
    start->Size = size;
    start->BitmapSize = bmp_size;

    MemSet(start->BitmapStart, 0, start->BitmapSize);

    auto *block = pool->LastBlock = pool->LastBlock ? (pool->LastBlock->Next = start) : start;

    PzReleaseSpinlock(&pool->Lock);
    return block;
}

void PlUnlinkBlock(PzKernelPool *pool, PzPoolBlockHeader *header)
{
    PzAcquireSpinlock(&pool->Lock);

    if (header == pool->FirstBlock)
        pool->FirstBlock = header->Next;

    if (header->Previous)
        header->Previous->Next = header->Next;

    if (header->Next)
        header->Next->Previous = header->Previous;

    MmVirtualFreeMemory(header, header->Size + header->BitmapSize + sizeof *header);
    PzReleaseSpinlock(&pool->Lock);
}

void PlInitializeKernelPool(PzKernelPool *pool, int init_size)
{
    pool->LastBlock = nullptr;
    pool->FirstBlock = PlCreateBlock(pool, init_size, PL_DEFAULT_GRANULARITY);
}

void *PlAllocateMemory(PzKernelPool *pool, int bytes, u32 flags)
{
    if (bytes == 0)
        return nullptr;

    #define MAKE_BLOCK \
        do { PzReleaseSpinlock(&pool->Lock); \
        if (!PlCreateBlock(pool, bytes, PL_DEFAULT_GRANULARITY)) return nullptr; \
        PzAcquireSpinlock(&pool->Lock); } while (0)

    PzAcquireSpinlock(&pool->Lock);

    if (!pool->FirstBlock)
        MAKE_BLOCK;

    for (PzPoolBlockHeader *header = pool->FirstBlock;
        header; header = header->Next) {
        /* Is there enough space in this block for the allocation?
           (this is just an initial check which obviously can't
           detect whether that space is contiguous) */
        if (header->Used + bytes <= header->Size) {
            int gran = header->Granularity;
            int needed = RoundUp(bytes, gran) / gran;
            int bmp_size = header->BitmapSize;
            int index = SearchCave(header->BitmapStart, bmp_size, gran, needed);

            if (index != -1) {
                int  left_id = index == 0 ? 1 : header->BitmapStart[index - 1];
                int right_id = index + needed >= bmp_size ? 1 : header->BitmapStart[index + needed];
                int  this_id = 1;

                while (this_id == left_id || this_id == right_id)
                    this_id++;

                FillEntries(header->BitmapStart + index, needed, this_id);

                header->Used += ALIGN(bytes, gran);
                header->LastIndex = index + needed;

                void *address = (void *)(header->BitmapStart + bmp_size + index * gran);

                PzReleaseSpinlock(&pool->Lock);
                return address;
            }
        }

        if (!header->Next)
            MAKE_BLOCK;
    }

    #undef MAKE_BLOCK
    PzReleaseSpinlock(&pool->Lock);
    return nullptr;
}

void *PlReAllocateMemory(PzKernelPool *pool, void *ptr, int bytes)
{
    int old_size = PlFreeMemory(pool, ptr);

    if (old_size == -1)
        return nullptr;

    void *new_location = PlAllocateMemory(pool, bytes, 0);

    if (!new_location)
        return nullptr;

    MemCopy(new_location, ptr, bytes > old_size ? old_size : bytes);
    return new_location;
}

int PlFreeMemory(PzKernelPool *pool, void *ptr)
{
    PzAcquireSpinlock(&pool->Lock);

    for (PzPoolBlockHeader *header = pool->FirstBlock;
        header; header = header->Next) {
        u8 *data_start = header->BitmapStart + header->BitmapSize;
        u32 gran = header->Granularity;

        if (ptr >= data_start && ptr <= data_start + header->Size) {
            int index = ((u8 *)ptr - data_start) / gran;

            if (index >= 0 && header->BitmapStart[index] == header->BitmapStart[index - 1]) {
                /* The bitmap region of consecutive values does not start at the location that was passed.
                   Therefore, this must be invalid. Return -1 to prevent heap corruption. */
                goto fail;
            }

            int bytes =
                ReplaceEntries(header->BitmapStart, index, 0) * gran;

            header->Used -= bytes;
            header->LastIndex = index;

            if (!header->Used) {
                PzReleaseSpinlock(&pool->Lock);
                PlUnlinkBlock(pool, header);
                PzAcquireSpinlock(&pool->Lock);
            }

            PzReleaseSpinlock(&pool->Lock);
            return bytes;
        }
    }

fail:
    PzReleaseSpinlock(&pool->Lock);
    return -1;
}