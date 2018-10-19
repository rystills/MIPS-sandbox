steps to run on ubuntu:

install sdl2:
sudo apt-get install libsdl2-dev

install glew:
sudo apt-get install libglew-dev

compile with sdl2 and lGL:
gcc -Wall  MIPS-sandbox.c -o main.out -lm -I. `sdl2-config --cflags --libs` -lGL
