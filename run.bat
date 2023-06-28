del vitmap-maker.exe
gcc main.c -o vitmap-maker.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -llibtess2 -lopengl32 -lgdi32 -lwinmm
vitmap-maker.exe