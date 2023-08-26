#ifndef _MODE_LUMBERJACK_TYPES_H_
#define _MODE_LUMBERJACK_TYPES_H_

enum lumberjackPlayerState {
    LUMBERJACK_DEAD = -1,
    LUMBERJACK_IDLE = 1,
    LUMBERJACK_RUN = 2,
    LUMBERJACK_DUCK = 3,
    LUMBERJACK_VICTORY = 4,
    LUMBERJACK_CLIMB = 5,
    LUMBERJACK_FALLING = 6
};

//Animation speeds
//90000 - run
//150000 - idle

int lumberjackAnimationNone[] = {0};
int lumberjackAnimationIdle[] = {0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 3, 1};
int lumberjackAnimationRun[] = {7, 8, 9, 10, 11, 12};
int lumberjackAnimationDuck[] = {16};
int lumberjackAnimationFall[] = {13};
int lumberjackAnimationDead[] = {14};
int lumberjackAnimationVictory[] = {15};
int lumberjackAnimationClimb[] = {17 ,18,19,20};

#endif