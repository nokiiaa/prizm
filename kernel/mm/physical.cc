#include <mm/physical.hh>
#include <mm/virtual.hh>
#include <x86/e820.hh>
#include <lib/util.hh>
#include <spinlock.hh>
#include <boot.hh>

#define MAX_ORDER 7
#define ORDERS ((MAX_ORDER) + 1)

struct AllocatorPhysRegion
{
    uptr DataStart, DataEnd;
    uptr BmpPhysicalStart;
    u32 BmpTotalSize;
    u32 BmpSizes[ORDERS], Search[ORDERS];
    u8 *Bitmaps[ORDERS];
} Allocator;

struct MemRegion
{
    uintptr_t Start, End;
};

struct MemRegionLinked
{
    uintptr_t Start, End;
    MemRegionLinked *Previous;
};

static PzSpinlock MmPhysicalLock;

/* Combines two memory ranges, assuming that b consists of a single member. */
MemRegionLinked *CombineRegions(
    MemRegionLinked *a,
    MemRegionLinked *b)
{
    for (MemRegionLinked *reg = a; reg; reg = reg->Previous) {
        /* Check for overlap */
        if (OVERLAPPING(reg->Start, reg->End, b->Start, b->End)) {
            *reg = MemRegionLinked {
                Min(reg->Start, b->Start),
                Max(reg->End, b->End),
                reg->Previous
            };

            return a;
        }
    }
    b->Previous = a;
    return b;
}

void EnumerateRegions(
    const E820MemRegion *region,
    MemRegionLinked *prev,
    MemRegion *regions,
    int &region_count,
    int max_regions)
{
    if (region->RegionType < E820_REGION_USABLE || region->RegionType > E820_REGION_BAD) {
        int count = 0;

        for (; max_regions > count; count++) {
            regions[count] = MemRegion{ prev->Start, prev->End };
            if (!(prev = prev->Previous))
                break;
        }

        region_count = count;

        /* Sort the regions with selection sort */
        for (int i = 0; i < count - 1; i++) {
            int j_min = i;
            for (int j = i + 1; j < count; j++) {
                if (regions[j].Start < regions[j_min].Start)
                    j_min = j;
            }
            if (j_min != i)
                Swap(regions[i], regions[j_min]);
        }

        return;
    }

    /* Ignore non-free regions */
    if (region->RegionType != E820_REGION_USABLE) {
        EnumerateRegions(
            region + 1, prev, regions,
            region_count, max_regions);
        return;
    }

    MemRegionLinked next_reg = { region->BaseLo, region->BaseLo + region->LengthLo, prev };
    EnumerateRegions(
        region + 1, CombineRegions(prev, &next_reg),
        regions, region_count, max_regions);
}

/* Utility routine to read a single bit from a bitmap. */
inline bool BmpReadBit(void *bmp, int index)
{
    return ((u32 *)bmp)[index / 32] & 1 << index % 32;
}

/* Utility routine to write a single bit to a bitmap. */
template<bool set> inline void BmpWriteBit(void *bmp, int index)
{
    if (set)
        ((u32 *)bmp)[index / 32] |= 1 << index % 32;
    else
        ((u32 *)bmp)[index / 32] &= ~(1 << index % 32);
}

#include <debug.hh>

template<bool set> void BmpWriteBits(u8 *bmp, int index, int count);

