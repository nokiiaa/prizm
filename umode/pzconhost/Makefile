LIBGCC_OBJECTS=$(file < ../../link/objects.txt)
MAIN_BIN=../../bin
CFLAGS=-msse2 -m32 -march=i686 -Werror -nostdlib -std=gnu++17 -ffreestanding -fdelete-null-pointer-checks -ffast-math -I.. -fno-exceptions -fno-use-cxa-atexit -fno-rtti -fno-threadsafe-statics $(addprefix ../../link/libgcc/, $(LIBGCC_OBJECTS))
CC=x86_64-w64-mingw32-g++
all:
	$(CC) -m32 -flto -O3 $(CFLAGS) main.cc math.cc -o $(MAIN_BIN)/pzconhost.exe -L../../link -L$(MAIN_BIN) -lpzdll -e_PzProcessStartup -Wl,--image-base,0x400000