#ifndef _RAY_DIALOG_H_
#define _RAY_DIALOG_H_

#include "mode_ray.h"

extern const char* finalDialog100_0;

void rayShowDialog(ray_t* ray, const char* dialogText, wsg_t* dialogPortrait);
void rayDialogCheckButtons(ray_t* ray);
void rayDialogRender(ray_t* ray, uint32_t elapsedUs);

#endif