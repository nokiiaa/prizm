#include <window/wnd.hh>
#include <lib/util.hh>
#include <sched/scheduler.hh>
#include <obj/directory.hh>
#include <debug.hh>

PzSpinlock WndServerRegSpinlock;
PzThreadObject *WndServerThread = nullptr;

PzStatus WndCreateWindow(
    PzHandle *handle, PzHandle parent, const PzString *title,
    int x, int y, int z,
    u32 width, u32 height,
    int style)
{
    PzWindowObject *obj_parent = nullptr, *obj_window;

    if (parent && !ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, parent, (ObPointer*)&obj_parent))
        return STATUS_INVALID_HANDLE;

    if (WndServerThread && !parent)
        ObAcquireObject(WndServerThread);

    ObCreateUnnamedObject(ObGetObjectDirectory(PZ_OBJECT_WINDOW),
        (ObPointer *)&obj_window, PZ_OBJECT_WINDOW, 0, false);

    ObAcquireObject(obj_window);
    obj_window->Parent = obj_parent;
    obj_window->Title = PzDuplicateString(title);
    obj_window->Parameters[WINDOW_WIDTH]  = width;
    obj_window->Parameters[WINDOW_HEIGHT] = height;
    obj_window->Parameters[WINDOW_POS_X]  = x;
    obj_window->Parameters[WINDOW_POS_Y]  = y;
    obj_window->Parameters[WINDOW_POS_Z]  = z;
    obj_window->Parameters[WINDOW_STYLE]  = style;
    ObReleaseObject(obj_window);

    if (obj_parent) {
        PzAcquireSpinlock(&obj_parent->Children.Spinlock);
        obj_parent->Children.Add(obj_window);
        PzReleaseSpinlock(&obj_parent->Children.Spinlock);
    }

    if (WndServerThread && !parent) {
        PzHandle server_handle;
        ObCreateHandle(WndServerThread->ParentProcess, 0, &server_handle, obj_window);

        WndServerThread->MessageQueue.Add(
            PzMessage{ WINDOW_SERVER_CREATE_WINDOW, (uptr)server_handle, 0, 0 });

        ObReleaseObject(WndServerThread);
    }

    ObCreateHandle(nullptr, 0, handle, obj_window);
    return STATUS_SUCCESS;
}

void EnumRecursive(LinkedList<PzWindowObject*> *list,
    u32 *max_count, PzHandle *handles, int count, bool top_only, bool recurse)
{
    PzAcquireSpinlock(&list->Spinlock);

    ENUM_LIST(window, *list) {
        auto *obj_window = (PzWindowObject *)window->Value;

        if (top_only && obj_window->Parent)
            continue;

        if (handles) {
            if (count < *max_count)
                ObCreateHandle(nullptr, 0, handles + count++, obj_window);
            else {
                PzReleaseSpinlock(&list->Spinlock);
                return;
            }
        }
        else
            (*max_count)++;

        if (recurse)
            EnumRecursive(&obj_window->Children, max_count, handles, count, false, true);
    }

    PzReleaseSpinlock(&list->Spinlock);
}

PzStatus WndEnumerateChildWindows(PzHandle window, u32 *max_count, PzHandle *handles, bool recursive)
{
    PzWindowObject *obj_window = nullptr;

    if (window && !ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, window, (ObPointer *)&obj_window))
        return STATUS_INVALID_HANDLE;

    int count = 0;

    if (obj_window == nullptr) {
        PzDirectoryObject *dir_window = ObGetObjectDirectory(PZ_OBJECT_WINDOW);
        EnumRecursive((LinkedList<PzWindowObject*>*)&dir_window->Objects,
            max_count, handles, 0, true, recursive);
    }
    else
        EnumRecursive(&obj_window->Children,
            max_count, handles, 0, false, recursive);

    ObDereferenceObject(obj_window);
    return STATUS_SUCCESS;
}

PzStatus WndGetWindowTitle(PzHandle window, char *out_title, u32 *max_size)
{
    PzWindowObject *obj_window;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, window, (ObPointer *)&obj_window))
        return STATUS_INVALID_HANDLE;

    ObAcquireObject(obj_window);

    if (out_title) {
        u32 size = Min(*max_size, (u32)(obj_window->Title->Size + 1));
        for (int i = 0; i < size; i++)
            out_title[i] = obj_window->Title->Buffer[i];
    }
    else
        *max_size = obj_window->Title->Size + 1;

    ObReleaseObject(obj_window);
    ObDereferenceObject(obj_window);

    return STATUS_SUCCESS;
}

PzStatus WndSetWindowTitle(PzHandle window, const PzString *out_title)
{
    PzWindowObject *obj_window;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, window, (ObPointer *)&obj_window))
        return STATUS_INVALID_HANDLE;

    ObAcquireObject(obj_window);
    obj_window->Title = PzDuplicateString(out_title);
    ObReleaseObject(obj_window);

    ObDereferenceObject(obj_window);
    return STATUS_SUCCESS;
}

