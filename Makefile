OSFLAG :=
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
	CFLAGS += -lgdiplus -lshlwapi -lcomctl32 -mwindows
else
    detected_OS := $(shell uname)  # same as "uname -s"
	CFLAGS += `sdl2-config --cflags --libs` -lGL `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
endif

make: src/MIPS-sandbox.c
	gcc -Wall src/MIPS-sandbox.c -o3 -o main.exe -lm -I. $(CFLAGS)
