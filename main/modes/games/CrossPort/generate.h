#ifndef GENERATE_H
#define GENERATE_H
#include "library.h"

void checkFaces(int x, int y, int z, int c, int faces[6]);
void generateWorld(int chunkX, int chunkY, int chunkZ, int c, int seed);
void cleanWorld();

#endif