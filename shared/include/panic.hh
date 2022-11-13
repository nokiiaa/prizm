#include <defs.hh>
#include <core.hh>

#define PANIC_REASON_FAILED_INIT_MOUNT   ((u32)1)
#define PANIC_REASON_FAILED_INIT_DRIVER  ((u32)2)
#define PANIC_REASON_FAILED_INIT_PROCESS ((u32)3)
#define PANIC_REASON_FAILED_INIT_ACPI    ((u32)4)
#define PANIC_REASON_OBJECT_MANAGER      ((u32)5)
#define PANIC_REASON_CPU_EXCEPTION       ((u32)6)
#define PANIC_REASON_INVALID_IRQL        ((u32)7)

PZ_KERNEL_EXPORT void PzPanic(CpuInterruptState *state, u32 reason, const char *error, ...);