OSFLAG :=
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
	CFLAGS += -lgdiplus -lshlwapi
else
    detected_OS := $(shell uname)  # same as "uname -s"
	CFLAGS += `sdl2-config --cflags --libs` -lGL
endif

make: src/MIPS-sandbox.c
	gcc -Wall src/MIPS-sandbox.c -o main.exe -lm -I. $(CFLAGS)
