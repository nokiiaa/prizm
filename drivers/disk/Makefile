LIBGCC_OBJECTS=$(file < ../../link/objects.txt)
MAIN_BIN=../../bin
CFLAGS=-m32 -march=i686 -Werror -nostdlib -std=gnu++17 -ffreestanding -flto -O3 -msse2 -fdelete-null-pointer-checks -ffast-math -I../../shared/include -fno-exceptions -fno-use-cxa-atexit -fno-rtti -fno-threadsafe-statics $(addprefix ../../link/libgcc/, $(LIBGCC_OBJECTS))
CC=x86_64-w64-mingw32-g++
all:
	$(CC) -m32 -s $(CFLAGS) -shared main.cc dispatch.cc init.cc -o $(MAIN_BIN)/disk.sys -L../../link -L$(MAIN_BIN) -lpzkernel -e_DriverEntry -Wl,--image-base,0x80100000