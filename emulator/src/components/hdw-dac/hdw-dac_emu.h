#pragma once

void dacInitSoundOutput(int numChannelsRec, int numChannelsPlay);
void dacHandleSoundOutput(short* out, int framesp, short numChannels);
