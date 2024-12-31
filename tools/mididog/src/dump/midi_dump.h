#pragma once

#include <stdio.h>

#include "midiFileParser.h"

#define MD_DUMP_MULTILINE 1

bool action_dump(void);

void fprintEvent(FILE* stream, int flags, const midiEvent_t* event);
void printEvent(const midiEvent_t* event);
