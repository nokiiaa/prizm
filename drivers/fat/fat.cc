#include "fat.hh"
#include <io/manager.hh>
#include <lib/util.hh>
#include <debug.hh>

PzStatus FatReadCluster(
    FatVolumeContext *fat, u32 cluster, void *buffer,
    int offset, u32 bytes, u32 &read)
{
    cluster &= 0x0FFFFFFF;
    cluster -= 2;

    u64 off = fat->DataStartOffset + fat->ClusterSize * cluster + offset;
    PzIoStatusBlock iosb = { 0 };
    PzStatus status = PzReadFileRaw(fat->BlockDevice, buffer, &iosb, bytes, &off);
    read = iosb.Information;

    return status;
}

PzStatus FatWriteCluster(
    FatVolumeContext *fat, u32 cluster, const void *buffer,
    int offset, u32 bytes, u32 &read)
{
    cluster &= 0x0FFFFFFF;
    cluster -= 2;

    u64 off = fat->DataStartOffset + fat->ClusterSize * cluster + offset;
    PzIoStatusBlock iosb = { 0 };
    PzStatus status = PzWriteFileRaw(fat->BlockDevice, buffer, &iosb, bytes, &off);
    read = iosb.Information;
    
    return status;
}

PzStatus FatGetNextCluster(FatVolumeContext *fat, u32 cluster, u32 &out)
{
    PzIoStatusBlock iosb;
    u64 offset = fat->FatStartOffset + cluster * sizeof(u32);
    return PzReadFileRaw(fat->BlockDevice, &out, &iosb, sizeof(u32), &offset);
   
    /*int bps = fat->Bpb.BytesPerSector;
    int entries_per_fat_sector = bps / sizeof(cluster);
    int fat_sector = cluster / entries_per_fat_sector;
    int sector_index = cluster % entries_per_fat_sector;
    void *cache_buffer;

    PzAcquireSpinlock(&fat->FatCache.Spinlock);

    if (fat->FatCache.Find(fat_sector, cache_buffer)) {
        out = ((u32 *)cache_buffer)[sector_index];
        //if (out >= 0xFFFFFF8) DbgPrintStr("found in cache; fat_sector=%i sector_index=%i out=%i\r\n", fat_sector, sector_index, out);
        PzReleaseSpinlock(&fat->FatCache.Spinlock);
        return STATUS_SUCCESS;
    }

    PzReleaseSpinlock(&fat->FatCache.Spinlock);

    u64 off = fat->FatStartOffset + fat_sector * bps;
    u32 *fat_buffer = (u32 *)MmVirtualAllocateMemory(nullptr, bps, PAGE_READWRITE, nullptr);
    PzIoStatusBlock iosb;

    if (!fat_buffer)
        return STATUS_ALLOCATION_FAILED;

    if (PzStatus status = PzReadFileRaw(fat->BlockDevice, fat_buffer, &iosb, bps, &off)) {
        MmVirtualFreeMemory(fat_buffer, bps);
        return STATUS_TRANSFER_FAILED;
    }*/

    /* Didn't read whole sector */
    /*if (iosb.Information != bps) {
        MmVirtualFreeMemory(fat_buffer, bps);
        return STATUS_TRANSFER_FAILED;
    }

    PzAcquireSpinlock(&fat->FatCache.Spinlock);
    fat->FatCache.Add(fat_sector, fat_buffer);
    out = fat_buffer[sector_index];
    //if (out >= 0xFFFFFF8) DbgPrintStr("not found in cache; fat_sector=%i sector_index=%i out=%i\r\n", fat_sector, sector_index, out);
    PzReleaseSpinlock(&fat->FatCache.Spinlock);

    return STATUS_SUCCESS;*/
}

void PrependLfn(PzString *path, Fat32LfnEntry *lfn)
{
    char cons[13 + 1] = { 0 };

    for (int i = 0; 13 > i; i++) {
        char c;
        if (i < 5)
            c = (char)lfn->Chars1[i];
        else if (i < 11)
            c = (char)lfn->Chars2[i - 5];
        else if (i < 13)
            c = (char)lfn->Chars3[i - 11];

        if (!c) break;
        else cons[i] = c;
    }

    PzString cs;
    PzInitStringViewUtf8(&cs, cons);
    PzPrependString(path, &cs);
}

