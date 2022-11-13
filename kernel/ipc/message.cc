#include <ipc/message.hh>
#include <obj/manager.hh>
#include <obj/thread.hh>
#include <sched/scheduler.hh>

PzStatus MsgPostThreadMessage(PzHandle thread, u32 type,
    uptr param1, uptr param2, uptr param3, uptr param4)
{
    PzThreadObject *obj_thread;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_THREAD, nullptr, thread, (ObPointer *)&obj_thread))
        return STATUS_INVALID_HANDLE;

    ObAcquireObject(obj_thread);
    obj_thread->MessageQueue.Add(PzMessage { type, param1, param2, param3, param4 });
    ObReleaseObject(obj_thread);

    ObDereferenceObject(obj_thread);
    return STATUS_SUCCESS;
}

PzStatus MsgPeekThreadMessage(PzMessage *out_message)
{
    PzThreadObject *thread = PsGetCurrentThread();
    ObAcquireObject(thread);

    if (thread->MessageQueue.Length == 0) {
        ObReleaseObject(thread);
        return STATUS_NO_DATA;
    }

    *out_message = thread->MessageQueue.First->Value;

    ObReleaseObject(thread);
    return STATUS_SUCCESS;
}

PzStatus MsgReceiveThreadMessage(PzMessage *out_message)
{
    PzThreadObject *thread = PsGetCurrentThread();
    ObAcquireObject(thread);

    if (thread->MessageQueue.Length == 0) {
        ObReleaseObject(thread);
        return STATUS_NO_DATA;
    }

    *out_message = thread->MessageQueue.First->Value;
    thread->MessageQueue.Remove(thread->MessageQueue.First);

    ObReleaseObject(thread);
    return STATUS_SUCCESS;
}