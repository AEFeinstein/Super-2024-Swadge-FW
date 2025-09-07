#ifndef MOVEMENT_H
#define MOVEMENT_H
#include "library.h"
#include "cam.h"

void movePlayer(Camera* c, Vect3i* Collidables, int amt);
void raycast(Camera* c);
void placingBlocks(Camera* c);

#endif