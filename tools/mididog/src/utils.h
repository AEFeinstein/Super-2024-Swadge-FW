#pragma once

#include <stdio.h>
#include <stdbool.h>

#include "midiFileParser.h"

midiFile_t* mididogLoadFile(FILE* stream);
midiFile_t* mididogLoadPath(const char* path);

bool mididogWriteFile(const midiFile_t* data, FILE* stream);
bool mididogWritePath(const midiFile_t* data, const char* path);

midiFile_t* mididogTokenizeMidi(const midiFile_t* midiFile);
midiFile_t* mididogUnTokenizeMidi(const midiFile_t* midiFile);

int writeNoteName(char* output, int max, int flags, uint8_t note);