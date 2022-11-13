#include <pzapi.h>
#include "math.hh"

extern "C" PzStatus PzProcessStartup(void *base)
{
    PzAllocateConsole();

    for (;;)
    for (int i = 0; i < 100000; i++)
        printf("%05i\r\n", i);

    return STATUS_SUCCESS;
}