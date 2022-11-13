#include <mm/virtual.hh>
#include <mm/physical.hh>
#include <lib/util.hh>
#include <debug.hh>
#include <obj/process.hh>

#define PDE_X86_PRESENT   1
#define PDE_X86_READWRITE 2
#define PDE_X86_USER      4
#define PDE_X86_WRITETHRU 8
#define PDE_X86_NONCACHED 16
#define PDE_X86_ACCESSED  32
#define PDE_X86_LARGEPAGE 128
#define PDE_X86_FREE_BIT  9

#define PAGE_X86_ALLOCATED 1

#define KM_PAGE_INDEX_TO_ADDR(index) (void*)(KERNEL_SPACE_START + (index) * PAGE_SIZE)

/* This is the global pointer to the page table of the upper half of
   virtual memory, global to all processes and page tables. */
static uptr *KernelHalfPtBase;

/* A pointer to a chunk of the page table allocated to
   identity mapping the lower 2 MB of physical memory. */
static uptr *BootImapPtBase;

#define PT_VIRT_BASE ((uptr*)-(1024 * 1024 * 2))
#define PD_VIRT_BASE ((uptr*)-4096)

/* This is a static array storing the initial page directory of the kernel,
   used during the boot process before the scheduler fires up. */
static uptr *BootPageDir;

static PzSpinlock MmVirtualLock, MmVirtualDmaLock;

void MmVirtualInitBootPageTable(KernelBootInfo *info)
{
    constexpr u32 lower_imap_pages = PAGES_IN(0x200000);

    KernelHalfPtBase = (uptr *)MmPhysicalAllocateContiguousPages(0, PAGES_IN(KERNEL_SPACE_SIZE) / 1024);
    BootImapPtBase = (uptr *)MmPhysicalAllocateContiguousPages(0, ALIGN(lower_imap_pages, 1024) / 1024);
    BootPageDir = (uptr *)MmPhysicalAllocatePage(0);

    int kernel_pages = PAGES_IN(info->KernelPhysicalEnd - info->KernelPhysicalStart);

    /* Map the kernel into the higher half. */
    for (int i = 0; i < kernel_pages; i++)
        KernelHalfPtBase[i] = info->KernelPhysicalStart + i * PAGE_SIZE
        | PDE_X86_READWRITE | PDE_X86_PRESENT;

    /* Zero out remaining page entries. */
    for (int i = kernel_pages; i < PAGES_IN(KERNEL_SPACE_SIZE); i++)
        KernelHalfPtBase[i] = 0;

    int recursive_pages = PAGES_IN(1024 * 1024 * 2);
    uptr *recursive_map = KernelHalfPtBase + 1024 * 1024 / 2 - recursive_pages;

    for (int i = 0; i < recursive_pages - 1; i++)
        recursive_map[i] = (uptr)KernelHalfPtBase + i * 4096
        | PDE_X86_READWRITE | PDE_X86_PRESENT;

    recursive_map[recursive_pages - 1] =
        (uptr)BootPageDir | PDE_X86_READWRITE | PDE_X86_PRESENT;

    for (int i = 0; i < kernel_pages; i++)
        KernelHalfPtBase[i] = info->KernelPhysicalStart + i * PAGE_SIZE
        | PDE_X86_READWRITE | PDE_X86_PRESENT;

    BootImapPtBase[0] = 0u;
    for (int i = 1; i < lower_imap_pages; i++)
        BootImapPtBase[i] = i * PAGE_SIZE | PDE_X86_READWRITE | PDE_X86_PRESENT;

    /* Map identity mapping page table into the page directory */
    for (int i = 0; i < ALIGN(lower_imap_pages, 1024) / 1024; i++)
        BootPageDir[i] = uptr(BootImapPtBase + i * 1024) | PDE_X86_READWRITE | PDE_X86_PRESENT;

    /* Map kernel half page table into the page directory */
    for (int i = 0; i < 512; i++)
        BootPageDir[i + 512] = uptr(KernelHalfPtBase + i * 1024) | PDE_X86_READWRITE | PDE_X86_PRESENT;

    HalSwitchPageTable((uptr)BootPageDir);
    MmiPhysicalPostVirtualInit();
}

