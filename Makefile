OSFLAG :=
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
    CFLAGS += -lgdiplus -lshlwapi -lcomctl32 -mwindows
else
    detected_OS := $(shell uname -s)
    ifeq ($(detected_OS),Linux)
		CFLAGS += `sdl2-config --cflags --libs` -lGL `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
    endif
	ifeq ($(detected_OS),Darwin)
		CFLAGS += `sdl2-config --cflags --libs` -lGL -framework AppKit
    endif
endif

make: src/MIPS-sandbox.c
	mkdir bin || echo
	copy nuklear\extra_font\ProggyClean.ttf bin || cp nuklear/extra_font/ProggyClean.ttf bin
	gcc -Wall src/MIPS-sandbox.c -o bin/main.exe -lm -I. $(CFLAGS)
#gcc -Wall src/MIPS-sandbox.c -O3 -o main.exe -lm -I. $(CFLAGS)
