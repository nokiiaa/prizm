#include <ldr/peldr.hh>
#include <lib/string.hh>
#include <obj/manager.hh>
#include <obj/process.hh>
#include <sched/scheduler.hh>
#include <debug.hh>

#pragma pack(push, 1)
struct MzHeader {
    u16 Signature;
    u8 WhoCares[0x3a];
    u32 PeSigOffset;
};

#define IMAGE_FILE_MACHINE_I386 0x14c
#define IMAGE_FILE_RELOCS_STRIPPED         (1u <<  0)
#define IMAGE_FILE_EXECUTABLE_IMAGE        (1u <<  1)
#define IMAGE_FILE_LINE_NUMS_STRIPPED      (1u <<  2)
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED     (1u <<  3)
#define IMAGE_FILE_AGGRESSIVE_WS_TRIM      (1u <<  4)
#define IMAGE_FILE_LARGE_ADDRESS_AWARE     (1u <<  5)
#define IMAGE_FILE_BYTES_REVERSED_LO       (1u <<  6)
#define IMAGE_FILE_32BIT_MACHINE           (1u <<  7)
#define IMAGE_FILE_DEBUG_STRIPPED          (1u <<  8)
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP (1u <<  9)
#define IMAGE_FILE_NET_RUN_FROM_SWAP       (1u << 10)
#define IMAGE_FILE_SYSTEM                  (1u << 11)
#define IMAGE_FILE_DLL                     (1u << 12)
#define IMAGE_FILE_UP_SYSTEM_ONLY          (1u << 13)
#define IMAGE_FILE_BYTES_REVERSED_HI       (1u << 14)


struct CoffHeader {
    u32 PeSignature;
    u16 Machine;
    u16 NumberOfSections;
    u32 TimeDateStamp;
    u32 PointerToSymbolTable;
    u32 NumberOfSymbols;
    u16 SizeOfOptionalHeader;
    u16 Characteristics;
};

struct ImageDataDirectory {
    u32 VirtualAddress;
    u32 Size;
};

#define IMAGE_SUBSYSTEM_UNKNOWN                  0
#define IMAGE_SUBSYSTEM_NATIVE                   1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI              2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI              3
#define IMAGE_SUBSYSTEM_OS2_CUI                  5
#define IMAGE_SUBSYSTEM_POSIX_CUI                7
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS           8
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI           9
#define IMAGE_SUBSYSTEM_EFI_APPLICATION          10
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  11
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER       12
#define IMAGE_SUBSYSTEM_EFI_ROM                  13
#define IMAGE_SUBSYSTEM_XBOX                     14
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION 16

#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE          0x0040u
#define IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY       0x0080u
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT             0x0100u
#define IMAGE_DLLCHARACTERISTICS_NO_ISOLATION          0x0200u
#define IMAGE_DLLCHARACTERISTICS_NO_SEH                0x0400u
#define IMAGE_DLLCHARACTERISTICS_NO_BIND               0x0800u
#define IMAGE_DLLCHARACTERISTICS_APPCONTAINER          0x1000u
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER            0x2000u
#define IMAGE_DLLCHARACTERISTICS_GUARD_CF              0x4000u
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE 0x8000u

