#pragma once

#include <main.hh>
#include <memory>
#include <queue>
#include <core.hh>
#include <windows.h>
#include <functional>

using CommandCallback = std::function<void(u32 response)>;

struct LinkPanicInfo
{
    u32 Reason;
    std::vector<uptr> StackTrace;
    CpuInterruptState CpuState;
    std::string MessageString;
};

struct LinkBreakInfo
{
    std::vector<uptr> StackTrace;
    CpuInterruptState CpuState;
};

struct LinkCommand
{
    u32 Id;
    std::vector<u8> Buffer;
    CommandCallback Callback;
    HANDLE Event;
    LinkCommand(u32 id) : Id(id) { Event = CreateEventW(nullptr, false, false, nullptr); }
    inline void SetCallback(CommandCallback callback) { Callback = callback; }
    inline void Data(const void *buffer, int length)
    { for (int i = 0; i < length; i++) Buffer.push_back(((u8 *)buffer)[i]); }
    template<class T> inline void Data(const T &value) { Data(&value, sizeof(T)); }
};

namespace Link
{
    extern std::vector<uptr>
        APipes, Devices, Directories, Drivers,
        Events, Iocbs, Modules, Mutexes,
        Processes, Semaphores, Symlinks, Threads,
        Timers, Windows, Graphics;
    extern std::queue<LinkCommand> CommandQueue;
    extern std::string LogBuffer;
    extern HANDLE PortHandle;
    extern volatile bool Established;
    extern bool Panicked, Breakpoint;
    extern LinkPanicInfo PanicInfo;
    extern LinkBreakInfo BreakInfo;

    int Open();
    bool AddObject(uptr addr);
    bool Peek(void *buffer, int length);
    bool Read(void *buffer, int length);
    bool Write(const void *buffer, int length);

    template<typename T> inline T Peek()
    {
        T value;
        Peek(&value, sizeof(T));
        return value;
    }
    template<class T> inline T Read()
    {
        T value;
        Read(&value, sizeof(T));
        return value;
    }
    template<class T> inline void Write(const T &value)
    {
        Write(&value, sizeof(T));
    }

    void StartListening();
    bool ReadVirtualMem(uptr start, void *buffer, int length);
    bool WriteVirtualMem(uptr start, const void *buffer, int length);
    bool RefreshObjectList();

    bool ReadKernelString(uptr str, std::string &out);
    uptr GetRootDirectory();

    template<typename T> inline bool ReadKernelObject(uptr obj, T *out)
    {
        return ReadVirtualMem(obj, out, sizeof(T));
    }

    void SubmitCommand(const LinkCommand &command, bool block = true);
    void Break();
    void Continue();

    template<class T> inline bool ReadKernelLinkedList(
        uptr start_node, std::vector<T> &out)
    {
        std::printf("ReadKernelLinkedList 0x%p\n", start_node);
        bool success = false;
        LinkCommand command(COMMAND_READ_LINKED_LIST);
        command.Data<uptr>(start_node);
        command.Data<u32>(sizeof(T));
        command.SetCallback([&](u32 response) {
            std::cout << "ReadKernelLinkedList lambda\n";
            if (response == ERROR_COMMAND_ABORTED)
                return;
            for (u32 length = Read<u32>(); length; length--) {
                //std::cout << length << "\n";
                T val;
                Read(&val, sizeof(T));
                out.push_back(val);
            }
            std::cout << "ReadKernelLinkedList lambda end\n";
            success = true;
        });
        SubmitCommand(command);
        std::cout << "ReadKernelLinkedList end\n";
        return success;
    }

    template<class T> inline bool ReadKernelLinkedList(
        LinkedList<T> list, std::vector<T> &out)
    {
        return ReadKernelLinkedList(list.First, out);
    }
};