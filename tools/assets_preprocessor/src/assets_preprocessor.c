#include <ctype.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fileUtils.h"

#include "chart_processor.h"
#include "image_processor.h"
#include "font_processor.h"
#include "json_processor.h"
#include "bin_processor.h"
#include "txt_processor.h"
#include "rmd_processor.h"
#include "raw_processor.h"

#include "assets_preprocessor.h"

static const fileProcessorMap_t fileHandlerMap[] = {
    {.inExt = "font.png", .outExt = "font", .processor = &fontProcessor},
    {.inExt = "png", .outExt = "wsg", .processor = &imageProcessor},
    {.inExt = "chart", .outExt = "cch", .processor = &chartProcessor},
    {.inExt = "json", .outExt = "json", .processor = &jsonProcessor},
    {.inExt = "txt", .outExt = "txt", .processor = &textProcessor},
    {.inExt = "rmd", .outExt = "rmh", .processor = &rmdProcessor},
    {.inExt = "mid", .outExt = "mid", .processor = &heatshrinkProcessor},
    {.inExt = "midi", .outExt = "mid", .processor = &heatshrinkProcessor},
    {.inExt = "raw", .outExt = "raw", .processor = &heatshrinkProcessor},
    {.inExt = "bin", .outExt = "bin", .processor = &binProcessor},
};

const char* outDirName = NULL;
int filesUpdated       = 0;
int processingErrors   = 0;
bool verbose           = false;

void print_usage(void);
bool endsWith(const char* filename, const char* suffix);

/**
 * @brief TODO
 *
 */
void print_usage(void)
{
    printf("Usage:\n  assets_preprocessor\n    -i INPUT_DIRECTORY\n    -o OUTPUT_DIRECTORY\n    [-t "
           "TIMESTAMP_FILE_OUTPUT]\n    [-v]\n");
}

/**
 * @brief TODO
 *
 * @param filename
 * @param suffix
 * @return true
 * @return false
 */
bool endsWith(const char* filename, const char* suffix)
{
    if (strlen(suffix) > strlen(filename))
    {
        return false;
    }
    return 0 == strcmp(&(filename[strlen(filename) - strlen(suffix)]), suffix);
}

/**
 * @brief TODO
 *
 * @param fpath
 * @param st
 * @param tflag
 * @return int
 */