#define IMAGE_SCN_TYPE_NO_PAD            0x00000008u
#define IMAGE_SCN_CNT_CODE               0x00000020u
#define IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040u
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080u
#define IMAGE_SCN_LNK_OTHER              0x00000100u
#define IMAGE_SCN_LNK_INFO               0x00000200u
#define IMAGE_SCN_LNK_REMOVE             0x00000800u
#define IMAGE_SCN_LNK_COMDAT             0x00001000u
#define IMAGE_SCN_GPREL                  0x00008000u
#define IMAGE_SCN_MEM_PURGEABLE          0x00020000u
#define IMAGE_SCN_MEM_16BIT              0x00020000u
#define IMAGE_SCN_MEM_LOCKED             0x00040000u
#define IMAGE_SCN_MEM_PRELOAD            0x00080000u
#define IMAGE_SCN_ALIGN_1BYTES           0x00100000u
#define IMAGE_SCN_ALIGN_2BYTES           0x00200000u
#define IMAGE_SCN_ALIGN_4BYTES           0x00300000u
#define IMAGE_SCN_ALIGN_8BYTES           0x00400000u
#define IMAGE_SCN_ALIGN_16BYTES          0x00500000u
#define IMAGE_SCN_ALIGN_32BYTES          0x00600000u
#define IMAGE_SCN_ALIGN_64BYTES          0x00700000u
#define IMAGE_SCN_ALIGN_128BYTES         0x00800000u
#define IMAGE_SCN_ALIGN_256BYTES         0x00900000u
#define IMAGE_SCN_ALIGN_512BYTES         0x00A00000u
#define IMAGE_SCN_ALIGN_1024BYTES        0x00B00000u
#define IMAGE_SCN_ALIGN_2048BYTES        0x00C00000u
#define IMAGE_SCN_ALIGN_4096BYTES        0x00D00000u
#define IMAGE_SCN_ALIGN_8192BYTES        0x00E00000u
#define IMAGE_SCN_LNK_NRELOC_OVFL        0x01000000u
#define IMAGE_SCN_MEM_DISCARDABLE        0x02000000u
#define IMAGE_SCN_MEM_NOT_CACHED         0x04000000u
#define IMAGE_SCN_MEM_NOT_PAGED          0x08000000u
#define IMAGE_SCN_MEM_SHARED             0x10000000u
#define IMAGE_SCN_MEM_EXECUTE            0x20000000u
#define IMAGE_SCN_MEM_READ               0x40000000u
#define IMAGE_SCN_MEM_WRITE              0x80000000u

struct OptionalHeader {
    u16 Magic;
    u8 MajorLinkerVersion;
    u8 MinorLinkerVersion;
    u32 SizeOfCode;
    u32 SizeOfInitializedData;
    u32 SizeOfUninitializedData;
    u32 AddressOfEntryPoint;
    u32 BaseOfCode;
    u32 BaseOfData;

    u32 ImageBase;
    u32 SectionAlignment;
    u32 FileAlignment;
    u16 MajorOperatingSystemVersion;
    u16 MinorOperatingSystemVersion;
    u16 MajorImageVersion;
    u16 MinorImageVersion;
    u16 MajorSubsystemVersion;
    u16 MinorSubsystemVersion;
    u32 Win32VersionValue;
    u32 SizeOfImage;
    u32 SizeOfHeaders;
    u32 CheckSum;
    u16 Subsystem;
    u16 DllCharacteristics;
    u32 SizeOfStackReserve;
    u32 SizeOfStackCommit;
    u32 SizeOfHeapReserve;
    u32 SizeOfHeapCommit;
    u32 LoaderFlags;
    u32 NumberOfRvaAndSizes;

    ImageDataDirectory ExportTable;
    ImageDataDirectory ImportTable;
    ImageDataDirectory ResourceTable;
    ImageDataDirectory ExceptionTable;
    ImageDataDirectory CertificateTable;
    ImageDataDirectory BaseRelocTable;
    ImageDataDirectory DebugTable;
    ImageDataDirectory Architecture;
    ImageDataDirectory GlobalPtr;
    ImageDataDirectory TlsTable;
    ImageDataDirectory LoadConfigTable;
    ImageDataDirectory BoundImportTable;
    ImageDataDirectory ImportAddrTable;
    ImageDataDirectory DelayImportDesc;
    ImageDataDirectory ClrRuntimeHeader;
    ImageDataDirectory Reserved;
};

struct SectionHeader
{
    char Name[8];
    u32 VirtualSize;
    u32 VirtualAddress;
    u32 SizeOfRawData;
    u32 PointerToRawData;
    u32 PointerToRelocations;
    u32 PointerToLinenumbers;
    u16 NumberOfRelocations;
    u16 NumberOfLinenumbers;
    u32 Characteristics;
};