void NormalizeShortName(char *output, char short_name[11], bool dir = false)
{
    int j = 0;
    if (dir)
        for (int i = 0; 11 > i; i++)
            output[j++] = short_name[i];
    else {
        for (int i = 0; 8 > i; i++) {
            if (short_name[i] == ' ') {
                for (int j = i; 8 > j; j++)
                    if (short_name[j] != ' ')
                        break;

                goto end_name;
            }
            output[j++] = short_name[i];
        }

    end_name:
        output[j++] = '.';

        for (int i = 8; 11 > i; i++)
            output[j++] = short_name[i];
    }
    output[j++] = '\0';
}

u32 FatFollowClusterChain(FatVolumeContext *fat, u32 &clus_num, int max_bytes, bool contiguous)
{
    int cs = fat->ClusterSize;
    u32 advanced = 0;

    while (max_bytes > 0) {
        if ((clus_num & 0x0FFFFFFF) >= 0x0FFFFFF8)
            return advanced;

        u32 old_clus = clus_num;

        // Add remaining bytes that don't form a full cluster and return
        if (max_bytes < cs)
            return advanced + max_bytes;

        if (PzStatus status = FatGetNextCluster(fat, clus_num, clus_num))
            return 0;

        u32 new_clus = clus_num;
        max_bytes -= cs;
        advanced += cs;

        // Contiguous area stops here
        if (contiguous && new_clus - old_clus != 1)
            return advanced;
    }

    return advanced;
}

u8 NameChecksum(char short_name[11])
{
    u8 sum = 0;

    for (int i = 0; i < 11; i++)
        sum = ((sum & 1) << 7) + (sum >> 1) + (u8)*short_name++;

    return sum;
}

#include <debug.hh>

PzStatus FatLocateEntry(FatVolumeContext *fat,
    const Fat32DirEntry &parent, const PzString *path,
    Fat32DirEntry &out, u64 &vol_offset)
{
    u32 _, next;
    u32 cs = fat->ClusterSize;
    u32 num_entries = cs / sizeof(Fat32DirEntry);
    u32 cluster = parent.ClusterNumber();
    Fat32DirEntry *ptr = new Fat32DirEntry[num_entries];

    if (!ptr)
        return STATUS_ALLOCATION_FAILED;

    bool found_lfn = false, recurse = false;
    PzString *lfn_name = PzAllocateString();
    const char *pptr = path->Buffer;
    u8 lfn_checksum;
    int entry_name_length = 0;
    int value = 0;

    do {
        pptr = Utf8DecodeNext(pptr, value);
        if (value == '/' || value == '\\') {
            recurse = true;
            break;
        }
        entry_name_length++;
    } while (value);

    PzString rest_of_path;
    PzInitStringViewUtf8(&rest_of_path, (char *)pptr);

    for (; (cluster & 0xFFFFFFFu) < 0xFFFFFF8u; ) {
        //DbgPrintStr("cluster=%i\r\n", cluster);

        if (PzStatus status = FatReadCluster(fat, cluster, ptr, 0, cs, _))
            return status;

        for (int i = 0; num_entries > i; i++) {
            u8 first_char = ptr[i].Filename[0];

            // File is free or deleted
            if (first_char == 0 || first_char == 0xE5 || first_char == 0x05)
                continue;

            if ((ptr[i].Attributes & FAT_LFN) == FAT_LFN) {
                auto lfn_entry = (Fat32LfnEntry *)(ptr + i);

                // This is the first LFN entry for this file
                if (!found_lfn) {
                    PzFreeString(lfn_name);
                    lfn_name = PzAllocateString();
                }
                // This is not the first entry, check if the checksum
                // of the previous one matches this one
                else if (lfn_entry->Checksum != lfn_checksum) {
                    // It's an orphan LFN.
                    found_lfn = false;
                    lfn_checksum = 0;
                    continue;
                }

                found_lfn = true;
                PrependLfn(lfn_name, lfn_entry);
                lfn_checksum = lfn_entry->Checksum;
            }
            else if (found_lfn) {
                found_lfn = false;
                // That was an orphan LFN. Backtrack and parse this short-named entry normally
                if (NameChecksum(ptr[i].Filename) != lfn_checksum) {
                    i--;
                    lfn_checksum = 0;
                    continue;
                }

                // This is the short-named entry that
                // the LFNs preceded
                // Compare the final LFN string against our entry name
                if (Utf8CompareRawStringsCaseIns(
                    path->Buffer, entry_name_length, lfn_name->Buffer, -1) == 0)
                    goto finish;
            }
            else {
                char buffer[13];

                NormalizeShortName(buffer, ptr[i].Filename,
                    ptr[i].Attributes & FAT_DIRECTORY);

                if (Utf8CompareRawStringsCaseIns(
                    buffer, -1, path->Buffer, entry_name_length) == 0)
                    goto finish;
            }
            continue;
        finish:
            if (lfn_name)
                PzFreeString(lfn_name);

            if (recurse) {
                PzStatus status = FatLocateEntry(fat,
                    ptr[i], &rest_of_path, out, vol_offset);

                delete[] ptr;
                return status;
            }

            else {
                out = ptr[i];
                vol_offset = fat->DataStartOffset +
                    cs * cluster + i * sizeof(Fat32DirEntry);

                delete[] ptr;
                return STATUS_SUCCESS;
            }
        }

        if (FatFollowClusterChain(fat, cluster, cs, false) <= 0)
            break;
    }

    //DbgPrintStr("!!! ptr=%p\r\n", ptr);
    //for (;;);
    delete[] ptr;
    return STATUS_PATH_NOT_FOUND;
}

