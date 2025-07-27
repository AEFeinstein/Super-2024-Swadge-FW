#pragma once

#include "artillery_phys.h"

physLine_t* physAddLine(physSim_t* phys, float x1, float y1, float x2, float y2, bool isTerrain);
physCirc_t* physAddCircle(physSim_t* phys, float x1, float y1, float r, circType_t type);
void updateLineProperties(physSim_t* phys, physLine_t* pl);
void updateCircleProperties(physSim_t* phys, physCirc_t* pc);
