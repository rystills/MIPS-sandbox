## steps to compile on ubuntu  
install sdl2:  
```sudo apt-get install libsdl2-dev```  
install glew:  
```sudo apt-get install libglew-dev```  
install GTK3:  
```sudo apt-get install libgtk-3-dev```  
create a folder 'bin'  
compile with sdl2 and lGL:  
```gcc -Wall  MIPS-sandbox.c -o3 -o bin/main.out -lm -I. `sdl2-config --cflags --libs` -lGL `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` ```  
finally, copy nuklear/extra_font/ProggyClean.ttf to the bin folder  
alternatively, run make:  
```make```  

## steps to compile on windows  
create a folder 'bin'  
compile with gdi:  
```gcc -Wall src/MIPS-sandbox.c -o3 -o bin/main.exe -lm -I. -lgdiplus -lshlwapi -lcomctl32 -mwindows```  
finally, copy nuklear/extra_font/ProggyClean.ttf to the bin folder  
alternatively, run make (if you have minGW installed with make somewhere in your system path):  
```make```  

## steps to compile on Mac OSX  
TODO: get access to a mac and fill me out  

## To develop in Eclipse on Windows  
-Navigate to Project Properties >> C/C++ Build  
-set Builder type to External builder  
-set toolchain to MinGW GCC  
-uncheck "Generate Makefiles automatically"  
-add the following makefile to your Debug/Release folder:  
```OSFLAG :=
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

src/MIPS-sandbox.c: 

all: src/MIPS-sandbox.c
	copy ..\nuklear\extra_font\ProggyClean.ttf || cp ../nuklear/extra_font/ProggyClean.ttf .
    gcc -Wall ../src/MIPS-sandbox.c -O3 -o MIPS-sandbox.exe -lm -I. $(CFLAGS)
```  
