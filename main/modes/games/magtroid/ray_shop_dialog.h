#ifndef _RAY_SHOP_DIALOG_H_
#define _RAY_SHOP_DIALOG_H_

#include "mode_ray.h"

void rayShowShopDialog(ray_t* ray, wsg_t* icon, uint32_t cost, rayMapCellType_t obj);
void rayShopDialogCheckButtons(ray_t* ray);
void rayShopDialogRender(ray_t* ray, uint32_t elapsedUs);

#endif