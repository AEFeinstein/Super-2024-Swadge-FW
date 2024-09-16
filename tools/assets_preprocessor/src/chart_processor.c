#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chart_processor.h"
#include "fileUtils.h"

typedef enum
{
    CS_NONE,
    CS_SONG,
    CS_SYNC_TRACK,
    CS_NOTES,
} charSection_t;

void process_chart(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

    char* dotptr = strrchr(outFilePath, '.');
    snprintf(&dotptr[1], strlen(dotptr), "cch");

    if (doesFileExist(outFilePath))
    {
        // printf("Output for %s already exists\n", infile);
        // return;
    }

    charSection_t section = CS_NONE;
    bool inSection        = false;

    /* Read input file */
    FILE* fp = fopen(infile, "rb");

    if (fp)
    {
        FILE* outFile = fopen(outFilePath, "wb");

        if (outFile)
        {
            char line[512]        = {0};
            char sectionName[512] = {0};
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
                    inSection = false;
                }
                else if (inSection && (CS_NOTES == section) && //
                         sscanf(line, "%d = N %d %d", &tick, &note, &length))
                {
                    fputc((tick >> 24) & 0xFF, outFile);
                    fputc((tick >> 16) & 0xFF, outFile);
                    fputc((tick >> 8) & 0xFF, outFile);
                    fputc((tick >> 0) & 0xFF, outFile);

                    if (length)
                    {
                        fputc(note & 0x80, outFile);
                        fputc((length >> 8) & 0xFF, outFile);
                        fputc((length >> 0) & 0xFF, outFile);
                    }
                    else
                    {
                        fputc(note, outFile);
                    }
                }
            }
            fclose(outFile);
        }
        fclose(fp);
    }
}