#define IMAGE_REL_BASED_ABSOLUTE       0
#define IMAGE_REL_BASED_HIGH           1
#define IMAGE_REL_BASED_LOW            2
#define IMAGE_REL_BASED_HIGHLOW        3
#define IMAGE_REL_BASED_HIGHADJ        4
#define IMAGE_REL_BASED_MIPS_JMPADDR   5
#define IMAGE_REL_BASED_ARM_MOV32      5
#define IMAGE_REL_BASED_RISCV_HIGH20   5
#define IMAGE_REL_BASED_THUMB_MOV32    7
#define IMAGE_REL_BASED_RISCV_LOW12I   7
#define IMAGE_REL_BASED_RISCV_LOW12S   8
#define IMAGE_REL_BASED_MIPS_JMPADDR16 9
#define IMAGE_REL_BASED_DIR64          10

struct ImageBaseRelocation {
    u32 VirtualAddress;
    u32 SizeOfBlock;
    u16 Data[0];
};

struct ExportDirectoryTable {
    u32 ExportFlags;
    u32 TimeDateStamp;
    u16 MajorVersion;
    u16 MinorVerison;
    u32 NameRva;
    u32 OrdinalBase;
    u32 ExportAddrTableEntries;
    u32 ExportNamePointerCount;
    u32 ExportAddrTableRva;
    u32 ExportNamePointerRva;
    u32 ExportOrdinalTableRva;
};

union ExportAddressEntry {
    u32 ExportRva;
    u32 ForwarderRva;
};

struct ImportDirectoryEntry {
    u32 ImageLookupTableRva;
    u32 TimeDateStamp;
    u32 ForwarderChain;
    u32 NameRva;
    u32 ImportAddrTableRva;
};

union ImportLookupEntry {
    u32 ImportByOrdinal;
    u16 OrdinalNumber;
    u32 HintNameTableRva;
    /* Assigned when struct is transformed into a ImportAddressEntry during binding */
    uptr ResolvedVa;
};

typedef ImportLookupEntry ImportAddressEntry;

struct HintNameTable {
    u16 Hint;
    char ImportName[0];
};

#pragma pack(pop)

#include <mm/virtual.hh>
#include <lib/util.hh>

//#define LDR_DEBUG_PRINT(...) DbgPrintStr("[PE loader] " __VA_ARGS__);
#define LDR_DEBUG_PRINT(...)

bool LdriCreateModule(
    PzProcessObject *process, PzHandle *handle, const PzString *name,
    void *base, int (*entry_point)(void *), u32 size);

void LdrInitializeLoader(KernelBootInfo *info)
{
    static const PzString KernelModuleName = PZ_CONST_STRING("pzkernel.exe");
    PzHandle handle;
    LdriCreateModule(PZ_KPROC,
        &handle, &KernelModuleName, (void *)info->KernelVirtualBase, nullptr, info->KernelSize);
}

bool LdriCreateModule(
    PzProcessObject *process, PzHandle *handle, const PzString *name,
    void *base, int (*entry_point)(void *), u32 size)
{
    auto *mod = process->LoadedModules.Add(nullptr);

    if (!ObCreateUnnamedObject(
        ObGetObjectDirectory(PZ_OBJECT_MODULE), (ObPointer *)&mod->Value,
        PZ_OBJECT_MODULE, 0, false))
        return false;

    mod->Value->BaseAddress = base;
    mod->Value->EntryPointAddress = entry_point;
    mod->Value->ImageSize = size;
    mod->Value->ModuleName = name;
    mod->Value->AssociatedProcess = process;

    if (!ObCreateHandle(PZ_KPROC, 0, handle, mod->Value)) {
        process->LoadedModules.Remove(mod);
        ObDereferenceObject(mod->Value);
        return false;
    }

    ObReferenceObject(process);
    return true;
}

#include <io/manager.hh>
#include <core.hh>
#include <mm/virtual.hh>

