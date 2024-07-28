#pragma once

#include "mode_pinball.h"

void createTableZones(pinball_t* p);
uint32_t pinZoneRect(pinball_t* p, pbRect_t r);
uint32_t pinZoneLine(pinball_t* p, pbLine_t l);
uint32_t pinZoneCircle(pinball_t* p, pbCircle_t c);
uint32_t pinZoneFlipper(pinball_t* p, pbFlipper_t* f);
