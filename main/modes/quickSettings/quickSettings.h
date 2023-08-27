#ifndef _QUICK_SETTINGS_MODE_H_
#define _QUICK_SETTINGS_MODE_H_

#include "swadge2024.h"
#include "hdw-tft.h"

/// The width of the quick settings panel, centered in the screen
#define QUICK_SETTINGS_PANEL_W (TFT_WIDTH - 90)
#define QUICK_SETTINGS_PANEL_H 60
#define QUICK_SETTINGS_PANEL_X ((TFT_WIDTH - QUICK_SETTINGS_PANEL_W) / 2)
#define QUICK_SETTINGS_PANEL_R 15

extern swadgeMode_t quickSettingsMode;

#endif
