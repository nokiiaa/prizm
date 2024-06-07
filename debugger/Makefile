IMGUI=$(wildcard imgui/*.cpp)
CODE=$(wildcard *.cc)
CC=g++
CFLAGS=-std=c++17 -Iinclude -Iinclude/SDL -I../shared/include -static-libstdc++
LIB=-Llib -lsdl2 -lSDL2main -lzydis
clean:
	rm bin/*
all:
	mkdir -p bin
	$(CC) glad.c $(CODE) $(IMGUI) $(CFLAGS) $(LIB) -o bin/prizmdbg
	cp font/FiraCode-Medium.ttf bin/FiraCode-Medium.ttf