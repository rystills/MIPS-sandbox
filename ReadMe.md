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