void MmPhysicalInitializeState(KernelBootInfo *info)
{
    int reg_count;
    MemRegion regions[64];
    EnumerateRegions(info->MemMapPointer, nullptr, regions, reg_count, 64);

    MemRegion *biggest = nullptr;
    u32 sizeof_biggest = 0;

    for (int i = 0; i < reg_count; i++) {
        u32 size = regions[i].End - regions[i].Start;
        if (size > sizeof_biggest) {
            biggest = regions + i;
            sizeof_biggest = size;
        }
    }

    if (info->KernelPhysicalEnd > biggest->Start)
        biggest->Start = info->KernelPhysicalEnd;

    uptr physical_start = ALIGN(biggest->Start, PAGE_SIZE);
    uptr physical_end = ALIGN(biggest->End, PAGE_SIZE);

    DbgPrintStr("[MmPhysicalInit] physical_start=0x%p, physical_end=0x%p\r\n", physical_start, physical_end);

    int total_pages = (physical_end - physical_start) / PAGE_SIZE;
    int total_entry_count = 0;

    for (int i = 0, p = total_pages; i < ORDERS; i++, p >>= 1)
        total_entry_count += (p + 31) & ~31;

    // Correct available page count accounting for bitmap size
    total_pages -= PAGES_IN(total_entry_count / 8);
    Allocator.BmpPhysicalStart = physical_start;

    for (int i = 0, p = total_pages; i < ORDERS; i++, p >>= 1) {
        Allocator.Bitmaps[i] = (u8 *)physical_start;
        Allocator.BmpSizes[i] = p;
        Allocator.Search[i] = 0;
        physical_start += ((p + 31) & ~31) / 8;
        BmpWriteBits<true>(Allocator.Bitmaps[i], p, 31 - (p & 31));
    }

    Allocator.BmpTotalSize = physical_start - Allocator.BmpPhysicalStart;
    MemSet((void *)Allocator.BmpPhysicalStart, 0, Allocator.BmpTotalSize);
    physical_start = ALIGN(physical_start, PAGE_SIZE << MAX_ORDER);

    Allocator.DataStart = physical_start;
    Allocator.DataEnd = physical_end;
}

#include <mm/virtual.hh>

void MmiPhysicalPostVirtualInit()
{
    uptr new_addr = (uptr)MmVirtualMapPhysical(nullptr,
        Allocator.BmpPhysicalStart,
        Allocator.BmpTotalSize, PAGE_READWRITE);

    for (int i = 0; i < ORDERS; i++)
        Allocator.Bitmaps[i] = RELOCATE(Allocator.Bitmaps[i],
            Allocator.BmpPhysicalStart, new_addr);
}

/* Utility routine to change a range of bits in a bitmap to be a certain value. */
template<bool set> void BmpWriteBits(u8 *bmp, int index, int count)
{
    /* Macros to change several bits in a dword at once. */
#define SET(dest, n, i)   dword[(dest)] |=   (1 << (n)) - 1 << (i)
#define CLEAR(dest, n, i) dword[(dest)] &= ~((1 << (n)) - 1 << (i))

    /* Ignore empty writes */
    if (count <= 0)
        return;

    /* Single-bit writes can be handled by a specialized function. */
    if (count == 1) {
        BmpWriteBit<set>(bmp, index);
        return;
    }

    u32 *dword = (u32 *)bmp;
    /* Check if the final bit's index rolls over to a righter dword */
    if ((index + count >> 5) != (index >> 5)) {
        /* First write bits that do not start on a dword boundary */
        if (index & 31) {
            int l = 32 - (index & 31);
            if (set) SET(index >> 5, l, index & 31);
            else CLEAR(index >> 5, l, index & 31);
            count -= l;
            index += l;
        }

        /* Fill in as many dwords as the remaining count fits */
        while (count >> 5) {
            dword[index >> 5] = set * 0xFFFFFFFF;
            count -= 32;
            index += 32;
        }

        /* Write trailing bits to the last dword */
        if (set) SET(index >> 5, count, 0);
        else CLEAR(index >> 5, count, 0);
    }
    /* If not, simply write the bits to the dword containing the index */
    else if (set)
        SET(index >> 5, count, index & 31);
    else
        CLEAR(index >> 5, count, index & 31);
#undef SET
#undef CLEAR
}

