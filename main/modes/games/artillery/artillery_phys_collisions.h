#pragma once

#include "artillery_phys.h"

void physCheckCollisions(physSim_t* phys);
bool physAnyCollision(physSim_t* phys, physCirc_t* c);
