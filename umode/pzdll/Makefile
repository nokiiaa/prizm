LIBGCC_OBJECTS=$(file < ../../link/objects.txt)
MAIN_BIN=../../bin
CFLAGS=-m32 -march=i686 -Werror -nostdlib -std=gnu++17 -ffreestanding -Os -fdelete-null-pointer-checks -ffast-math -I.. -fno-exceptions -fno-use-cxa-atexit -fno-rtti -fno-threadsafe-statics $(addprefix ../../link/libgcc/, $(LIBGCC_OBJECTS))
CC=x86_64-w64-mingw32-g++
all:
	$(CC) -s -shared $(CFLAGS) main.cc printf.cc heap.cc -o $(MAIN_BIN)/pzdll.dll -L../../link -L$(MAIN_BIN) -e_PzDllEntry -Wl,--image-base,0x10000000,--out-implib,../../bin/pzdll.lib