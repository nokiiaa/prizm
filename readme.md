# Prizm
This is a complete overhaul of the old Prizm operating system, which was a single-address space and completely static unikernel without userspace support.
Development is aiming for better code quality, readability and design closeness to normal monolithic kernel operating systems such as Windows and Linux.
It is an x86_32 monolithic kernel operating system that mimicks some of NT's design.
A dynamic driver model is present. The simple FAT32 bootloader loads the kernel image and a crucial filesystem driver from disk, which allows the kernel to then load it from memory and subsequently load any other drivers and files to boot up the system.
As of right now, getting the kernel to run is not an automated process but it will be made easier in the future. The code and the Makefile are still there, however. As PE, as it is in NT, is the main executable format, x86_64-w64-mingw32-gcc is a prerequisite for compiling the kernel.

## **Implemented/worked-on features**
- Physical memory manager
- Virtual memory manager
- Scheduler
- Driver model with an NT-like interface, simple dynamic filesystem driver support
- Refcounted object manager and I/O manager kernel APIs
- PCI IDE read-only disk driver
- Read-only FAT32 filesystem driver
- Making the move into userspace (61 system calls are implemented so far with executables linking to a layer called pzdll.dll)
- Software-rendered graphics subsystem with support for 3D graphics
- Kernel debugger through a named pipe (see debugger directory)
- SEH-like exception handling in usermode
- Windowing system

## **Plans**
- Possible VMware SVGA II acceleration for graphics subsystem
- Multiprocessed scheduling
- Possible 64-bit port (depending on difficulty)
- Keyboard and mouse support
- Audio subsystem, perhaps with drivers for AC'97 or Intel HDA devices emulated by VMware, VirtualBox and qemu
- Security features

In this old image, the label on the window and the title of the window itself have been edited out.

![Demonstration](https://i.imgur.com/nrP3V58.png)

## **Known bugs**
- There seems to be a bug related to starting particular processes simultaneously, which still needs to be investigated.

## **How to build**
- Use Make to build the source code. The batch files are written for my own personal convenience.