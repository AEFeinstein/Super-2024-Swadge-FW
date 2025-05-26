#pragma once

#include "swadge2024.h"

extern swadgeMode_t bubbleShooterMode;

void bubbleShooterGameLoop(int64_t elapsedUs);

void bubbleShooterInitGame(void);
void bubbleShooterGameDraw(void);
void bubbleShooterDestroyGame(void);

typedef struct
{
    wsg_t bubbleSprite[7];
    int grid[12][8];
    
} bubbleShooterGame_t;