#pragma once

void emuSetUseRealTime(bool useRealTime);
void emuSetEspTimerTime(int64_t timeUs);
void emuTimerPause(void);
void emuTimerUnpause(void);
bool emuTimerIsPaused(void);
