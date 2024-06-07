#pragma once

#include <defs.hh>
#include <lib/string.hh>
#include <lib/list.hh>
#include <io/manager.hh>

#define FAT_READONLY  (1u << 0)
#define FAT_HIDDEN    (1u << 1)
#define FAT_SYSTEM    (1u << 2)
#define FAT_VOLUME_ID (1u << 3)
#define FAT_DIRECTORY (1u << 4)
#define FAT_ARCHIVE   (1u << 5)
#define FAT_LFN       (FAT_READONLY | FAT_HIDDEN | FAT_SYSTEM | FAT_VOLUME_ID)

#pragma pack(push, 1)
struct Fat32Time
{
    u16 Hour : 5;
    u16 Minutes : 6;
    u16 Seconds : 5;
};

struct Fat32Date
{
    u16 Year : 7;
    u16 Month : 4;
    u16 Day : 5;
};

struct Fat32DirEntry
{
    char Filename[11];
    u8 Attributes;
    u8 NTReserved;
    u8 CreationTimeSec;
    Fat32Time CreationTime;
    Fat32Date CreationDate;
    Fat32Date LastAccessDate;
    u16 ClusterHi;
    Fat32Time ModifiedTime;
    Fat32Date ModifiedDate;
    u16 ClusterLo;
    u32 FileSize;
    inline u32 ClusterNumber() const { return ClusterLo | ClusterHi << 16; }
};

struct Fat32LfnEntry
{
    u8 Order;
    i16 Chars1[5];
    u8 Attributes;
    u8 LongEntryType;
    u8 Checksum;
    i16 Chars2[6];
    i16 Reserved;
    i16 Chars3[2];
};

struct Fat32Bpb
{
    u8 Jump[3];
    char OEM[8];
    u16 BytesPerSector;
    u8 SectorsPerCluster;
    u16 ReservedSectors;
    u8 FatCount;
    u16 EntryCount;
    u16 TotalSectors16;
    u8 MediaDescType;
    u16 SectorsPerFat16;
    u16 SectorsPerTrack;
    u16 HeadCount;
    u32 HiddenSectors;
    u32 TotalSectors32;
    /* FAT32 EBPB */
    u32 SectorsPerFat32;
    u16 Flags;
    u16 FatVersion;
    u32 RootDirCluster;
    u16 FsInfoSector;
    u8 Reserved[12];
    u8 DriveNumber;
    u8 NTFlags;
    u8 Signature;
    u32 SerialNumber;
    char VolumeLabel[11];
    char SystemId[8];
};
#pragma pack(pop)

struct CacheEntry
{
    u64 Id;
    void *Data;
    inline CacheEntry(u64 id, void *data) : Id(id), Data(data) {}
    inline CacheEntry() {}
};

struct Cache
{
    LinkedList<CacheEntry> Entries;
    PzSpinlock Spinlock;

    inline CacheEntry &Add(u64 id, void *value)
    {
        return Entries.Add(CacheEntry(id, value))->Value;
    }

    inline bool Find(u64 id, void *&data)
    {
        auto node = Entries.First;
        while (node) {
            if (node->Value.Id == id) {
                data = node->Value.Data;
                return true;
            }
            node = node->Next;
        }
        return false;
    }
};

struct FatVolumeContext
{
    PzIoControlBlock *BlockDevice;
    PzString *MountPoint;
    Fat32Bpb Bpb;
    u32 ClusterSize;
    u64 DataStartOffset;
    u64 FatStartOffset;
    Cache FatCache;
};

struct FatFileContext
{
    u64 EntryVolumeOffset;
    Fat32DirEntry DirEntry;
    u32 CurrentCluster;
    u32 Size;
};

PzStatus FatInitFile(
    FatVolumeContext *fat,
    const PzString *filename,
    FatFileContext *out_file);

PzStatus FatReadCluster(
    FatVolumeContext *fat,
    u32 cluster,
    void *buffer, int offset,
    u32 bytes, u32 &read);

PzStatus FatWriteCluster(
    FatVolumeContext *fat, u32 cluster,
    const void *buffer, int offset,
    u32 bytes, u32 &read);

PzStatus FatGetNextCluster(
    FatVolumeContext *fat, u32 cluster, u32 &out);

u32 FatFollowClusterChain(
    FatVolumeContext *fat, u32 &clus_num,
    int max_bytes, bool contiguous);

PzStatus FatLocateEntry(FatVolumeContext *fat,
    const Fat32DirEntry &parent, const PzString *path,
    Fat32DirEntry &out, u64 &part_location);

PzStatus FatReadFile(
    FatVolumeContext *fat,
    FatFileContext *file,
    PzIoControlBlock *iocb,
    void *buffer, u64 offset,
    u32 length, u32 &read);