int MmFindVirtualCave(u32 pages)
{
    if (pages == 0)
        return -1;

    for (int i = 0; i < PAGES_IN(KERNEL_SPACE_SIZE); i++) {
        int streak = 0, index = i;

        while (!(PT_VIRT_BASE[i] & PDE_X86_PRESENT)) {
            if (streak++ == pages)
                return index;

            if (++i >= PAGES_IN(KERNEL_SPACE_SIZE))
                return -1;
        }
    }

    return -1;
}

u32 KernelFlagsToPtFlags(u32 flags)
{
    return PDE_X86_PRESENT | (flags & PAGE_WRITE ? PDE_X86_READWRITE : 0);
}

bool MmVirtualProtectMemory(void *start, u32 bytes, u32 flags)
{
    int pages = PAGES_IN(bytes + (uptr)start % PAGE_SIZE);

    if (!pages)
        return false;

    PzAcquireSpinlock(&MmVirtualLock);

    int index = (uptr(start) - KERNEL_SPACE_START) / PAGE_SIZE;

    for (int i = 0; i < pages; i++)
        if (!(PT_VIRT_BASE[index + i] & PDE_X86_PRESENT))
            goto fail;

    for (int i = 0; i < pages; i++) {
        PT_VIRT_BASE[index + i] &= -PAGE_SIZE;
        PT_VIRT_BASE[index + i] |= KernelFlagsToPtFlags(flags);
        HalFlushCacheForPage(KM_PAGE_INDEX_TO_ADDR(index + i));
    }

    PzReleaseSpinlock(&MmVirtualLock);
    return true;

fail:
    PzReleaseSpinlock(&MmVirtualLock);
    return false;
}

void *MmVirtualMapPhysical(void *start, uptr physical_addr, u32 bytes, u32 flags)
{
    if (physical_addr % PAGE_SIZE)
        return nullptr;

    int pages = PAGES_IN(bytes);

    if (!pages)
        return nullptr;

    int index = 0;

    PzAcquireSpinlock(&MmVirtualLock);

    if (start != nullptr) {
        if (uptr(start) < KERNEL_SPACE_START)
            goto fail;

        index = (uptr(start) - KERNEL_SPACE_START) / PAGE_SIZE;

        for (int i = 0; i < pages; i++)
            if (PT_VIRT_BASE[index + i] & PDE_X86_PRESENT)
                goto fail;
    }
    else if ((index = MmFindVirtualCave(pages)) == -1)
        goto fail;

    for (int i = 0; i < pages; i++) {
        PT_VIRT_BASE[index + i] = physical_addr + i * PAGE_SIZE | KernelFlagsToPtFlags(flags);
        HalFlushCacheForPage(KM_PAGE_INDEX_TO_ADDR(index + i));
    }

    PzReleaseSpinlock(&MmVirtualLock);
    return KM_PAGE_INDEX_TO_ADDR(index);

fail:
    PzReleaseSpinlock(&MmVirtualLock);
    return nullptr;
}

