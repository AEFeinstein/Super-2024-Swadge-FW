#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chart_processor.h"
#include "assets_preprocessor.h"
#include "fileUtils.h"

bool process_chart(processorInput_t* arg);

const assetProcessor_t chartProcessor
    = {.name = "chart", .type = FUNCTION, .function = process_chart, .inFmt = FMT_FILE_BIN, .outFmt = FMT_FILE_BIN};

typedef enum
{
    CS_NONE,
    CS_SONG,
    CS_SYNC_TRACK,
    CS_NOTES,
} charSection_t;

bool process_chart(processorInput_t* arg)
{
    charSection_t section = CS_NONE;
    bool inSection        = false;

    /* Read input file */
    FILE* fp      = arg->in.file;
    FILE* outFile = arg->out.file;

    bool ok            = false;
    uint8_t* tmpSpace  = malloc(1024 * 1024);
    uint32_t tmpIdx    = 0;
    uint32_t noteCount = 0;

    char line[512]        = {0};
    char sectionName[512] = {0};
    int lastTick          = -1;
    int lastNote          = -1;
    while (NULL != fgets(line, sizeof(line), fp))
    {
        int tick, note, length;

        if (sscanf(line, "[%s]", sectionName))
        {
            sectionName[strlen(sectionName) - 1] = 0;

            if (strstr(sectionName, "Song"))
            {
                section = CS_SONG;
            }
            else if (strstr(sectionName, "SyncTrack"))
            {
                section = CS_SYNC_TRACK;
            }
            else
            {
                section = CS_NOTES;
            }
        }
        else if (strstr(line, "{"))
        {
            inSection = true;
        }
        else if (strstr(line, "}"))
        {
            if (CS_NOTES == section)
            {
                fputc((noteCount >> 8) & 0xFF, outFile);
                fputc((noteCount >> 0) & 0xFF, outFile);
                noteCount = 0;
                fwrite(tmpSpace, tmpIdx, 1, outFile);
                tmpIdx = 0;
            }
            inSection = false;
        }
        else if (inSection && (CS_NOTES == section) && //
                 sscanf(line, "%d = N %d %d", &tick, &note, &length))
        {
            if (lastTick == tick && lastNote == note)
            {
                // Don't allow overlapping notes
                continue;
            }
            lastTick = tick;
            lastNote = note;

            noteCount++;

            tmpSpace[tmpIdx++] = (tick >> 24) & 0xFF;
            tmpSpace[tmpIdx++] = (tick >> 16) & 0xFF;
            tmpSpace[tmpIdx++] = (tick >> 8) & 0xFF;
            tmpSpace[tmpIdx++] = (tick >> 0) & 0xFF;

            if (length)
            {
                tmpSpace[tmpIdx++] = note | 0x80;

                tmpSpace[tmpIdx++] = (length >> 8) & 0xFF;
                tmpSpace[tmpIdx++] = (length >> 0) & 0xFF;
            }
            else
            {
                tmpSpace[tmpIdx++] = note;
            }
        }
    }

    ok = true;

    return ok;
}