PzStatus LdrLoadImageFile(
    PzHandle process,
    const PzString *mod_path,
    const PzString *image_name,
    PzHandle *mod_handle)
{
    PzHandle handle;
    PzIoStatusBlock status;

    if (PzStatus cf_status = PzCreateFile(true, &handle, mod_path,
        &status, ACCESS_READ, OPEN_EXISTING))
        return cf_status;

    /* Retrieve the file's length so we can allocate a buffer of sufficient size for it. */
    PzFileInformationBasic basic;

    if (PzStatus q_status = PzQueryInformationFile(handle, &status,
        FILE_INFORMATION_BASIC,
        &basic, sizeof(PzFileInformationBasic))) {
        PzCloseHandle(handle);
        return q_status;
    }

    /* Allocate a buffer for the file and read it */
    void *buffer = MmVirtualAllocateMemory(nullptr, basic.Size, PAGE_READWRITE, nullptr);

    if (!buffer) {
        PzCloseHandle(handle);
        return STATUS_ALLOCATION_FAILED;
    }

    if (PzStatus rd_status = PzReadFile(handle, buffer, &status, basic.Size, nullptr)) {
        PzCloseHandle(handle);
        MmVirtualFreeMemory(buffer, basic.Size);
        return rd_status;
    }

    PzProcessObject *proc;
    ObReferenceObjectByHandle(PZ_OBJECT_PROCESS, nullptr, process, (ObPointer *)&proc);

    if (!LdrLoadImage(proc, image_name->Buffer, buffer, basic.Size, mod_handle)) {
        ObDereferenceObject(proc);
        MmVirtualFreeMemory(buffer, basic.Size);
        PzCloseHandle(handle);
        return -1;
    }

    /* Free buffer and close executable file handle */
    ObDereferenceObject(proc);
    MmVirtualFreeMemory(buffer, basic.Size);
    PzCloseHandle(handle);

    return STATUS_SUCCESS;
}

uptr LdriGetSymbolAddress(
    PzModuleObject *mod,
    const char *symbol_name)
{

#define RVA_TO_VA_DLL(addr) ((uptr)dll_bytes + (addr))

    PzPushAddressSpace(mod->AssociatedProcess->Cr3);
    u8 *dll_bytes = (u8 *)mod->BaseAddress;
    MzHeader *dll_mz_header = (MzHeader *)mod->BaseAddress;
    OptionalHeader *dll_opt_header = OPTIONAL_HEADER_START(dll_bytes);

    auto *edt_base = (ExportDirectoryTable *)
        RVA_TO_VA_DLL(dll_opt_header->ExportTable.VirtualAddress);

    u32 *export_name_table = (u32 *)RVA_TO_VA_DLL(edt_base->ExportNamePointerRva);
    u16 *export_ordinal_table = (u16 *)RVA_TO_VA_DLL(edt_base->ExportOrdinalTableRva);
    u32 *export_addr_table = (u32 *)RVA_TO_VA_DLL(edt_base->ExportAddrTableRva);

    for (int i = 0; i < edt_base->ExportNamePointerCount; i++) {
        char *export_name = (char *)RVA_TO_VA_DLL(export_name_table[i]);

        /* A match has been found! */
        if (Utf8CompareRawStrings(export_name, -1, symbol_name, -1) == 0) {
            uptr ret = export_addr_table[export_ordinal_table[i]] + (uptr)dll_bytes;
            PzPopAddressSpace();
            return ret;
        }
    }
    PzPopAddressSpace();
    return 0;

#undef RVA_TO_VA_DLL
}

uptr LdrGetSymbolAddress(
    PzHandle mod_handle,
    const char *symbol_name)
{
    PzModuleObject *mod;
    ObReferenceObjectByHandle(PZ_OBJECT_MODULE, nullptr, mod_handle, (ObPointer*)&mod);
    uptr result = LdriGetSymbolAddress(mod, symbol_name);
    ObDereferenceObject(mod);
    return result;
}