void *MmVirtualAllocateMemory(
    void *start, u32 bytes, u32 flags, uptr *last_page_physical)
{
    if (uptr(start) % PAGE_SIZE)
        return nullptr;

    int pages = PAGES_IN(bytes);
    if (!pages)
        return nullptr;

    int index = 0;

    PzAcquireSpinlock(&MmVirtualLock);

    if (start != nullptr) {
        if (uptr(start) < KERNEL_SPACE_START)
            goto fail;

        index = (uptr(start) - KERNEL_SPACE_START) / PAGE_SIZE;

        for (int i = 0; i < pages; i++) {
            if (PT_VIRT_BASE[index + i] & PDE_X86_PRESENT)
                goto fail;
        }
    }
    else if ((index = MmFindVirtualCave(pages)) == -1)
        goto fail;

    for (int i = 0; i < pages; i++) {
        uptr physical_page = (uptr)MmPhysicalAllocatePage(0);

        if (!physical_page) {
            /* Revert every physical allocation if one allocation fails */
            for (int j = 0; j < i; j++) {
                MmPhysicalFreePages(PT_VIRT_BASE[index + j] & -PAGE_SIZE, 0, 1);
                PT_VIRT_BASE[index + j] = 0;
                HalFlushCacheForPage(KM_PAGE_INDEX_TO_ADDR(index + j));
            }

            goto fail;
        }

        if (last_page_physical)
            *last_page_physical = physical_page;

        PT_VIRT_BASE[index + i] =
            physical_page |
            PAGE_X86_ALLOCATED << PDE_X86_FREE_BIT |
            KernelFlagsToPtFlags(flags);

        HalFlushCacheForPage(KM_PAGE_INDEX_TO_ADDR(index + i));
    }

    PzReleaseSpinlock(&MmVirtualLock);
    return KM_PAGE_INDEX_TO_ADDR(index);

fail:
    PzReleaseSpinlock(&MmVirtualLock);
    return nullptr;
}

bool MmVirtualFreeMemory(void *start, u32 bytes)
{
    int pages = PAGES_IN(bytes);

    if (!pages)
        return false;

    PzAcquireSpinlock(&MmVirtualLock);
    int index = (uptr(start) - KERNEL_SPACE_START) / PAGE_SIZE;

    for (int i = 0; i < pages; i++) {
        if (!(PT_VIRT_BASE[index + i] & PDE_X86_PRESENT)) {
            PzReleaseSpinlock(&MmVirtualLock);
            return false;
        }
    }

    for (int i = 0; i < pages; i++) {
        uptr &old = PT_VIRT_BASE[index + i];
        if (((old >> PDE_X86_FREE_BIT) & 7) == PAGE_X86_ALLOCATED)
            MmPhysicalFreePages(old & -PAGE_SIZE, 0, 1);

        old = 0;
        HalFlushCacheForPage(KM_PAGE_INDEX_TO_ADDR(index + i));
    }

    PzReleaseSpinlock(&MmVirtualLock);
    return true;
}

uptr MmiVirtualToPhysical(void *page, PzProcessObject *process)
{
    uptr p = (uptr)page;

    if (p >= KERNEL_SPACE_START) {
        PzAcquireSpinlock(&MmVirtualLock);
        uptr addr = PT_VIRT_BASE[(p - KERNEL_SPACE_START) / PAGE_SIZE] & -PAGE_SIZE;
        PzReleaseSpinlock(&MmVirtualLock);

        return addr;
    }

    if (!process)
        return 0;

    auto *lock = &process->VirtualAllocations.Spinlock;
    PzAcquireSpinlock(lock);

    if (uptr *pd = process->VirtualPageDirectory[p >> 22]) {
        uptr addr = pd[p >> 12 & 0x3FF] & -PAGE_SIZE;
        PzReleaseSpinlock(lock);
        return addr;
    }

    PzReleaseSpinlock(lock);
    return 0;
}

uptr MmVirtualToPhysical(void *page, PzHandle process)
{
    PzProcessObject *proc_obj = nullptr;

    if (uptr(page) < KERNEL_SPACE_START &&
        !ObReferenceObjectByHandle(PZ_OBJECT_PROCESS, nullptr, process, (ObPointer *)&proc_obj))
        return 0;

    uptr ret = MmiVirtualToPhysical(page, proc_obj);

    if (proc_obj)
        ObDereferenceObject(proc_obj);

    return ret;
}

void *MmAllocateDmaMemory(u32 bytes, u32 alignment, uptr *physical, u32 flags)
{
    if (alignment < PAGE_SHIFT || !bytes)
        return nullptr;

    int pages = (bytes + (1u << alignment) - 1) >> alignment;

    PzAcquireSpinlock(&MmVirtualDmaLock);

    if (uptr addr = MmPhysicalAllocateContiguousPages(alignment - PAGE_SHIFT, pages)) {
        if (physical)
            *physical = addr;

        if (void *ret = MmVirtualMapPhysical(nullptr, addr, bytes, flags)) {
            PzReleaseSpinlock(&MmVirtualDmaLock);
            return ret;
        }

        MmPhysicalFreePages(addr, alignment - PAGE_SHIFT, pages);
    }

    PzReleaseSpinlock(&MmVirtualDmaLock);
    return nullptr;
}