PzStatus WndGetWindowBuffer(PzHandle window, int index, GfxHandle *handle)
{
    if (index != 0 && index != 1)
        return STATUS_INVALID_ARGUMENT;

    PzWindowObject *obj_window;
    if (!ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, window, (ObPointer *)&obj_window))
        return STATUS_INVALID_HANDLE;

    ObAcquireObject(obj_window);

    if (index == 0 && !obj_window->FrontBuffer ||
        index == 1 && !obj_window->BackBuffer) {
        ObReleaseObject(obj_window);
        ObDereferenceObject(obj_window);
        return STATUS_INVALID_ARGUMENT;
    }

    ObCreateHandle(nullptr, 0, handle,
        index == 0 ? obj_window->FrontBuffer : obj_window->BackBuffer);

    ObReleaseObject(obj_window);
    ObDereferenceObject(obj_window);

    return STATUS_SUCCESS;
}

PzStatus WndSetWindowBuffer(PzHandle window, int index, GfxHandle handle)
{
    if (index != 0 && index != 1)
        return STATUS_INVALID_ARGUMENT;

    PzWindowObject *obj_window;
    PzGraphicsObject *gfx;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, window, (ObPointer *)&obj_window))
        return STATUS_INVALID_HANDLE;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_GFX, nullptr, handle, (ObPointer *)&gfx)) {
        ObDereferenceObject(obj_window);
        return STATUS_INVALID_HANDLE;
    }

    ObAcquireObject(obj_window);
    if (index == 0)
        obj_window->FrontBuffer = gfx;
    else
        obj_window->BackBuffer = gfx;
    ObReleaseObject(obj_window);

    ObDereferenceObject(obj_window);
    return STATUS_SUCCESS;
}

PzStatus WndSwapBuffers(PzHandle window)
{
    PzWindowObject *obj_window;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, window, (ObPointer *)&obj_window))
        return STATUS_INVALID_HANDLE;

    ObAcquireObject(obj_window);
    Swap(obj_window->FrontBuffer, obj_window->BackBuffer);
    ObReleaseObject(obj_window);

    ObDereferenceObject(obj_window);
    return STATUS_SUCCESS;
}

PzStatus WndGetWindowParameter(PzHandle window, int index, uptr *value)
{
    if (index < 0 || index > WINDOW_MAX_PARAM)
        return STATUS_INVALID_ARGUMENT;

    PzWindowObject *obj_window;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, window, (ObPointer *)&obj_window))
        return STATUS_INVALID_HANDLE;

    ObAcquireObject(obj_window);
    *value = obj_window->Parameters[index];
    ObReleaseObject(obj_window);

    ObDereferenceObject(obj_window);
    return STATUS_SUCCESS;
}

PzStatus WndSetWindowParameter(PzHandle window, int index, uptr value)
{
    if (index < 0 || index > WINDOW_MAX_PARAM)
        return STATUS_INVALID_ARGUMENT;

    PzWindowObject *obj_window;
    if (!ObReferenceObjectByHandle(PZ_OBJECT_WINDOW, nullptr, window, (ObPointer *)&obj_window))
        return STATUS_INVALID_HANDLE;

    ObAcquireObject(obj_window);
    obj_window->Parameters[index] = value;
    ObReleaseObject(obj_window);

    ObDereferenceObject(obj_window);
    return STATUS_SUCCESS;
}

PzStatus WndRegisterWindowServer()
{
    PzAcquireSpinlock(&WndServerRegSpinlock);

    if (WndServerThread) {
        PzReleaseSpinlock(&WndServerRegSpinlock);
        return STATUS_ACCESS_DENIED;
    }

    ObReferenceObject(WndServerThread = PsGetCurrentThread());
    /* Open handles to all currently existing windows and send them to the new window server */
    ObAcquireObject(WndServerThread);

    PzDirectoryObject *dir_window = ObGetObjectDirectory(PZ_OBJECT_WINDOW);

    ENUM_LIST(window, dir_window->Objects) {
        PzHandle server_handle;
        auto *obj_window = (PzWindowObject*)window->Value;

        if (obj_window->Parent == nullptr) {
            ObAcquireObject(obj_window);
            ObCreateHandle(WndServerThread->ParentProcess, 0, &server_handle, obj_window);
            WndServerThread->MessageQueue.Add(
                PzMessage{ WINDOW_SERVER_CREATE_WINDOW, (uptr)server_handle, 0, 0 });
            ObReleaseObject(obj_window);
        }
    }

    ObReleaseObject(WndServerThread);

    PzReleaseSpinlock(&WndServerRegSpinlock);
    return STATUS_SUCCESS;
}

PzStatus WndUnregisterWindowServer()
{
    PzAcquireSpinlock(&WndServerRegSpinlock);

    if (PsGetCurrentThread() == WndServerThread) {
        ObDereferenceObject(WndServerThread);
        WndServerThread = nullptr;
        PzReleaseSpinlock(&WndServerRegSpinlock);
        return STATUS_SUCCESS;
    }

    PzReleaseSpinlock(&WndServerRegSpinlock);
    return STATUS_ACCESS_DENIED;
}