/*
    Bit twiddling hacks to quickly determine
    the lowest set bit of a 32-bit unsigned integer x
    (I didn't write this)
*/
int LowestSetBit(u32 x)
{
    static const int MultiplyDeBruijnBitPosition[] =
    {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return MultiplyDeBruijnBitPosition[((u32)((x & -x) * 0x077CB531U)) >> 27];
}

/*
    Slightly optimized utility function to search for the closest clear bit
    in a bitmap (starting from a certain index) and return its index
*/
u32 BmpSearchClearBit(void *bmp, u32 size, u32 start)
{
    int i = start / 32;
    int j = start / 32;

    u32 dword_size = size / 32;
    u32 *dword = (u32 *)bmp, value;

    /* There are two counters headed in opposite directions, both
       starting from the start parameter of the function, searching
       for dwords with clear bits in the bitmap */
    for (;;) {
        if (i >= 0) {
            /* If there are clear bits in a dword, find the lowest one */
            /* and return its index */
            if (value = ~dword[i])
                return i * 32 + LowestSetBit(value);

            i--;
        }
        else if (j < dword_size) {
            if (value = ~dword[j])
                return j * 32 + LowestSetBit(value);

            j++;
        }
        else
            break;
    }
    return -1;
}

/*
    Function to mark a certain page in a certain order's bitmap
    as allocated or free, making necessary adjustments to the other
    orders' bitmaps as well.
*/
template<bool free> void MarkPageAs(u32 order, u32 page)
{
    for (int i = 0; i < order; i++)
        BmpWriteBits<!free>(Allocator.Bitmaps[i], page << (order - i), 1 << (order - i));

    BmpWriteBits<!free>(Allocator.Bitmaps[order], page, 1);

    if (!free) {
        for (int i = order + 1; i < ORDERS; i++) {
            /* Mark the entry containing this entry in the parent order as used */
            BmpWriteBits<true>(Allocator.Bitmaps[i], page >> (i - order), 1);
        }
    }
    else {
        for (int i = order + 1; i < ORDERS; i++) {
            /* If the sibling of the entry that was just cleared
               is also clear, the entry containing them both
               in the parent order is cleared. */
            int child_index = page >> (i - order - 1);
            bool sibling_clear = !BmpReadBit(Allocator.Bitmaps[i - 1], child_index ^ 1);

            if (sibling_clear)
                BmpWriteBits<false>(Allocator.Bitmaps[i], page >> (i - order), 1);
        }
    }
}

uptr MmPhysicalAllocatePage(int order)
{
    if (order > MAX_ORDER)
        return 0;

    PzAcquireSpinlock(&MmPhysicalLock);
    int index = BmpSearchClearBit(
        Allocator.Bitmaps[order],
        Allocator.BmpSizes[order],
        Allocator.Search[order]);

    if (index == -1) {
        PzReleaseSpinlock(&MmPhysicalLock);
        return 0;
    }

    Allocator.Search[order] = index;
    MarkPageAs<false>(order, index);
    uptr addr = Allocator.DataStart + (index << PAGE_SHIFT << order);
    PzReleaseSpinlock(&MmPhysicalLock);

    return addr;
}

uptr MmPhysicalAllocateContiguousPages(u32 order, u32 count)
{
    if (count == 0 || order > MAX_ORDER)
        return 0;
    else if (count == 1)
        return MmPhysicalAllocatePage(order);

    /* Find the first cave in the order's bitmap that fits the
       necessary amount of pages */
    int streak = 0, streak_start = 0;
    uptr cave_start = 0;

    PzAcquireSpinlock(&MmPhysicalLock);
    for (int i = 0; i < Allocator.BmpSizes[order]; i += 32) {
        u32 chunk = ~((u32 *)Allocator.Bitmaps[order])[i / 32];

        for (int j = 0; j < 32; j++) {
            if (chunk & 1 << j) {
                if (streak == 0) {
                    streak_start = i + j;
                    cave_start = Allocator.DataStart
                        + (streak_start << PAGE_SHIFT << order);
                }

                streak++;

                if (streak == count) {
                    for (int k = 0; k < streak; k++)
                        MarkPageAs<false>(order, streak_start + k);

                    PzReleaseSpinlock(&MmPhysicalLock);
                    return cave_start;
                }
            }
            else
                streak = 0;
        }
    }

    PzReleaseSpinlock(&MmPhysicalLock);
    return 0;
}

bool MmPhysicalFreePages(uptr address, u32 order, u32 number)
{
    if (order > MAX_ORDER)
        return false;

    PzAcquireSpinlock(&MmPhysicalLock);

    int index = ((uptr)address - Allocator.DataStart) >> PAGE_SHIFT >> order;

    if (index < 0 || index >= Allocator.BmpSizes[order]) {
        PzReleaseSpinlock(&MmPhysicalLock);
        return false;
    }

    Allocator.Search[order] = index;

    for (int i = 0; i < number; i++)
        MarkPageAs<true>(order, index + i);

    PzReleaseSpinlock(&MmPhysicalLock);

    return true;
}