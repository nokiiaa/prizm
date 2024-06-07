#pragma once 

#include <obj/apipe.hh>

#define APIPE_BUFSIZE_DEFAULT 16384
#define APIPE_BUFSIZE_MAX     65536

PZ_KERNEL_EXPORT PzStatus PzCreatePipe(
    PzProcessObject *readProcess, PzProcessObject *writeProcess,
    PzHandle *read, PzHandle *write, int buffer_cap);
PzStatus PziReadPipe(PzAnonymousPipeObject *pipe, void *data, u32 bytes);
PzStatus PziWritePipe(PzAnonymousPipeObject *pipe, const void *data, u32 bytes);