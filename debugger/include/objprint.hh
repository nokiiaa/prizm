#pragma once

#include <view.hh>

#define DECL_PRINT(name, param) \
    std::shared_ptr<ObjView> name( \
    const char *name, const std::vector<uptr> &param);

namespace ObjPrint {
    std::shared_ptr<ObjView> Handles(
        const char *name, const std::vector<PzHandleTableEntry> &handles);

    std::shared_ptr<ObjView> ThreadControlBlock(
        const char *name, PzThreadContext tcb);

    std::shared_ptr<ObjView> VirtualAllocations(
        const char *name, const std::vector<PzUserVirtualRegion> &regions);
    DECL_PRINT(APipes, apipes)
    DECL_PRINT(Events, events)
    DECL_PRINT(Mutexes, mutexes)
    DECL_PRINT(Semaphores, semaphores)
    DECL_PRINT(Timers, timers)
    DECL_PRINT(Iocbs, iocbs)
    DECL_PRINT(Devices, devices)
    DECL_PRINT(Drivers, drivers)
    DECL_PRINT(Modules, modules)
    DECL_PRINT(Threads, threads)
    DECL_PRINT(Processes, processes)
    DECL_PRINT(Symlinks, symlinks)
    DECL_PRINT(Directories, directories)
    DECL_PRINT(Windows, windows)
    DECL_PRINT(Graphics, graphics)
    std::shared_ptr<ObjView> Object(uptr addr, int type = -1);
}