static int processFile(const char* inFile, const struct stat* st __attribute__((unused)), int tflag)
{
    if (FTW_F == tflag)
    {
        char extBuf[16]   = {0};
        char outFile[256] = {0};

        for (int i = 0; i < (sizeof(fileHandlerMap) / sizeof(*fileHandlerMap)); i++)
        {
            const fileProcessorMap_t* extMap  = &fileHandlerMap[i];
            const assetProcessor_t* processor = extMap->processor;

            snprintf(extBuf, sizeof(extBuf), ".%s", extMap->inExt);
            if (endsWith(inFile, extBuf))
            {
                // This is the matching processor!
                // Calculate the outFile name (replace the extension) and

                strcat(outFile, outDirName);
                // strcat(outFile, "/");
                strcat(outFile, get_filename(inFile));

                // Clip off the input file extension
                outFile[strlen(outFile) - strlen(extMap->inExt)] = '\0';

                // Add the output extension
                strcat(outFile, extMap->outExt);

                if (!isSourceFileNewer(inFile, outFile))
                {
                    if (verbose)
                    {
                        printf("[%s] SKIP %s -> %s\n", extMap->inExt, get_filename(inFile), outFile);
                    }
                    break;
                }
                else if (doesFileExist(outFile))
                {
                    printf("[assets-preprocessor] %s modified! Regenerating %s\n", get_filename(inFile),
                           get_filename(outFile));
                }
                filesUpdated++;

                bool result    = false;
                bool readError = false;

                if (FUNCTION == processor->type)
                {
                    FILE* inHandle             = NULL;
                    FILE* outHandle            = NULL;
                    processorFileData_t inData = {0};

                    switch (processor->inFmt)
                    {
                        case FMT_FILE:
                        case FMT_TEXT:
                        case FMT_LINES:
                        {
                            inHandle = fopen(inFile, "r");
                            break;
                        }

                        case FMT_FILE_BIN:
                        case FMT_DATA:
                        {
                            inHandle = fopen(inFile, "rb");
                            break;
                        }
                    }

                    if (!inHandle)
                    {
                        fprintf(stderr, "[%s] FAILED! Cannot open input file '%s'\n", extMap->inExt, inFile);
                        break;
                    }

                    switch (processor->outFmt)
                    {
                        case FMT_FILE:
                        case FMT_TEXT:
                        case FMT_LINES:
                        {
                            outHandle = fopen(outFile, "w");
                            break;
                        }

                        case FMT_FILE_BIN:
                        case FMT_DATA:
                        {
                            outHandle = fopen(outFile, "wb");
                            break;
                        }
                    }

                    if (!outHandle)
                    {
                        fprintf(stderr, "[%s] FAILED! Cannot open output file '%s'\n", extMap->inExt, outFile);
                        break;
                    }

                    // Input and output files have been opened!
                    // Now, handle any extra processing for the input:
                    switch (processor->inFmt)
                    {
                        case FMT_FILE:
                        case FMT_FILE_BIN:
                        {
                            inData.file = inHandle;
                            break;
                        }

                        case FMT_TEXT:
                        case FMT_DATA:
                            // Open file, read text
                            {
                                bool binFile = (processor->inFmt == FMT_DATA);
                                fseek(inHandle, 0L, SEEK_END);
                                long size = ftell(inHandle);
                                fseek(inHandle, 0L, SEEK_SET);

                                char* data = malloc(size + (binFile ? 0 : 1));

                                if (!data)
                                {
                                    readError = true;
                                    break;
                                }

                                fread(data, size, 1, inHandle);

                                if (binFile)
                                {
                                    inData.data   = (uint8_t*)data;
                                    inData.length = size;
                                }
                                else
                                {
                                    data[size]      = '\0';
                                    inData.text     = data;
                                    inData.textSize = size + 1;
                                }
                                break;
                            }

                        case FMT_LINES:
                        {
                            int lines = 0;
                            int last  = 0;
                            int ch    = 0;
                            long size = 0;
                            while (-1 != (ch = getc(inHandle)))
                            {
                                switch (ch)
                                {
                                    case '\n':
                                    {
                                        lines++;
                                        break;
                                    }

                                    default:
                                        break;
                                }

                                last = ch;
                                size++;
                            }

                            // Handle when a file doesn't end with a newline
                            if ('\n' != last)
                            {
                                lines++;
                            }

                            // Go back to the beginning for real reading
                            fseek(inHandle, 0L, SEEK_SET);

                            char* data = (char*)malloc(size + 1);
                            if (!data)
                            {
                                readError = true;
                                break;
                            }

                            char** lineList = malloc(lines * sizeof(char*));
                            if (!lineList)
                            {
                                free(data);
                                readError = true;
                                break;
                            }
                            fread(data, size, 1, inHandle);
                            fclose(inHandle);

                            int outLine     = 0;
                            char* cur       = data;
                            const char* end = data + size;
                            char* lineStart = cur;
                            while (cur < end)
                            {
                                switch (*cur)
                                {
                                    case '\r':
                                    {
                                        if (cur + 1 < end && *(cur + 1) == '\n')
                                        {
                                            *cur = '\0';
                                        }
                                        break;
                                    }

                                    case '\n':
                                    {
                                        *cur                = '\0';
                                        lineList[outLine++] = lineStart;
                                        lineStart           = NULL;

                                        break;
                                    }

                                    default:
                                    {
                                        if (!lineStart)
                                        {
                                            lineStart = cur;
                                        }
                                    }
                                }
                                cur++;
                            }
                            *cur = '\0';

                            inData.lines     = lineList;
                            inData.lineCount = lines;
                            break;
                        }
                    }

                    processorInput_t arg = {
                        .in         = inData,
                        .out        = {.file = outHandle},
                        .inFilename = get_filename(inFile),
                        .data       = NULL,
                    };

                    if (!readError)
                    {
                        result = processor->function(&arg);
                        if (verbose)
                        {
                            printf("[%s] FUNC %s -> %s\n", extMap->inExt, arg.inFilename, outFile);
                        }
                    }

                    fclose(inHandle);

                    switch (processor->outFmt)
                    {
                        case FMT_FILE:
                        case FMT_FILE_BIN:
                            // Nothing else necessary
                            break;

                        case FMT_DATA:
                        {
                            fwrite(arg.out.data, arg.out.length, 1, outHandle);

                            if ((processor->inFmt != FMT_DATA || arg.out.data != arg.in.data)
                                && (processor->inFmt != FMT_TEXT || (void*)arg.out.data != (void*)arg.in.text))
                            {
                                free(arg.out.data);
                            }
                            break;
                        }

                        case FMT_TEXT:
                        {
                            fwrite(arg.out.text, strlen(arg.out.text), 1, outHandle);

                            if ((processor->inFmt != FMT_TEXT || arg.out.text != arg.in.text)
                                && (processor->inFmt != FMT_DATA || (void*)arg.out.text != (void*)arg.in.data))
                            {
                                free(arg.out.text);
                            }
                            break;
                        }

                        case FMT_LINES:
                        {
                            for (size_t n = 0; n < arg.out.lineCount; n++)
                            {
                                fwrite(arg.out.lines[n], strlen(arg.out.lines[n]), 1, outHandle);
                                putc('\n', outHandle);
                            }

                            if (processor->inFmt != FMT_LINES || arg.out.lines != arg.in.lines)
                            {
                                free(arg.out.lines[0]);
                                free(arg.out.lines);
                            }
                            break;
                        }
                    }

                    fclose(outHandle);

                    // And clean up the input file however necessary
                    switch (processor->inFmt)
                    {
                        case FMT_FILE:
                        case FMT_FILE_BIN:
                        {
                            break;
                        }

                        case FMT_DATA:
                        {
                            free(arg.in.data);
                            break;
                        }

                        case FMT_TEXT:
                        {
                            free(arg.in.text);
                            break;
                        }

                        case FMT_LINES:
                        {
                            free(arg.in.lines[0]);
                            free(arg.in.lines);
                            break;
                        }

                        default:
                            break;
                    }
                }
                else if (EXEC == processor->type)
                {
                    // 2048 chars ought to be enough for anybody!!
                    char buf[2048];
                    bool escaped = false;
                    bool quoted  = false;
                    char* out    = buf;

                    const char* cur = processor->exec;
                    while (*cur)
                    {
                        switch (*cur)
                        {
                            case '%':
                            {
                                const char* substStr = NULL;
                                cur++;
                                switch (*cur)
                                {
                                    // %i -> input file path
                                    case 'i':
                                        substStr = inFile;
                                        break;
                                    // %f -> input file name
                                    case 'f':
                                        substStr = get_filename(inFile);
                                        break;
                                    // %o -> output file path
                                    case 'o':
                                        substStr = outFile;
                                        break;
                                    // %a -> input file extension
                                    case 'a':
                                        substStr = extMap->inExt;
                                        break;
                                    // %b -> output file extension
                                    case 'b':
                                        substStr = extMap->outExt;
                                        break;
                                    // %% -> % (escape)
                                    case '%':
                                    {
                                        *out++ = *cur;
                                        break;
                                    }
                                    default:
                                    {
                                        *out++ = '%';
                                        *out++ = *cur;
                                        break;
                                    }
                                }
                                if (substStr)
                                {
                                    out = stpcpy(out, substStr);
                                }
                                break;
                            }

                            default:
                            {
                                *out++ = *cur;
                            }
                            break;
                        }
                        cur++;
                    }
                    *out = '\0';

                    if (verbose)
                    {
                        printf("[%s] EXEC %s -> %s\n", extMap->inExt, get_filename(inFile), outFile);
                        printf(" >>> %s\n", buf);
                    }

                    result = (0 == system(buf));

                    if (!result)
                    {
                        fprintf(stderr, "Command failed!!!\n");
                    }
                }


                if (!result)
                {
                    fprintf(stderr, "[assets-preprocessor] Error! Failed to process %s!\n", get_filename(inFile));
                    processingErrors++;
                }

                break;
            }
        }
    }
    else if (FTW_D != tflag)
    {
        return -1;
    }

    return 0;
}

