#include <defs.hh>
#include <obj/thread.hh>

#define CONSOLE_HOST_ALLOC_CONSOLE 3
#define CONSOLE_HOST_WRITE_CONSOLE 4
#define CONSOLE_HOST_READ_CONSOLE  5

extern PzSpinlock ConHostRegSpinlock;
extern PzThreadObject *ConHostThread;
extern PzHandle ConHostEvent;

PZ_KERNEL_EXPORT PzStatus ConAllocateConsole();
PZ_KERNEL_EXPORT PzStatus ConRegisterConsoleHost();
PZ_KERNEL_EXPORT PzStatus ConUnregisterConsoleHost();