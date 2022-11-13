#pragma once
#include <defs.hh>

#pragma pack(push, 1)
struct RsdpDescriptor
{
    u64 Signature;
    u8 Checksum;
    char OEMID[6];
    u8 Revision;
    u32 RsdtAddress;

    u32 Length;
    u64 XsdtAddress;
    u8 ExtendedChecksum;
    u8 _[3];
};

struct IsdtHeader
{
    u32 Signature;
    u32 Length;
    u8 Revision;
    u8 Checksum;
    char OEMID[6];
    char OEMTableID[8];
    u32 OEMRevision;
    u32 CreatorID;
    u32 CreatorRevision;
};
#pragma pack(pop)

void AcpiInitialize();
IsdtHeader *AcpiFindTable(u32 signature);
void AcpiInitializeTables();
IsdtHeader *AcpiMapTable(uptr isdt);
bool AcpiUnmapTable(IsdtHeader *isdt);