static int writeTimestampFile(const char* fpath)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    // Turns out time_t doesn't printf well, so stick it in something that does
    uint64_t timeSec    = (uint64_t)ts.tv_sec;
    uint64_t timeMillis = (uint64_t)ts.tv_nsec / 1000000;

    FILE* out = fopen(fpath, "w");

    if (NULL != out)
    {
        fprintf(out, "%" PRIu64 "%03" PRIu64 "\n", timeSec, timeMillis);

        fclose(out);

        return 0;
    }

    return -1;
}

/**
 * @brief TODO
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char** argv)
{
    int c;
    const char* inDirName         = NULL;
    const char* timestampFileName = NULL;

    opterr = 0;
    while ((c = getopt(argc, argv, "i:o:t:v")) != -1)
    {
        switch (c)
        {
            case 'i':
            {
                inDirName = optarg;
                break;
            }
            case 'o':
            {
                outDirName = optarg;
                break;
            }
            case 't':
            {
                timestampFileName = optarg;
                break;
            }
            case 'v':
            {
                verbose = true;
                break;
            }
            default:
            {
                fprintf(stderr, "Invalid argument %c\n", c);
                print_usage();
                return -1;
            }
        }
    }

    if (NULL == inDirName || NULL == outDirName)
    {
        fprintf(stderr, "Failed to provide all arguments\n");
        print_usage();
        return -1;
    }

    // Create output directory if it doesn't exist
    struct stat st = {0};
    if (stat(outDirName, &st) == -1)
    {
#if defined(WINDOWS) || defined(__WINDOWS__) || defined(_WINDOWS) || defined(WIN32) || defined(WIN64) \
    || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(_MSC_VER)
        mkdir(outDirName);
#elif defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__) || defined(__CYGWIN__) \
    || defined(__APPLE__)
        mkdir(outDirName, 0777);
#endif
    }

    if (ftw(inDirName, processFile, 99) == -1)
    {
        fprintf(stderr, "Failed to walk file tree\n");
        return -1;
    }

    if (NULL != timestampFileName && filesUpdated > 0)
    {
        if (0 != writeTimestampFile(timestampFileName))
        {
            fprintf(stderr, "Failed to write timestamp to '%s'!\n", timestampFileName);
            return -1;
        }
    }

    if (processingErrors > 0)
    {
        fprintf(stderr, "[assets-preprocessor] %d file%s failed to process!!\n", processingErrors,
                (processingErrors == 1) ? "" : "s");
        return 1;
    }

    if (filesUpdated > 0)
    {
        printf("[assets-preprocessor] %d file%s updated!\n", filesUpdated, (filesUpdated == 1) ? "" : "s");
    }
    return 0;
}
