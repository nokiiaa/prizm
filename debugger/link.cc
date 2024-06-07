#include <link.hh>
#include <windows.h>

namespace Link {
    std::vector<uptr>
        APipes, Devices, Directories, Drivers,
        Events, Iocbs, Modules, Mutexes,
        Processes, Semaphores, Symlinks, Threads,
        Timers, Windows, Graphics;

    std::queue<LinkCommand> CommandQueue;
    std::string LogBuffer;
    HANDLE PortHandle;
    volatile bool Established;
    bool Panicked, Breakpoint;
    LinkPanicInfo PanicInfo;
    LinkBreakInfo BreakInfo;
}

int Link::Open()
{
    for (;;) {
        PortHandle = CreateFileW(
            L"\\\\.\\pipe\\PrizmDbg",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            0
        );

        if (PortHandle != INVALID_HANDLE_VALUE)
            return 0;

        int error = GetLastError();

        if (error != ERROR_PIPE_BUSY && error != ERROR_FILE_NOT_FOUND)
            return error;
    }
}

bool Link::Peek(void *buffer, int length)
{
    DWORD read, avail, left;

    do {
        if (!PeekNamedPipe(PortHandle, buffer, length, &read, &avail, &left))
            return false;
    } while (read < length);

    return true;
}

bool Link::Read(void *buffer, int length)
{
    DWORD read = 0;
    u8 *buf = (u8*)buffer;

    for (int left = length; left; left -= read) {
        if (!ReadFile(PortHandle, buf, left, &read, nullptr))
            return false;
        buf += read;
    }

    return true;
}

bool Link::Write(const void *buffer, int length)
{
    DWORD written;

    if (!WriteFile(PortHandle, buffer, length, &written, nullptr))
        return false;

    return true;
}

bool Link::ReadVirtualMem(uptr start, void *buffer, int length)
{
    std::printf("ReadVirtualMem 0x%p 0x%p 0x%i\n", start, buffer, length);

    LinkCommand command(COMMAND_READ_VMEM);
    command.Data<uptr>(start);
    command.Data<u32>(length);
    bool success = true;

    command.SetCallback([&](u32 response) {
        std::cout << "ReadVirtualMem lambda\n";
        if (response == ERROR_ACCESS_VIOLATION || response == ERROR_COMMAND_ABORTED) {
            std::cout << "ReadVirtualMem error\n";
            success = false;
        }
        Read(buffer, length);
        std::cout << "ReadVirtualMem lambda end\n";
    });

    SubmitCommand(command);
    std::cout << "ReadVirtualMem end\n";
    return success;
}

bool Link::WriteVirtualMem(uptr start, const void *buffer, int length)
{
    LinkCommand command(COMMAND_WRITE_VMEM);
    command.Data<uptr>(start);
    command.Data<u32>(length);
    command.Data(buffer, length);
    bool success = true;

    command.SetCallback([&](u32 response) {
        if (response == ERROR_ACCESS_VIOLATION || response == ERROR_COMMAND_ABORTED)
            success = false;
    });

    SubmitCommand(command);
    return success;
}

bool Link::AddObject(uptr addr)
{
    ObObjectHeader header;
    if (!ReadVirtualMem(addr, &header, sizeof(ObObjectHeader)))
        return false;
    addr += sizeof(ObObjectHeader);
    #define CASE(type_id, list) \
    case type_id: { list.push_back(addr); break; }

    switch (header.Type) {
        CASE(PZ_OBJECT_TIMER,     Timers)
        CASE(PZ_OBJECT_DRIVER,    Drivers)
        CASE(PZ_OBJECT_DEVICE,    Devices)
        CASE(PZ_OBJECT_SYMLINK,   Symlinks)
        CASE(PZ_OBJECT_DIRECTORY, Directories)
        CASE(PZ_OBJECT_MUTEX,     Mutexes)
        CASE(PZ_OBJECT_SEMAPHORE, Semaphores)
        CASE(PZ_OBJECT_EVENT,     Events)
        CASE(PZ_OBJECT_PROCESS,   Processes)
        CASE(PZ_OBJECT_THREAD,    Threads)
        CASE(PZ_OBJECT_IOCB,      Iocbs)
        CASE(PZ_OBJECT_MODULE,    Modules)
        CASE(PZ_OBJECT_APIPE,     APipes)
        CASE(PZ_OBJECT_WINDOW,    Windows)
        CASE(PZ_OBJECT_GFX,       Graphics)
    }

    return true;
    #undef CASE
}

