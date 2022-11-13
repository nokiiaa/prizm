LIBGCC_OBJECTS=$(file < link/objects.txt)
CFLAGS=-m32 -march=i686 -Werror -nostdlib -std=gnu++17 -ffreestanding -Llink -O3 -fdelete-null-pointer-checks -ffast-math -Ishared/include -fno-exceptions -fno-use-cxa-atexit -fno-rtti -fno-threadsafe-statics $(addprefix link/libgcc/, $(LIBGCC_OBJECTS))
CC=x86_64-w64-mingw32-gcc
AS=nasm
FILES=$(wildcard kernel/*.cc) $(wildcard kernel/*/*.cc) $(wildcard kernel/*/*/*.cc) $(wildcard kernel/*.c) $(wildcard kernel/*/*.c) obj/helper.o
dbginfo:
	./link/dwarfdump --print-lines bin/pzkernel.exe > bin/pzkernel_lines.dbg
	./link/dwarfdump --print-info bin/pzkernel.exe > bin/pzkernel_info.dbg
	strip -s --remove-section=.comment bin/pzkernel.exe
clean:
	rm -r bin obj
build:
	mkdir -p bin obj
	$(AS) -fwin32 kernel/x86/helper.asm -o obj/helper.o
	$(CC) -g -T link/linker.ld -o bin/pzkernel.exe $(FILES) $(CFLAGS) -Wl,--image-base,0x80100000,--out-implib,bin/pzkernel.lib
	make -C drivers/disk all
	make -C drivers/fat all
	make -C drivers/gfx all
	make -C umode/pzdll all
	make -C umode/pzinit all
	make -C umode/pzwindow all
	make -C umode/funny all
	make -C umode/pzconhost all
	make -C umode/pzshell all
	$(AS) -fbin kernel/x86/boot/part1.asm -o bin/bootloader_p1.bin
	$(AS) -fbin kernel/x86/boot/part2.asm -o bin/bootloader_p2.bin