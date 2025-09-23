#pragma once

#include "mode_swadgeHero.h"

void shSetupMenu(shVars_t* sh);
void shTeardownMenu(shVars_t* sh);
void shMenuInput(shVars_t* sh, buttonEvt_t* btn);
void shMenuDraw(shVars_t* sh, int32_t elapsedUs);

bool shGetSettingFail(void);
int32_t shGetSettingSpeed(void);
