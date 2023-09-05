#ifndef _MODE_LUMBERJACK_TYPES_H_
#define _MODE_LUMBERJACK_TYPES_H_

enum lumberjackPlayerState
{
    LUMBERJACK_DEAD        = -1,
    LUMBERJACK_IDLE        = 1,
    LUMBERJACK_RUN         = 2,
    LUMBERJACK_DUCK        = 3,
    LUMBERJACK_VICTORY     = 4,
    LUMBERJACK_CLIMB       = 5,
    LUMBERJACK_FALLING     = 6,
    LUMBERJACK_OFFSCREEN   = 7, // reserved for enemies only
    LUMBERJACK_BUMPED      = 8,
    LUMBERJACK_BUMPED_IDLE = 9
};

// Animation speeds
// 90000 - run
// 150000 - idle

#endif