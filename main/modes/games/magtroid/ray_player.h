#ifndef _RAY_PLAYER_H_
#define _RAY_PLAYER_H_

#include "mode_ray.h"

bool initializePlayer(ray_t* ray);
void rayPlayerCheckButtons(ray_t* ray, rayObjCommon_t* centeredSprite, uint32_t elapsedUs);
void rayPlayerCheckJoystick(ray_t* ray, uint32_t elapsesUs);
void rayPlayerTouchItem(ray_t* ray, rayObjCommon_t* item, int32_t mapId);
void rayPlayerCheckFloorEffect(ray_t* ray, uint32_t elapsedUs);
void raySavePlayer(ray_t* ray);
void raySaveVisitedTiles(ray_t* ray);
void rayPlayerDecrementHealth(ray_t* ray, int32_t health);

#endif