void *LdrLoadImage(
    PzProcessObject *process,
    const char *mod_name,
    void *start_address, u32 img_size,
    PzHandle *handle)
{
    DbgPrintStr("LdrLoadImage\r\n");
    void *real_base = nullptr;
    bool need_relocation = false;
    bool kernel = process == PZ_KPROC;
    u8 *bytes = (u8 *)start_address;
    auto *mz_header = (MzHeader *)start_address;
    LDR_DEBUG_PRINT("Loading module image @ %p\r\nVerifying PE headers...\r\n", start_address);

    if (mz_header->Signature != 0x5A4Du) {
        LDR_DEBUG_PRINT("Invalid image: does not start with 'MZ'\r\n");
        return nullptr;
    }

    auto *coff_header = (CoffHeader *)(bytes + mz_header->PeSigOffset);
    auto *opt_header = (OptionalHeader *)(coff_header + 1);
    auto *sections = (SectionHeader *)((u8 *)opt_header + coff_header->SizeOfOptionalHeader);

    if (coff_header->PeSignature != 0x00004550u) {
        LDR_DEBUG_PRINT("Invalid image: 'PE\\0\\0' signature not present\r\n");
        goto fail;
    }

    if (coff_header->Machine != IMAGE_FILE_MACHINE_I386) {
        LDR_DEBUG_PRINT("Invalid image: Target machine isn't i386\r\n");
        goto fail;
    }

    /* Try to allocate the image at the specified image base first */
    if (kernel) {
        real_base = MmVirtualAllocateMemory(
            (void *)opt_header->ImageBase,
            opt_header->SizeOfImage,
            PAGE_READWRITE, nullptr);
    }
    else {
        /* Switch the page table to access this process's address space */
        PzPushAddressSpace(process->Cr3);

        real_base = MmiVirtualAllocateUserMemory(
            process, (void *)opt_header->ImageBase,
            opt_header->SizeOfImage, PAGE_READWRITE);
    }

    /* If allocation failed, let the allocator assign an address to the image */
    if (!real_base) {
        if (kernel) {
            real_base = MmVirtualAllocateMemory(
                nullptr, opt_header->SizeOfImage,
                PAGE_READWRITE, nullptr);
        }
        else {
            real_base = MmiVirtualAllocateUserMemory(
                process, nullptr,
                opt_header->SizeOfImage,
                PAGE_READWRITE);
        }

        need_relocation = true;

        if (!real_base) {
            LDR_DEBUG_PRINT("Failed to allocate space for driver image\r\n");
            goto fail;
        }
    }

    LDR_DEBUG_PRINT("Module image reserved at %p. Relocation needed=%i\r\n", real_base, need_relocation);
    #define RVA_TO_VA(addr) ((uptr)real_base + (addr))

    /* Copy the headers over to the image */
    MemCopy(real_base, start_address, opt_header->SizeOfHeaders);

    /* Load the image's sections according to the section headers */
    for (int i = 0; i < coff_header->NumberOfSections; i++) {
        int vsize = sections[i].VirtualSize;
        int rawsize = sections[i].SizeOfRawData;
        void *vstart = (void *)RVA_TO_VA(sections[i].VirtualAddress);
        void *sec_data = bytes + sections[i].PointerToRawData;

        /* Prevent writes above userspace */
        if (!kernel &&
            (uptr(vstart) >= KERNEL_SPACE_START        ||
             uptr(vstart) + vsize > KERNEL_SPACE_START))
            goto fail;

        char section_name[8 + 1] = { 0 };
        for (int j = 0; j < 8; j++)
            section_name[j] = sections[i].Name[j];

        LDR_DEBUG_PRINT("Loading section %s\r\n", section_name);

        if (rawsize < vsize) {
            int padding = vsize - rawsize;
            u8 *pad_start = (u8 *)vstart + sections[i].SizeOfRawData;
            MemCopy(vstart, sec_data, sections[i].SizeOfRawData);
            MemSet(pad_start, 0, padding);
        }
        else
            MemCopy(vstart, sec_data, sections[i].VirtualSize);
    }

    LDR_DEBUG_PRINT("Performing base relocations...\r\n");
    /* Perform base relocations */
    if (need_relocation) {
        u32 reloc_vaddr = opt_header->BaseRelocTable.VirtualAddress;

        LDR_DEBUG_PRINT("Relocation directory: %p %p\r\n",
            opt_header->BaseRelocTable.VirtualAddress,
            opt_header->BaseRelocTable.Size);

        if (!reloc_vaddr)
            goto fail_free;

        auto *reloc = (ImageBaseRelocation *)RVA_TO_VA(reloc_vaddr);
        u8 *reloc_end = (u8 *)reloc + opt_header->BaseRelocTable.Size;
        i32 diff = (uptr)real_base - opt_header->ImageBase;

        while ((u8 *)reloc < reloc_end) {
            u32 bsize = reloc->SizeOfBlock;
            int entries = (bsize - sizeof(ImageBaseRelocation)) / sizeof(u16);

            for (int i = 0; i < entries; i++) {
                u16 *word_at = (u16 *)(
                    (uptr)real_base + reloc->VirtualAddress + (reloc->Data[i] & 0xFFF));

                /* Can't write above userspace */
                if (!kernel && u32(word_at) >= KERNEL_SPACE_START)
                    goto fail;

                u32 *dword_at = (u32 *)word_at;
                u32 next_rva = 0;

                switch (reloc->Data[i] >> 12) {
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;

                case IMAGE_REL_BASED_HIGHLOW:
                    *dword_at += diff;
                    break;

                case IMAGE_REL_BASED_HIGH:
                    *word_at += diff >> 16;
                    break;

                case IMAGE_REL_BASED_LOW:
                    *word_at += diff & 0xFFFF;
                    break;

                case IMAGE_REL_BASED_HIGHADJ:
                    if ((u8 *)(&reloc->Data[0] + i + 1) >= reloc_end)
                        break;

                    next_rva = (uptr)real_base + reloc->VirtualAddress + (reloc->Data[i + 1] & 0xFFF);
                    *word_at = ((*word_at << 16) + next_rva + diff) >> 16;
                    i++;
                    break;
                }
            }
            reloc = (ImageBaseRelocation *)((u8 *)reloc + bsize);
        }
    }

    LDR_DEBUG_PRINT("Performing binding...\r\n");

    /* Perform binding */
    if (opt_header->ImportTable.VirtualAddress) {
        auto *idt_base = (ImportDirectoryEntry *)RVA_TO_VA(opt_header->ImportTable.VirtualAddress);
        LDR_DEBUG_PRINT("Import directory entry @ %p\r\n", idt_base);
        auto *module_list = &process->LoadedModules;

        for (; idt_base->ImageLookupTableRva; idt_base++) {
            auto *ilt_base = (ImportLookupEntry *)RVA_TO_VA(idt_base->ImageLookupTableRva);
            auto *iat_base = (ImportAddressEntry *)RVA_TO_VA(idt_base->ImportAddrTableRva);

            LDR_DEBUG_PRINT(
                "idt_base=%p\r\n"
                "ilt_base=%p\r\n"
                "iat_base=%p\r\n", idt_base, ilt_base, iat_base);

            u32 function_va = 0;
            char *dll_name = (char *)RVA_TO_VA(idt_base->NameRva);
            void *dll_base = nullptr;

            /* Search for the requested module name in the loaded module list */
            LDR_DEBUG_PRINT("Searching for module %s...\r\n", dll_name);

            for (auto *list = module_list->First; list; list = list->Next)
                if (Utf8CompareRawStrings(list->Value->ModuleName->Buffer, -1, dll_name, -1) == 0)
                    dll_base = list->Value->BaseAddress;

            LDR_DEBUG_PRINT("Module found at %p\r\n", dll_base);

            if (!dll_base)
                continue;

            #define RVA_TO_VA_DLL(addr) ((uptr)dll_base + (addr))
            u8 *dll_bytes = (u8 *)dll_base;
            MzHeader *dll_mz_header = (MzHeader *)dll_bytes;
            OptionalHeader *dll_opt_header = OPTIONAL_HEADER_START(dll_base);

            auto *edt_base = (ExportDirectoryTable *)
                RVA_TO_VA_DLL(dll_opt_header->ExportTable.VirtualAddress);
            u32 *export_name_table = (u32 *)RVA_TO_VA_DLL(edt_base->ExportNamePointerRva);
            u16 *export_ordinal_table = (u16 *)RVA_TO_VA_DLL(edt_base->ExportOrdinalTableRva);
            u32 *export_addr_table = (u32 *)RVA_TO_VA_DLL(edt_base->ExportAddrTableRva);

            LDR_DEBUG_PRINT("export_name_table=%p export_ordinal_table=%p export_addr_table=%p\r\n",
                export_name_table, export_ordinal_table, export_addr_table);

            for (; ilt_base->OrdinalNumber; ilt_base++, iat_base++) {

                /* Find the function's RVA by ordinal */
                if (ilt_base->ImportByOrdinal >> 31)
                    function_va = export_addr_table[ilt_base->OrdinalNumber - edt_base->OrdinalBase];
                else {

                    /* Find the name in the export name pointer table of the module we're importing from */
                    auto hint_name = (HintNameTable *)RVA_TO_VA(ilt_base->HintNameTableRva);
                    LDR_DEBUG_PRINT("Searching for symbol %s...\r\n", &hint_name->ImportName[0]);
                    LDR_DEBUG_PRINT("edt_base=%p ExportNamePointerCount=%i\r\n",
                        edt_base,
                        edt_base->ExportNamePointerCount);

                    for (int i = 0; i < edt_base->ExportNamePointerCount; i++) {
                        char *export_name = (char *)RVA_TO_VA_DLL(export_name_table[i]);

                        /* A match has been found! */
                        if (Utf8CompareRawStrings(export_name, -1, &hint_name->ImportName[0], -1) == 0) {
                            function_va = export_addr_table[export_ordinal_table[i]];
                            LDR_DEBUG_PRINT("Symbol %s located; starts at %p\r\n", export_name,
                                RVA_TO_VA_DLL(function_va));
                        }
                    }
                }

                LDR_DEBUG_PRINT("Putting symbol's address at %p\r\n", &iat_base->ResolvedVa);
                iat_base->ResolvedVa = RVA_TO_VA_DLL(function_va);
            }
            #undef RVA_TO_VA_DLL
        }
    }

    LDR_DEBUG_PRINT("Applying section attributes...\r\n");

    /* Apply section attributes */
    for (int i = 0; i < coff_header->NumberOfSections; i++) {
        int vsize = sections[i].VirtualSize;
        int rawsize = sections[i].SizeOfRawData;
        void *vstart = (void *)RVA_TO_VA(sections[i].VirtualAddress);

        u32 flags = PAGE_READ |
            (sections[i].Characteristics & IMAGE_SCN_MEM_WRITE ? PAGE_WRITE : 0) |
            (sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE ? PAGE_EXECUTE : 0);
        LDR_DEBUG_PRINT("Protecting 0x%p-0x%p %08x\r\n", vstart, (u8*)vstart + vsize, flags);

        if (kernel) MmVirtualProtectMemory(vstart, vsize, flags);
        else MmiVirtualProtectUserMemory(process, vstart, vsize, flags);
    }

    {
        char *module_name =
            opt_header->ExportTable.VirtualAddress ?
            (char *)RVA_TO_VA(((ExportDirectoryTable *)
            RVA_TO_VA(opt_header->ExportTable.VirtualAddress))->NameRva) :
            (char *)mod_name;

        u32 module_name_size = StringLength(module_name);

        const PzString name = { module_name_size, module_name };
        auto *entry_point = (int(*)(void *))RVA_TO_VA(opt_header->AddressOfEntryPoint);

        if (!LdriCreateModule(
            process, handle, PzDuplicateString(&name),
            real_base, entry_point, opt_header->SizeOfImage)) {
            goto fail_free;
        }
    }

    #undef RVA_TO_VA

    if (!kernel)
        PzPopAddressSpace();

    LDR_DEBUG_PRINT("Returning, success\r\n");
    return real_base;

fail_free:
    if (real_base) {
        if (kernel)
            MmVirtualFreeMemory(real_base, opt_header->SizeOfImage);
        else {
            MmiVirtualFreeUserMemory(process, real_base, opt_header->SizeOfImage);
            PzPopAddressSpace();
        }
    }

fail:
    if (!kernel)
        PzPopAddressSpace();

    LDR_DEBUG_PRINT("Returning, failure\r\n");
    return nullptr;
}

void LdrUnloadImage(PzModuleObject *module_obj)
{
    PzProcessObject *process = module_obj->AssociatedProcess;

    ENUM_LIST(mod, process->LoadedModules) {
        if (mod->Value->BaseAddress == mod->Value) {
            if (process->IsUserMode)
                MmiVirtualFreeUserMemory(process, mod->Value->BaseAddress, mod->Value->ImageSize);
            else
                MmVirtualFreeMemory(mod->Value->BaseAddress, mod->Value->ImageSize);

            ObDereferenceObject(mod->Value);
            process->LoadedModules.Remove(mod);
            return;
        }
    }
}