PzStatus FatInitFile(
    FatVolumeContext *fat,
    const PzString *filename,
    FatFileContext *out_file)
{
    Fat32DirEntry root;
    root.ClusterLo = 2;
    root.ClusterHi = 0;

    if (PzStatus status = FatLocateEntry(fat, root, filename,
        out_file->DirEntry, out_file->EntryVolumeOffset))
        return status;

    out_file->CurrentCluster = out_file->DirEntry.ClusterNumber();
    out_file->Size = out_file->DirEntry.FileSize;

    return STATUS_SUCCESS;
}

PzStatus FatReadFile(
    FatVolumeContext *fat,
    FatFileContext *file,
    PzIoControlBlock *iocb,
    void *buffer, u64 offset,
    u32 length, u32 &read)
{
    u32 cs = fat->ClusterSize, _;

    if (iocb->CurrentOffset != offset) {
        u32 advance;

        if (iocb->CurrentOffset < offset) {
            file->CurrentCluster = file->DirEntry.ClusterNumber();
            advance = (offset - iocb->CurrentOffset) & -cs;
        }
        else
            advance = offset;

        if (FatFollowClusterChain(
            fat, file->CurrentCluster, advance, false) !=
            (offset & -cs)) {
            read = 0;
            return STATUS_SUCCESS;
        }
    }

    length = Min(u32(file->Size - offset), length);
    u8 *byte_buffer = (u8 *)buffer;
    int ptr_unaligned = offset % cs;
    int unaligned = Min(cs - ptr_unaligned, length);
    u32 total_read = 0;

    if (ptr_unaligned > 0) {
        if (FatReadCluster(fat, file->CurrentCluster, byte_buffer, ptr_unaligned, unaligned, total_read)
            != STATUS_SUCCESS) {
            return STATUS_TRANSFER_FAILED;
        }

        if ((offset + length) / cs != offset / cs)
            FatGetNextCluster(fat, file->CurrentCluster, file->CurrentCluster);

        byte_buffer += unaligned;
        length -= unaligned;
    }

    while (length) {
        int start = file->CurrentCluster;
        u32 chunk = FatFollowClusterChain(fat, file->CurrentCluster, length, true);
        u32 left = Min(length, chunk);

        if (FatReadCluster(fat, start, byte_buffer, 0, left, _) != STATUS_SUCCESS)
            return STATUS_TRANSFER_FAILED;

        total_read += _;
        byte_buffer += left;
        length -= left;
    }

    read = total_read;
    return STATUS_SUCCESS;
}