bool MmFreeDmaMemory(void *addr, u32 alignment, u32 bytes)
{
    if (alignment < PAGE_SHIFT || !bytes || !addr)
        return false;

    PzAcquireSpinlock(&MmVirtualDmaLock);

    uptr address = MmVirtualToPhysical(addr, 0);
    int pages = (bytes + (1u << alignment) - 1) >> alignment;
    bool result = false;

    if (!MmVirtualFreeMemory(addr, bytes))
        goto quit;

    result = MmPhysicalFreePages(address, alignment - PAGE_SHIFT, pages);

quit:
    PzReleaseSpinlock(&MmVirtualDmaLock);
    return result;
}

uptr **MmiAllocateProcessPageDirectory(PzProcessObject *process)
{
    uptr **page_dir = process->VirtualPageDirectory =
        (uptr **)MmVirtualAllocateMemory(nullptr, 1024 * sizeof(uptr),
            PAGE_READWRITE, nullptr);

    if (!page_dir)
        return nullptr;

    if (!(process->PhysicalPageDirectory =
        (uptr **)MmVirtualAllocateMemory(nullptr, 1024 * sizeof(uptr),
            PAGE_READWRITE, &process->Cr3))) {

        MmVirtualFreeMemory(page_dir, 1024 * sizeof(uptr));
        return nullptr;
    }

    for (int i = 0; i < 512; i++) {
        page_dir[i] = 0;
        process->PhysicalPageDirectory[i] = 0;
    }

    for (int i = 0; i < 512; i++) {
        process->PhysicalPageDirectory[i + 512] =
            page_dir[i + 512] = (uptr *)((uptr)KernelHalfPtBase + i * PAGE_SIZE
                | PDE_X86_READWRITE | PDE_X86_PRESENT);
    }

    return page_dir;
}

void MmiFreeProcessPageDirectory(PzProcessObject *process)
{
    if (!process->PhysicalPageDirectory || !process->VirtualPageDirectory)
        return;

    for (int i = 0; i < 512; i++)
        if (process->VirtualPageDirectory[i])
            MmVirtualFreeMemory(process->VirtualPageDirectory[i], PAGE_SIZE);

    MmVirtualFreeMemory(process->VirtualPageDirectory, PAGE_SIZE);
    MmVirtualFreeMemory(process->PhysicalPageDirectory, PAGE_SIZE);
}

#include <core.hh>

