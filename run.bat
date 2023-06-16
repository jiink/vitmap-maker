gcc main.c -o main.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -llibtess2 -lopengl32 -lgdi32 -lwinmm
main.exe