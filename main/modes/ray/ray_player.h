#ifndef _RAY_PLAYER_H_
#define _RAY_PLAYER_H_

#include "mode_ray.h"

void initializePlayer(ray_t* ray, bool initialLoad);
void rayPlayerCheckButtons(ray_t* ray, rayObjCommon_t* centeredSprite, uint32_t elapsedUs);
void rayPlayerCheckJoystick(ray_t* ray, uint32_t elapsesUs);
void rayPlayerTouchItem(ray_t* ray, rayMapCellType_t type, int32_t mapId, int32_t itemId);
void rayPlayerCheckLava(ray_t* ray, uint32_t elapsedUs);

#endif