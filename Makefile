make: src/MIPS-sandbox.c
	gcc -Wall  src/MIPS-sandbox.c -o main.out -lm -I. `sdl2-config --cflags --libs` -lGL
