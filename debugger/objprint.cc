#include <objprint.hh>
#include <main.hh>

#define PRINT_OBJECT(table, addr, type) \
    do { \
        auto search = BackcheckMap.find(addr); \
        if (search == BackcheckMap.end()) { \
            auto obj = ObjPrint::Object(addr, type); \
            table->ValueTable(obj); \
            if (obj) \
                BackcheckMap[addr] = obj; \
        } \
        else table->ValueTable(search->second); \
    } while (0)

#define TRY_PRINT(x, y) if (x) y; else table->ValueCStr("???");

std::shared_ptr<ObjView> ObjPrint::Handles(
    const char *name, const std::vector<PzHandleTableEntry>& handles)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    table->ValName("Flags");
    table->ValName("Id");
    table->ValName("ObjectPointer");

    for (auto handle : handles) {
        table->ValueHex(handle.Flags);
        table->ValueInt(handle.Id);
        table->ValueHex(handle.ObjectPointer);
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::APipes(
    const char *name, const std::vector<uptr>& apipes)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 9;
    table->ValName("Address");
    table->ValName("Buffer");
    table->ValName("BufferCapacity");
    table->ValName("BufferSize");
    table->ValName("MutexRead");
    table->ValName("MutexWrite");
    table->ValName("ReadPtr");
    table->ValName("WritePtr");
    table->ValName("SemaphoreRead");
    table->ValName("SemaphoreWrite");

    for (auto apipe : apipes) {
        table->ValueHex(apipe);
        PzAnonymousPipeObject read;

        if (!Link::ReadKernelObject<PzAnonymousPipeObject>(apipe, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            table->ValueHex(read.Buffer);
            table->ValueHex(read.BufferCapacity);
            table->ValueHex(read.BufferSize);
            table->ValueHex(read.MutexRead);
            table->ValueHex(read.MutexWrite);
            table->ValueHex(read.ReadPtr);
            table->ValueHex(read.WritePtr);
            table->ValueHex(read.SemaphoreEmpty);
            table->ValueHex(read.SemaphoreFull);
        }
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::Events(
    const char *name, const std::vector<uptr>& events)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 2;
    table->ValName("Address");
    table->ValName("Type");
    table->ValName("Signaled");

    for (auto event : events) {
        table->ValueHex(event);
        PzEventObject read;

        if (!Link::ReadKernelObject<PzEventObject>(event, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            table->ValueInt(read.Type);
            table->ValueBool(read.Signaled);
        }
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::Mutexes(
    const char *name, const std::vector<uptr>& mutexes)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    table->ValName("Address");
    table->ValName("Signaled");

    for (auto mutex : mutexes) {
        table->ValueHex(mutex);
        PzMutexObject read;

        if (!Link::ReadKernelObject<PzMutexObject>(mutex, &read))
            table->ValueCStr("???");
        else
            table->ValueBool(read.Signaled);
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::Semaphores(
    const char *name, const std::vector<uptr>& semaphores)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    table->ValName("Address");
    table->ValName("Signaled");
    table->ValName("Count");
    for (auto semaphore : semaphores) {
        table->ValueHex(semaphore);
        PzSemaphoreObject read;

        if (!Link::ReadKernelObject<PzSemaphoreObject>(semaphore, &read))
            table->ValueCStr("???"), table->ValueCStr("???");
        else {
            table->ValueBool(read.Signaled);
            table->ValueInt(read.Count);
        }
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::Timers(
    const char *name, const std::vector<uptr>& timers)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 3;
    table->ValName("Address");
    table->ValName("Type");
    table->ValName("Signaled");
    table->ValName("TimeLeft");

    for (auto timer : timers) {
        table->ValueHex(timer);
        PzTimerObject read;

        if (!Link::ReadKernelObject<PzTimerObject>(timer, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            table->ValueInt(read.Type);
            table->ValueBool(read.Signaled);
            table->ValueInt(read.TimeLeft);
        }
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::Iocbs(
    const char *name, const std::vector<uptr>& iocbs)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 5;
    table->ValName("Address");
    table->ValName("Device");
    table->ValName("Filename");
    table->ValName("Irp");
    table->ValName("CurrentOffset");
    table->ValName("Context");

    for (auto iocb : iocbs) {
        table->ValueHex(iocb);
        PzIoControlBlock read;

        if (!Link::ReadKernelObject<PzIoControlBlock>(iocb, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            PRINT_OBJECT(table, read.Device, PZ_OBJECT_DEVICE);
            std::string name;

            TRY_PRINT(Link::ReadKernelString(read.Filename, name), table->ValueStr('"' + name + '"'));
            table->ValueHex(read.Irp);
            table->ValueInt(read.CurrentOffset);
            table->ValueHex(read.Context);
        }
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::Devices(
    const char *name, const std::vector<uptr>& devices)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 6;
    table->ValName("Address");
    table->ValName("Type");
    table->ValName("Flags");
    table->ValName("Context");
    table->ValName("Name");
    table->ValName("DriverObject");
    table->ValName("MajorFunctions");

    for (auto device : devices) {
        table->ValueHex(device);
        PzDeviceObject read;

        if (!Link::ReadKernelObject<PzDeviceObject>(device, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            table->ValueInt(read.Type);
            table->ValueInt(read.Flags);
            table->ValueHex(read.Context);

            std::string name;
            TRY_PRINT(Link::ReadKernelString(read.Name, name), table->ValueStr('"' + name + '"'));
            PRINT_OBJECT(table, read.DriverObject, PZ_OBJECT_DRIVER);

            std::shared_ptr<ObjView> funcs(new ObjView("Show"));
            funcs->ValName("IRP_MJ_CREATE");
            funcs->ValName("IRP_MJ_READ");
            funcs->ValName("IRP_MJ_WRITE");
            funcs->ValName("IRP_MJ_SET_FILE");
            funcs->ValName("IRP_MJ_QUERY_INFO_DIR");
            funcs->ValName("IRP_MJ_QUERY_INFO_FILE");
            funcs->ValName("IRP_MJ_SET_INFO_DIR");
            funcs->ValName("IRP_MJ_SET_INFO_FILE");
            funcs->ValName("IRP_MJ_DEVICE_IOCTL");
            funcs->ValName("IRP_MJ_CLOSE");
            funcs->ValName("IRP_MJ_MOUNT_FS");

            for (int i = 1; i <= IRP_MJ_LAST; i++)
                funcs->ValueHex(read.MajorFunctions[i]);

            table->ValueTable(funcs);
        }
    }
    return table;
}


std::shared_ptr<ObjView> ObjPrint::Drivers(
    const char *name, const std::vector<uptr>& drivers)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 5;

    table->ValName("Address");
    table->ValName("Name");
    table->ValName("AssociatedModule");
    table->ValName("AddDeviceFunction");
    table->ValName("InitFunction");
    table->ValName("UnloadFunction");

    for (auto driver : drivers) {
        PzDriverObject read;
        table->ValueHex(driver);
        std::string name;

        if (!Link::ReadKernelObject<PzDriverObject>(driver, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            TRY_PRINT(Link::ReadKernelString(read.Name, name), table->ValueStr('"' + name + '"'));
            PRINT_OBJECT(table, read.AssociatedModule, PZ_OBJECT_MODULE);
            table->ValueHex(read.AddDeviceFunction);
            table->ValueHex(read.InitFunction);
            table->ValueHex(read.UnloadFunction);
        }
    }
    return table;
}

std::shared_ptr<ObjView> ObjPrint::Modules(
    const char *name, const std::vector<uptr>& modules)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 5;
    table->ValName("Address");           table->ValName("ModuleName");
    table->ValName("BaseAddress");       table->ValName("ImageSize");
    table->ValName("AssociatedProcess"); table->ValName("EntryPointAddress");

    for (auto module : modules) {
        table->ValueHex(module);
        PzModuleObject read;

        if (!Link::ReadKernelObject<PzModuleObject>(module, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            std::string name;
            TRY_PRINT(Link::ReadKernelString(read.ModuleName, name), table->ValueStr('"' + name + '"'));
            table->ValueHex(read.BaseAddress);
            table->ValueHex(read.ImageSize);
            PRINT_OBJECT(table, read.AssociatedProcess, PZ_OBJECT_PROCESS);
            table->ValueHex(read.EntryPointAddress);
        }
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::ThreadControlBlock(
    const char *name, PzThreadContext tcb)
{
    std::shared_ptr<ObjView> table(new ObjView(name));

    table->ValName("Eflags"); table->ValName("Eax");
    table->ValName("Ecx");  table->ValName("Edx");
    table->ValName("Ebx"); table->ValName("Esp");
    table->ValName("Ebp"); table->ValName("Esi");
    table->ValName("Edi"); table->ValName("Eip");
    table->ValName("Cs"); table->ValName("Ds");
    table->ValName("Es"); table->ValName("Fs");
    table->ValName("Gs"); table->ValName("Ss");
    table->ValName("Cr3");
    table->ValueHex(tcb.Eflags); table->ValueHex(tcb.Eax);
    table->ValueHex(tcb.Ecx);    table->ValueHex(tcb.Edx);
    table->ValueHex(tcb.Ebx);    table->ValueHex(tcb.Esp);
    table->ValueHex(tcb.Ebp);    table->ValueHex(tcb.Esi);
    table->ValueHex(tcb.Edi);    table->ValueHex(tcb.Eip);
    table->ValueHex((u16)tcb.Cs); table->ValueHex((u16)tcb.Ds);
    table->ValueHex((u16)tcb.Es); table->ValueHex((u16)tcb.Fs);
    table->ValueHex((u16)tcb.Gs); table->ValueHex((u16)tcb.Ss);
    table->ValueHex(tcb.Cr3);

    return table;
}

std::shared_ptr<ObjView> ObjPrint::Threads(
    const char *name, const std::vector<uptr> &threads)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 12;

    table->ValName("Address");
    table->ValName("IsUserMode");
    table->ValName("Id");
    table->ValName("Flags");
    table->ValName("SuspendedCount");
    table->ValName("ParentProcess");
    table->ValName("ControlBlock");
    table->ValName("SchThreadListNode");
    table->ValName("WaitObject");
    table->ValName("UserStack");
    table->ValName("KernelStack");
    table->ValName("UserStackSize");
    table->ValName("KernelStackSize");
    table->ValName("Priority");
    table->ValName("RemainingQuanta");

    for (auto thread : threads) {
        table->ValueHex(thread);
        PzThreadObject read;

        if (!Link::ReadKernelObject<PzThreadObject>(thread, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            table->ValueBool(read.IsUserMode);
            table->ValueInt(read.Id);
            table->ValueInt(read.Flags);
            table->ValueInt(read.SuspendedCount);
            PRINT_OBJECT(table, read.ParentProcess, PZ_OBJECT_PROCESS);
            table->ValueTable(ThreadControlBlock("Show", read.ControlBlock));
            table->ValueHex(read.SchThreadListNode);
            table->ValueHex(read.WaitObject);
            table->ValueHex(read.UserStack);
            table->ValueHex(read.KernelStack);
            table->ValueHex(read.UserStackSize);
            table->ValueHex(read.KernelStackSize);
            table->ValueInt(read.Priority);
            table->ValueInt(read.RemainingQuanta);
        }
    }
    return table;
}

std::shared_ptr<ObjView> ObjPrint::VirtualAllocations(
    const char *name, const std::vector<PzUserVirtualRegion> &regions)
{
    std::shared_ptr<ObjView> table(new ObjView(name));

    table->ValName("Start");
    table->ValName("End");
    table->ValName("Flags");

    for (auto reg : regions) {
        table->ValueHex(reg.Start);
        table->ValueHex(reg.End);
        table->ValueHex(reg.Flags);
    }

    return table;
}

std::shared_ptr<ObjView> ObjPrint::Processes(
    const char *name, const std::vector<uptr> &processes)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 16;

    table->ValName("Address");
    table->ValName("Id");
    table->ValName("Flags");
    table->ValName("LastHandleId");
    table->ValName("PhysicalPageDirectory");
    table->ValName("VirtualPageDirectory");
    table->ValName("UserThreadExit");
    table->ValName("Cr3");
    table->ValName("Name");
    table->ValName("VirtualAllocations");
    table->ValName("HandleTable");
    table->ValName("LoadedModules");
    table->ValName("Threads");
    table->ValName("StandardInput");
    table->ValName("StandardOutput");
    table->ValName("StandardError");
    table->ValName("UserMode");

    for (auto process : processes) {
        PzProcessObject read;
        table->ValueHex(process);

        if (!Link::ReadKernelObject<PzProcessObject>(process, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            BackcheckMap[process] = table;
            table->ValueInt(read.Id);
            table->ValueInt(read.Flags);
            table->ValueInt(read.LastHandleId);
            table->ValueHex(read.PhysicalPageDirectory);
            table->ValueHex(read.VirtualPageDirectory);
            table->ValueHex(read.UserThreadExit);
            table->ValueHex(read.Cr3);

            std::string name;
            TRY_PRINT(Link::ReadKernelString(read.Name, name), table->ValueStr('"' + name + '"'));

            std::vector<PzUserVirtualRegion> regs;
            std::vector<PzHandleTableEntry> handles;
            std::vector<uptr> modules, threads;

            TRY_PRINT(Link::ReadKernelLinkedList(read.VirtualAllocations, regs),
                table->ValueTable(VirtualAllocations("Show", regs)));
            TRY_PRINT(Link::ReadKernelLinkedList(read.HandleTable, handles),
                table->ValueTable(Handles("Show", handles)));
            TRY_PRINT(Link::ReadKernelLinkedList(read.LoadedModules, modules),
                table->ValueTable(Modules("Show", modules)));
            TRY_PRINT(Link::ReadKernelLinkedList(read.Threads, threads),
                table->ValueTable(Threads("Show", threads)));

            table->ValueInt(read.StandardInput);
            table->ValueInt(read.StandardOutput);
            table->ValueInt(read.StandardError);
            table->ValueBool(read.IsUserMode);
        }
    }
    return table;
}

std::shared_ptr<ObjView> ObjPrint::Symlinks(
    const char *name, const std::vector<uptr> &symlinks)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 2;

    table->ValName("Address");
    table->ValName("Name");
    table->ValName("To");

    for (auto symlink : symlinks) {
        table->ValueHex(symlink);
        PzSymlinkObject read;

        if (!Link::ReadKernelObject<PzSymlinkObject>(symlink, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            std::string name;
            TRY_PRINT(Link::ReadKernelString(read.Name, name), table->ValueStr('"' + name + '"'));
            PRINT_OBJECT(table, read.To, -1);
        }
    }
    return table;
}

std::shared_ptr<ObjView> ObjPrint::Windows(
    const char *name, const std::vector<uptr> &windows)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 4;

    table->ValName("Address");
    table->ValName("Title");
    table->ValName("Parent");
    table->ValName("Children");
    table->ValName("Parameters");

    for (auto window : windows) {
        table->ValueHex(window);

        PzWindowObject read;

        if (!Link::ReadKernelObject<PzWindowObject>(window, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            BackcheckMap[window] = table;
            std::string title;

            TRY_PRINT(Link::ReadKernelString(read.Title, title), table->ValueStr('"' + title + '"'));
            PRINT_OBJECT(table, read.Parent, -1);

            std::vector<uptr> children;
            Link::ReadKernelLinkedList(read.Children.First, children);
            table->ValueTable(Windows("Show", children));

            std::shared_ptr<ObjView> params(new ObjView("Show"));
            for (int i = 0; i <= WINDOW_MAX_PARAM; i++) {
                params->ValName("");
                params->ValueHex(read.Parameters[i]);
            }

            table->ValueTable(params);
        }
    }
    return table;
}

std::shared_ptr<ObjView> ObjPrint::Graphics(
    const char *name, const std::vector<uptr> &graphics)
{
    std::shared_ptr<ObjView> table(new ObjView(name));
    int columns = 2;

    table->ValName("Address");
    table->ValName("ObjectType");
    table->ValName("Data");

    for (auto gfx : graphics) {
        table->ValueHex(gfx);
        PzGraphicsObject read;

        if (!Link::ReadKernelObject<PzGraphicsObject>(gfx, &read))
            for (int i = 0; i < columns; i++)
                table->ValueCStr("???");
        else {
            table->ValueInt(read.ObjectType);
            table->ValueHex(read.Data);
        }
    }
    return table;
}

std::shared_ptr<ObjView> ObjPrint::Directories(
    const char *name, const std::vector<uptr> &directories)
{
    std::shared_ptr<ObjView> table(new ObjView(name));

    table->ValName("Address");
    table->ValName("Objects");

    for (auto directory : directories) {
        table->ValueHex(directory);
        PzDirectoryObject read;
        if (!Link::ReadKernelObject<PzDirectoryObject>(directory, &read))
            table->ValueCStr("???");
        else {
            std::vector<ObPointer> objects;
            if (Link::ReadKernelLinkedList(read.Objects, objects)) {
                std::shared_ptr<ObjView> table_objs(new ObjView("Show"));

                for (ObPointer ptr : objects) {
                    ObObjectHeader header;

                    Link::ReadVirtualMem(ptr - sizeof(ObObjectHeader), &header, sizeof(ObObjectHeader));

                    static const char *obj_names[] = {
                        "Timer", "Driver", "Device", "Symlink", "Directory", "Mutex", "Semaphore",
                        "Event", "Process", "Thread", "Iocb", "Module", "Anonymous Pipe",
                        "Window", "Graphics"
                    };

                    if (header.Type >= 1 && header.Type <= PZ_OBJECT_TYPE_CNT)
                        table_objs->ValName(obj_names[header.Type - 1]);
                    else
                        table_objs->ValName("");

                    PRINT_OBJECT(table_objs, ptr, -1);
                }

                table->ValueTable(table_objs);
            }
        }
    }
    return table;
}

std::shared_ptr<ObjView> ObjPrint::Object(uptr addr, int type)
{
    if (type == -1) {
        ObObjectHeader header;

        if (!Link::ReadVirtualMem(addr - sizeof(ObObjectHeader), &header, sizeof(ObObjectHeader)))
            return nullptr;

        type = header.Type;
    }

#define CASE(type_id, func) \
    case type_id: return func("Show", std::vector<uptr> { addr });
    switch (type) {
        CASE(PZ_OBJECT_TIMER, Timers)
        CASE(PZ_OBJECT_DRIVER, Drivers)
        CASE(PZ_OBJECT_DEVICE, Devices)
        CASE(PZ_OBJECT_SYMLINK, Symlinks)
        CASE(PZ_OBJECT_DIRECTORY, Directories)
        CASE(PZ_OBJECT_MUTEX, Mutexes)
        CASE(PZ_OBJECT_SEMAPHORE, Semaphores)
        CASE(PZ_OBJECT_EVENT, Events)
        CASE(PZ_OBJECT_PROCESS, Processes)
        CASE(PZ_OBJECT_THREAD, Threads)
        CASE(PZ_OBJECT_IOCB, Iocbs)
        CASE(PZ_OBJECT_MODULE, Modules)
        CASE(PZ_OBJECT_APIPE, APipes)
        CASE(PZ_OBJECT_WINDOW, Windows)
        CASE(PZ_OBJECT_GFX, Graphics)
        default: return nullptr;
    }
#undef CASE
}