void *MmiVirtualAllocateUserMemory(PzProcessObject *process, void *start, u32 bytes, u32 flags)
{
    PzSpinlock *lock = &process->VirtualAllocations.Spinlock;
    PzAcquireSpinlock(lock);

    u32 **phys_page_dir = (u32 **)process->PhysicalPageDirectory;
    u32 **virt_page_dir = (u32 **)process->VirtualPageDirectory;
    uptr last_cave = 0x1000;
    int index = -1;

    LLNode<PzUserVirtualRegion> *insert_at = nullptr;

    if (start == nullptr) {
        auto *node = process->VirtualAllocations.First;

        for (; node; node = node->Next) {
            if (last_cave + bytes <= node->Value.Start) {
                index = last_cave / PAGE_SIZE;
                break;
            }

            insert_at = node;
            last_cave = node->Value.End;
        }

        if (index == -1) {
            if ((index = last_cave / PAGE_SIZE) >= 512 * 1024) {
                PzReleaseSpinlock(lock);
                return nullptr;
            }
        }
    }
    else {
        /* Allocating on non-page-aligned addresses or
           above userspace is not allowed */
        if (uptr(start) % PAGE_SIZE ||
            uptr(start) > KERNEL_SPACE_START ||
            uptr(start) + bytes > KERNEL_SPACE_START) {
            PzReleaseSpinlock(lock);
            return nullptr;
        }

        iptr min_dist = INT32_MAX;
        bool first = true;

        for (auto *node = process->VirtualAllocations.First;
            node; node = node->Next, first = false) {
            /* If any of the allocated ranges overlap with the range we want to
               allocate, bail out */
            if (OVERLAPPING(
                (uptr)start, (uptr)start + bytes,
                node->Value.Start, node->Value.End)) {
                PzReleaseSpinlock(lock);
                return nullptr;
            }

            /* Find an allocation with the minimum distance
               to this one and insert after its node to keep list sorted */

            iptr dist = (iptr)start - node->Value.End;

            /* If first node is higher than the end of this allocation,
               then the entry will be prepended to the list */
            if (first && (uptr)start + bytes < node->Value.Start) {
                insert_at = nullptr;
                break;
            }
            else if (dist < min_dist) {
                min_dist = dist;
                insert_at = node;
            }
        }

        index = (uptr)start / PAGE_SIZE;
    }

    void *base = (void*)(index * PAGE_SIZE);

    auto *alloc_node = process->VirtualAllocations.Insert(insert_at,
        PzUserVirtualRegion { (uptr)base, (uptr)base + ALIGN(bytes, PAGE_SIZE), 0 });

    if (!alloc_node) {
        PzReleaseSpinlock(lock);
        return nullptr;
    }

    for (int i = 0; i < PAGES_IN(bytes); i++, index++) {
        u32 *&dir_ent = virt_page_dir[index >> 10];

        if (!dir_ent) {
            dir_ent = (u32 *)MmVirtualAllocateMemory(nullptr,
                1024 * sizeof(u32), PAGE_READWRITE, (u32 *)&phys_page_dir[index >> 10]);

            if (!dir_ent) {
                process->VirtualAllocations.Remove(alloc_node);
                PzReleaseSpinlock(lock);
                return nullptr;
            }

            MemSet(dir_ent, 0, 1024 * sizeof(u32));
            *(u32 *)&phys_page_dir[index >> 10] |= PDE_X86_READWRITE | PDE_X86_USER | PDE_X86_PRESENT;
        }

        uptr physical_page = (uptr)MmPhysicalAllocatePage(0);

        if (!physical_page) {
            /* TODO: make a proper revert routine */
            process->VirtualAllocations.Remove(alloc_node);
            PzReleaseSpinlock(lock);
            return nullptr;
        }

        dir_ent[index & 0x3FF] =
            physical_page | PDE_X86_USER |
            PAGE_X86_ALLOCATED << PDE_X86_FREE_BIT | KernelFlagsToPtFlags(flags);

        HalFlushCacheForPage((void *)(index << PAGE_SHIFT));
    }

    PzReleaseSpinlock(lock);
    return base;
}

bool MmiVirtualFreeUserMemory(PzProcessObject *process, void *start, u32 flags)
{
    if (!start)
        return false;

    PzSpinlock *lock = &process->VirtualAllocations.Spinlock;
    PzAcquireSpinlock(lock);

    for (auto *node = process->VirtualAllocations.First;
        node; node = node->Next) {
        uptr **phys_page_dir = (uptr **)process->PhysicalPageDirectory;
        uptr **virt_page_dir = (uptr **)process->VirtualPageDirectory;

        uptr istart = uptr(start);

        if (node->Value.Start == istart) {
            for (; istart < node->Value.End; istart += PAGE_SIZE) {
                uptr &entry = virt_page_dir[istart >> 22][istart >> 12 & 0x3FF];

                if (((entry >> PDE_X86_FREE_BIT) & 7) == PAGE_X86_ALLOCATED)
                    MmPhysicalFreePages(entry & -PAGE_SIZE, 0, 1);

                HalFlushCacheForPage((void *)entry);
                entry = 0;
            }

            process->VirtualAllocations.Remove(node);
            PzReleaseSpinlock(lock);
            return true;
        }
    }

    PzReleaseSpinlock(lock);
    return false;
}

