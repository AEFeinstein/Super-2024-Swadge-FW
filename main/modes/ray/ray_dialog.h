#ifndef _RAY_DIALOG_H_
#define _RAY_DIALOG_H_

#include "mode_ray.h"

extern const char lorem[];

void rayShowDialog(ray_t* ray, const char* dialogText, wsg_t* dialogPortrait);
void rayDialogCheckButtons(ray_t* ray);
void rayDialogRender(ray_t* ray);

#endif