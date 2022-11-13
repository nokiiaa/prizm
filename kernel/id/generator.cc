#include <id/generator.hh>
#include <spinlock.hh>

uptr CurrentKernelObjectId  = -7,
     CurrentUserObjectId    =  1,
     CurrentThreadProcessId =  1;

PzSpinlock IdSpinlock;

uptr IdGenerateUniqueId(int id_namespace)
{
    PzAcquireSpinlock(&IdSpinlock);

    uptr id = -1;
    switch (id_namespace) {
    case NAMESPACE_KERNEL_OBJECT:
        id = CurrentKernelObjectId--;
        break;

    case NAMESPACE_USER_OBJECT:
        id = CurrentUserObjectId++;
        break;

    case NAMESPACE_THREAD_PROCESS:
        id = CurrentThreadProcessId++;
        break;
    }

    PzReleaseSpinlock(&IdSpinlock);
    return id;
}