bool MmiVirtualProtectUserMemory(PzProcessObject *process, void *start, u32 bytes, u32 flags)
{
    if (!start)
        return false;

    PzSpinlock *lock = &process->VirtualAllocations.Spinlock;
    PzAcquireSpinlock(lock);

    uptr **virt_page_dir = (uptr **)process->VirtualPageDirectory;
    uptr end_ptr = ALIGN((uptr)start + bytes, PAGE_SIZE);

    #define ENTRY virt_page_dir[start_ptr >> 22][start_ptr >> 12 & 0x3FF]

    for (uptr start_ptr = (uptr)start; start_ptr < end_ptr; start_ptr += PAGE_SIZE) {
        if (!(ENTRY & PDE_X86_PRESENT)) {
            PzReleaseSpinlock(lock);
            return false;
        }
    }

    for (uptr start_ptr = (uptr)start; start_ptr < end_ptr; start_ptr += PAGE_SIZE) {
        uptr &entry = ENTRY;
        entry &= -PAGE_SIZE;
        entry |= KernelFlagsToPtFlags(flags) | PDE_X86_USER;
    }

    #undef ENTRY

    PzReleaseSpinlock(lock);
    return true;
}

void *MmVirtualAllocateUserMemory(PzHandle process, void *start, u32 bytes, u32 flags)
{
    PzProcessObject *proc_obj;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_PROCESS, nullptr, process, (ObPointer *)&proc_obj))
        return nullptr;

    void *ret = MmiVirtualAllocateUserMemory(proc_obj, start, bytes, flags);
    
    ObDereferenceObject(proc_obj);
    return ret;
}

bool MmVirtualFreeUserMemory(PzHandle process, void *start, u32 flags)
{
    PzProcessObject *proc_obj;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_PROCESS, nullptr, process, (ObPointer *)&proc_obj))
        return false;

    bool ret = MmiVirtualFreeUserMemory(proc_obj, start, flags);

    ObDereferenceObject(proc_obj);
    return ret;
}

bool MmVirtualProtectUserMemory(PzHandle process, void *start, u32 bytes, u32 flags)
{
    PzProcessObject *proc_obj;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_PROCESS, nullptr, process, (ObPointer *)&proc_obj))
        return false;

    bool ret = MmiVirtualProtectUserMemory(proc_obj, start, bytes, flags);

    ObDereferenceObject(proc_obj);
    return ret;
}

#include <sched/scheduler.hh>

bool MmVirtualProbeKernelMemory(uptr start, usize size, bool write)
{
    if (start < KERNEL_SPACE_START)
        return false;

    uptr end = ALIGN(start + size, PAGE_SIZE);

    PzAcquireSpinlock(&MmVirtualLock);

    for (; start < end; start += PAGE_SIZE) {
        uptr flags = PT_VIRT_BASE[(start - KERNEL_SPACE_START) >> PAGE_SHIFT];

        if (!(flags & PDE_X86_PRESENT) ||
            write && !(flags & PDE_X86_READWRITE)) {
            PzReleaseSpinlock(&MmVirtualLock);
            return false;
        }
    }

    PzReleaseSpinlock(&MmVirtualLock);
    return true;
}

bool MmVirtualProbeMemory(bool as_user, uptr start, usize size, bool write)
{
    if (start >= KERNEL_SPACE_START)
        return !as_user && MmVirtualProbeKernelMemory(start, size, write);

    uptr end = ALIGN(start + size, PAGE_SIZE);
    PzProcessObject *process = PsGetCurrentProcess();

    if (!process)
        return false;

    PzSpinlock *lock = &process->VirtualAllocations.Spinlock;
    uptr **pd = process->VirtualPageDirectory;

    PzAcquireSpinlock(lock);

    for (; start < end; start += PAGE_SIZE) {
        uptr flags = pd[start >> 22][start >> 12 & 0x3FF];

        if (!(flags & PDE_X86_PRESENT)         ||
            as_user && !(flags & PDE_X86_USER) ||
            write && !(flags & PDE_X86_READWRITE)) {
            PzReleaseSpinlock(lock);
            return false;
        }
    }

    PzReleaseSpinlock(lock);
    return true;
}