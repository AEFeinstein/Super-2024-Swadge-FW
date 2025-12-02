#include <stdint.h>
#include "mgGameData.h"

typedef enum
{
    Pulse, Sawtooth, Bigma, TrashMan
} cutsceneCharacters;

void levelStartCutscene(mgGameData_t* gameData);

void bossStartCutscene(mgGameData_t* gameData);

void bossDeathCutscene(mgGameData_t* gameData);