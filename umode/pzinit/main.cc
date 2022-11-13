#include <pzapi.h>
#include <list.h>

#define printf PzPutStringFormatted

extern "C" PzStatus PzProcessStartup(void *base)
{
    printf("pzinit.exe has started\r\n");

    static PzString init_run_name = PZ_CONST_STRING("SystemDir\\InitRun");

    PzIoStatusBlock iosb;
    PzHandle init_run_handle;

    if (PzStatus cf = PzCreateFile(&init_run_handle, &init_run_name, &iosb, ACCESS_READ, OPEN_EXISTING)) {
        printf("PzCreateFile failed %i\r\n", cf);
        return cf;
    }

    PzFileInformationBasic basic;

    if (PzStatus query = PzQueryInformationFile(init_run_handle, &iosb, FILE_INFORMATION_BASIC,
        &basic, sizeof(PzFileInformationBasic))) {
        printf("PzQueryInformationFile failed %i\r\n", query);
        return query;
    }

    int init_run_size = basic.Size;

    char *init_run_string = nullptr;
    PzAllocateVirtualMemory(CPROC_HANDLE, (void**)&init_run_string, init_run_size + 1, PAGE_READWRITE);

    if (PzStatus rf = PzReadFile(init_run_handle, init_run_string, &iosb, init_run_size, 0)) {
        printf("PzReadFile failed %i\r\n", rf);
        return rf;
    }

    init_run_string[init_run_size] = 0;

    char *current_filename;

    while (*init_run_string) {
        u32 size = 0;
        current_filename = init_run_string;

        while (*init_run_string && *init_run_string != '\r' && *init_run_string != '\n') {
            init_run_string++;
            size++;
        }

        if (size == 0) {
            init_run_string++;
            continue;
        }

        *init_run_string++ = 0;

        PzString path_str = {size, current_filename};
        PzString name_str = {size, current_filename};

        char *sep_ptr = name_str.Buffer;

        do {
            if (*sep_ptr == '\\') {
                name_str.Buffer = sep_ptr + 1;
                name_str.Size = current_filename + size - name_str.Buffer;
            }
        } while (*sep_ptr++);

        PzProcessCreationParams params = {
            &name_str,
            &path_str,
            false,
            -1,
            -1,
            -1
        };

        PzHandle proc_handle;

        printf("Creating process %s...\r\n", path_str.Buffer);

        if (PzStatus cp = PzCreateProcess(&proc_handle, &params)) {
            printf("Creating process %s failed with code %i\r\n", path_str.Buffer, cp);
            return cp;
        }
    }

    return STATUS_SUCCESS;
}