void Link::SubmitCommand(const LinkCommand &command, bool block)
{
    Write<u32>(command.Id);
    Write(command.Buffer.data(), command.Buffer.size());

    CommandQueue.push(command);

    if (block)
        WaitForSingleObject(command.Event, INFINITE);
}

DWORD ListenerThread(LPVOID param)
{
    Link::Open();
    Sleep(1000);
    Link::Write<u32>(CONNECT_SEQUENCE);

    for (;;) {
        u32 read = Link::Peek<u32>();
        LinkCommand &command = Link::CommandQueue.front();
        switch (read) {
            case CONNECT_SEQUENCE:
                Link::Read<u32>();
                Link::Established = true;
                break;

            case EVENT_LOG_STRING: {
                Link::Read<u32>();
                while (char c = Link::Read<char>())
                    Link::LogBuffer += c;
                break;
            }

            case EVENT_KERNEL_PANIC: {
                Link::Read<u32>();
                u32 reason = Link::PanicInfo.Reason = Link::Read<u32>();
                u32 address;
                while ((address = Link::Read<uptr>()) != -1)
                    Link::PanicInfo.StackTrace.push_back(address);
                if (Link::Read<u32>()) /* If CPU state has been sent */
                    Link::Read(&Link::PanicInfo.CpuState, sizeof(CpuInterruptState));
                while (char c = Link::Read<char>())
                    Link::PanicInfo.MessageString += c;
                Link::Panicked = true;
                break;
            }

            case EVENT_BREAKPOINT:
                Link::Read<u32>();
                u32 address;
                Link::BreakInfo.StackTrace.clear();
                while ((address = Link::Read<uptr>()) != -1)
                    Link::BreakInfo.StackTrace.push_back(address);
                Link::Read(&Link::BreakInfo.CpuState, sizeof(CpuInterruptState));
                Link::Breakpoint = true;
                break;

            default:
                if (Link::CommandQueue.size()) {
                    command.Callback(Link::Read<u32>());
                    SetEvent(command.Event);
                    Link::CommandQueue.pop();
                }
                break;
        }
    }
}

uptr Link::GetRootDirectory()
{
    LinkCommand command(COMMAND_OBJ_ROOT);
    uptr read;

    command.SetCallback([&](u32 response) {
        read = response == ERROR_COMMAND_ABORTED ? 0 : (uptr)Read<u32>();
    });

    SubmitCommand(command);
    return read;
}

void Link::Break()
{
    LinkCommand command(COMMAND_BREAK);
    command.SetCallback([&](u32 response) {});
    SubmitCommand(command);
}

void Link::Continue()
{
    LinkCommand command(COMMAND_CONTINUE);
    command.SetCallback([&](u32 response) { });
    SubmitCommand(command);
}

bool Link::ReadKernelString(uptr str, std::string &out)
{
    std::printf("ReadKernelString 0x%p\n", str);
    LinkCommand command(COMMAND_READ_STRING);
    command.Data<uptr>(str);
    bool success = true;

    command.SetCallback([&](u32 response) {
        std::cout << "ReadKernelString lambda\n";

        if (response == ERROR_ACCESS_VIOLATION || response == ERROR_COMMAND_ABORTED)
            success = false;
        else {
            u32 size = Read<u32>();
            std::cout << "size=" << size << "\n";
            std::shared_ptr<char[]> chars(new char[size]);
            Read(chars.get(), size);
            out = std::string(chars.get());
        }

        std::cout << "ReadKernelString lambda end\n";
    });

    SubmitCommand(command);
    std::cout << "ReadKernelString end\n";
    return success;
}

bool Link::RefreshObjectList()
{
    std::vector<uptr> *colls[] = {
        &Timers, &Drivers, &Devices, &Symlinks,
        &Directories, &Mutexes, &Semaphores, &Events,
        &Processes, &Threads, &Iocbs, &Modules, &APipes,
        &Windows, &Graphics
    };

    for (auto *c : colls) c->clear();
    LinkCommand command(COMMAND_OBJECT_LIST);
    std::vector<u32> addresses;
    bool success = true;

    command.SetCallback([&](u32 response) {
        if (response == ERROR_COMMAND_ABORTED) {
            success = false;
            return;
        }

        for (u32 addr; addr = Read<u32>(); ) {
            std::printf("0x%p\r\n", addr);
            addresses.push_back(addr);
        }
    });

    SubmitCommand(command);
    for (u32 addr : addresses)
        AddObject(addr);

    return success;
}

void Link::StartListening()
{
    CreateThread(nullptr, 0,
        (LPTHREAD_START_ROUTINE)ListenerThread,
        nullptr, 0, nullptr);
}