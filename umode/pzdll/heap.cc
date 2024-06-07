#include <pzapi.h>

PzHeap DefaultHeap = { 0, 0, 0 };

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

PzHeap *PzGetDefaultHeap()
{
    if (!DefaultHeap.Mutex) {
        PzCreateMutex(&DefaultHeap.Mutex, nullptr);
        PzInitializeHeap(&DefaultHeap, 1024 * 1024);
    }
    return &DefaultHeap;
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

u8 SHUTUP[1024 * 1024 * 2];

PZDLL_EXPORT PzHeapBlockHeader *PzCreateHeapBlock(PzHeap *heap, int size, int granularity)
{
    if (!heap)
        heap = PzGetDefaultHeap();

    PzWaitForObject(heap->Mutex);
    int bmp_size;
    bmp_size = RoundUp(size, granularity) / granularity;
    bmp_size = RoundUp(bmp_size, 64);
    int block_space = sizeof(PzHeapBlockHeader) + bmp_size + size;
    PzHeapBlockHeader *start = nullptr;

    //printf("allocating :flushed:\r\n");

    //start = (PzHeapBlockHeader*)SHUTUP;
    if (PzAllocateVirtualMemory(CPROC_HANDLE, (void **)&start, block_space, PAGE_READWRITE)
        != STATUS_SUCCESS) {
        PzReleaseMutex(heap->Mutex);
        return nullptr;
    }

    //printf("allocated :flushed:\r\n");

    start->Previous = heap->LastBlock;
    start->Next = nullptr;
    start->Granularity = granularity;
    start->LastIndex = 0;
    start->Used = 0;
    start->Size = size;
    start->BitmapSize = bmp_size;

    MemSet(start->BitmapStart, 0, start->BitmapSize);

    auto *block = heap->LastBlock = heap->LastBlock ? (heap->LastBlock->Next = start) : start;

    PzReleaseMutex(heap->Mutex);
    return block;
}

PZDLL_EXPORT void PzUnlinkHeapBlock(PzHeap *heap, PzHeapBlockHeader *header)
{
    if (!heap)
        heap = PzGetDefaultHeap();
    
    PzWaitForObject(heap->Mutex);

    if (header == heap->FirstBlock)
        heap->FirstBlock = header->Next;

    if (header->Previous)
        header->Previous->Next = header->Next;

    if (header->Next)
        header->Next->Previous = header->Previous;

    u32 size = header->Size + header->BitmapSize + sizeof * header;
    //PzFreeVirtualMemory(CPROC_HANDLE, header, &size);
    PzReleaseMutex(heap->Mutex);
}

PZDLL_EXPORT void PzInitializeHeap(PzHeap *heap, int init_size)
{
    heap->LastBlock = nullptr;
    heap->FirstBlock = PzCreateHeapBlock(heap, init_size, PL_DEFAULT_GRANULARITY);
}

PZDLL_EXPORT void *PzAllocateHeapMemory(PzHeap *heap, int bytes, u32 flags)
{
    if (!heap)
        heap = PzGetDefaultHeap();

    if (bytes == 0)
        return nullptr;

    #define MAKE_BLOCK \
        do { PzReleaseMutex(heap->Mutex); \
        PzCreateHeapBlock(heap, bytes, PL_DEFAULT_GRANULARITY); \
        PzWaitForObject(heap->Mutex); } while (0)

    PzWaitForObject(heap->Mutex);

    if (!heap->FirstBlock)
        MAKE_BLOCK;

    for (PzHeapBlockHeader *header = heap->FirstBlock;
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
                PzReleaseMutex(heap->Mutex);
                return address;
            }
        }

        if (!header->Next)
            MAKE_BLOCK;
    }
    #undef MAKE_BLOCK

    PzReleaseMutex(heap->Mutex);
    return nullptr;
}

PZDLL_EXPORT void *PzReAllocateHeapMemory(PzHeap *heap, void *ptr, int bytes)
{
    if (!heap)
        heap = PzGetDefaultHeap();

    int old_size = PzFreeHeapMemory(heap, ptr);

    if (old_size == -1)
        return nullptr;

    void *new_location = PzAllocateHeapMemory(heap, bytes, 0);

    if (!new_location)
        return nullptr;

    MemCopy(new_location, ptr, bytes > old_size ? old_size : bytes);
    return new_location;
}

PZDLL_EXPORT int PzFreeHeapMemory(PzHeap *heap, void *ptr)
{
    if (!heap)
        heap = PzGetDefaultHeap();

    PzWaitForObject(heap->Mutex);

    for (PzHeapBlockHeader *header = heap->FirstBlock;
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

            /*if (!header->Used) {
                PzReleaseMutex(heap->Mutex);
                PzUnlinkHeapBlock(heap, header);
                PzWaitForObject(heap->Mutex);
            }*/

            PzReleaseMutex(heap->Mutex);
            return bytes;
        }
    }
fail:
    PzReleaseMutex(heap->Mutex);
    return -1;
}