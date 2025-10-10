#pragma once

#include "artillery_phys.h"

physLine_t* physAddLine(physSim_t* phys, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, bool isTerrain);
physCirc_t* physAddCircle(physSim_t* phys, uint16_t x1, uint16_t y1, uint16_t r, circType_t type,
                          paletteColor_t baseColor, paletteColor_t accentColor);
void updateLineProperties(physSim_t* phys, physLine_t* pl);
void updateCircleProperties(physSim_t* phys, physCirc_t* pc);
