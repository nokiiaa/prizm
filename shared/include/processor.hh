#include <lib/list.hh>
#include <sched/scheduler.hh>

#define PASSIVE_LEVEL 0 
#define DISPATCH_LEVEL 1

struct SchedulerQueue {
    LinkedList<PzThreadObject *> ThreadList;
    LinkedList<PzTimerObject *> ActiveTimers;
    int NumberOfActiveThreads;
    bool SoftwareInducedTick;
    PzThreadContext FsSpace;
};

struct PzProcessor {
    volatile int IntLevel;
    LinkedList<uptr> AddressSpaceStack;
    SchedulerQueue Queue;
};

PZ_KERNEL_EXPORT PzProcessor *PzGetCurrentProcessor();
PZ_KERNEL_EXPORT int PzRaiseIrql(int new_irql);
PZ_KERNEL_EXPORT int PzGetCurrentIrql();
PZ_KERNEL_EXPORT int PzLowerIrql(int new_irql);