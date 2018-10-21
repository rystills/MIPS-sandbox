## steps to compile on ubuntu  
install sdl2:  
```sudo apt-get install libsdl2-dev```  
install glew:  
```sudo apt-get install libglew-dev```  
compile with sdl2 and lGL:  
```gcc -Wall  MIPS-sandbox.c -o main.out -lm -I. `sdl2-config --cflags --libs` -lGL```  
or run make:  
```make```

## steps to compile on windows  
compile with gdi:  
```gcc -Wall src/MIPS-sandbox.c -o main.exe -lm -I. -lgdiplus -lshlwapi```  
or run make (if you have minGW installed with make somewhere in your system path):  
```make```

## To develop in Eclipse on Windows  
-Navigate to Project Properties >> C/C++ Build  
-set Builder type to External builder  
-set toolchain to MinGW GCC  
-uncheck "Generate Makefiles automatically"  
-add the following makefile to your Debug/Release folder:  
```OSFLAG :=
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
	CFLAGS += -lgdiplus -lshlwapi
else
    detected_OS := $(shell uname)  # same as "uname -s"
	CFLAGS += `sdl2-config --cflags --libs` -lGL
endif

src/MIPS-sandbox.c: 

all: src/MIPS-sandbox.c
	gcc -Wall ../src/MIPS-sandbox.c -o MIPS-sandbox.exe -lm -I. $(CFLAGS)
```  