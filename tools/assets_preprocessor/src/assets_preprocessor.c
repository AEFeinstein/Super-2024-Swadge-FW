//==============================================================================
// Includes
//==============================================================================

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

#include "assets_preprocessor.h"
#include "fileUtils.h"

//==============================================================================
// Asset Processor Includes
//==============================================================================
// Include your <type>_processor.h file here (alphabetized, please)
//==============================================================================
#include "bin_processor.h"
#include "chart_processor.h"
#include "cfun_processor.h"
#include "font_processor.h"
#include "image_processor.h"
#include "json_processor.h"
#include "raw_processor.h"
#include "sudoku_processor.h"
#include "txt_processor.h"
//==============================================================================
// END Asset Processor Includes
//==============================================================================

//==============================================================================
// Asset Processor List
//==============================================================================
// EDIT HERE to register a new asset processor
//==============================================================================
static const assetProcessor_t* allAssetProcessors[] = {
    &binProcessor,   &chartProcessor, &fontProcessor, &heatshrinkProcessor,
    &imageProcessor, &jsonProcessor,  &sudokuProcessor, &textProcessor,
    &cfunProcessor,
};
//==============================================================================
// END Asset Processor List
//==============================================================================

//==============================================================================
// Variables
//==============================================================================

const char* inDirName                  = NULL;
const char* outDirName                 = NULL;
int filesUpdated                       = 0;
int processingErrors                   = 0;
bool verbose                           = false;
const processorOptions_t* globalConfig = NULL;

static const fileProcessorMap_t* loadedExtMappings = NULL;
static size_t loadedExtMappingCount                = 0;

//==============================================================================
// Function declarations
//==============================================================================

void print_usage(void);
bool startsWith(const char* path, const char* prefix);
bool endsWith(const char* filename, const char* suffix);
static const assetProcessor_t* findProcessor(const char* name);
static void setupConfig(assetProcessor_t* execProcessors, size_t* procCount, fileProcessorMap_t* mappings,
                        size_t* mapCount, const processorOptions_t* options);

//==============================================================================
// Const data
//==============================================================================

static const char optionsFileExtension[] = "opts";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
void print_usage(void)
{
    printf("Usage:\n  assets_preprocessor\n    -i INPUT_DIRECTORY\n    -o OUTPUT_DIRECTORY\n    [-t "
           "TIMESTAMP_FILE_OUTPUT]\n    [-c CONFIG_FILE]\n    [-v]\n");
    printf("\n All Asset processors:\n");
    for (int n = 0; n < sizeof(allAssetProcessors) / sizeof(*allAssetProcessors); n++)
    {
        printf("  - %s\n", allAssetProcessors[n]->name);
    }
    printf("\n");
}

bool startsWith(const char* path, const char* prefix)
{
    if (strlen(prefix) > strlen(path))
    {
        return false;
    }

    return 0 == strncmp(path, prefix, strlen(prefix));
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
        char extBuf[16]           = {0};
        char outFile[256]         = {0};
        char optionsFilename[256] = {0};

        for (size_t i = 0; i < loadedExtMappingCount; i++)
        {
            const fileProcessorMap_t* extMap  = &loadedExtMappings[i];
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

                // Add the output file extension
                strcat(outFile, extMap->outExt);

                // Now, construct the options filename

                // Add the whole input file path to the buffer
                strncpy(optionsFilename, inFile, sizeof(optionsFilename));

                // Clip off the input extension  again
                optionsFilename[strlen(optionsFilename) - strlen(extMap->inExt)] = '\0';

                // And append the options file extension
                strcat(optionsFilename, optionsFileExtension);

                // Now, check if the options filename exists
                bool hasOptions = doesFileExist(optionsFilename);

                if (!hasOptions)
                {
                    // First, just remove the filename and replace it with the opts extension
                    // This should be guaranteed to be inside the assets dir still...
                    char* lastSlash  = strrchr(optionsFilename, '/');
                    *(lastSlash + 1) = '\0';
                    strcat(optionsFilename, ".");
                    strcat(optionsFilename, optionsFileExtension);

                    hasOptions = doesFileExist(optionsFilename);
                    if (!hasOptions)
                    {
                        do
                        {
                            // Trim the first slash, of "/.opts"
                            lastSlash  = strrchr(optionsFilename, '/');
                            *lastSlash = '\0';
                            // Find the next previous slash
                            lastSlash = strrchr(optionsFilename, '/');
                            // Chop the string after it
                            *(lastSlash + 1) = '\0';
                            // And append the options extension
                            strcat(optionsFilename, ".");
                            strcat(optionsFilename, optionsFileExtension);
                            // Then, at the end of the loop, first we make sure the new filename is still inside
                            // the assets directory. We don't want to touch anything outside the input directory!
                            // Next, if that's true, we set hasOptions based on if the file exists and exit if so
                        } while (startsWith(optionsFilename, inDirName)
                                 && !(hasOptions = doesFileExist(optionsFilename)));
                    }
                }

                // And if the options file has been modified since the output was generated,
                // regenerate it the same as though the source file was modified
                bool optionsModified = hasOptions && isSourceFileNewer(optionsFilename, outFile);
                bool inFileModified  = isSourceFileNewer(inFile, outFile);

                if (!inFileModified && !optionsModified)
                {
                    if (verbose)
                    {
                        printf("[%s] SKIP %s -> %s\n", extMap->inExt, get_filename(inFile), get_filename(outFile));
                    }
                    break;
                }
                else if (doesFileExist(outFile))
                {
                    printf("[assets-preprocessor] %s modified! Regenerating %s\n",
                           (!inFileModified) ? (optionsFilename + strlen(inDirName)) : get_filename(inFile),
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
                    processorFileData_t outData = {0};

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
                        case FMT_FILENAME:
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

                    const char * outFileName = NULL;

                    switch (processor->outFmt)
                    {
                        case FMT_FILE:
                        case FMT_TEXT:
                        case FMT_LINES:
                        {
                            outHandle = fopen(outFile, "w");
                            outData = (processorFileData_t){ .file = outHandle };
                            break;
                        }

                        case FMT_FILENAME:
                        {
                            outFileName = outFile;
                            outData = (processorFileData_t){ .fileName = outFileName };
                            break;
                        }

                        case FMT_FILE_BIN:
                        case FMT_DATA:
                        {
                            outHandle = fopen(outFile, "wb");
                            outData = (processorFileData_t){ .file = outHandle };
                            break;
                        }
                    }

                    if (!outHandle && !outFileName)
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

                        case FMT_FILENAME:
                        {
                            inData.fileName = inFile;
                            break;
                        }

                        case FMT_TEXT:
                        case FMT_DATA:
                        {
                            // Open file, read text
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

                    processorOptions_t options = {0};
                    if (hasOptions)
                    {
                        if (getOptionsFromIniFile(&options, optionsFilename))
                        {
                            if (verbose)
                            {
                                printf("[%s] OPTS %s <- %s (%" PRIu32 ")\n", extMap->inExt, get_filename(inFile),
                                       optionsFilename + strlen(inDirName), (uint32_t)options.optionCount);
                            }
                        }
                        else
                        {
                            if (verbose)
                            {
                                fprintf(
                                    stderr,
                                    "[WRN] Options file %s exists but contains no options! Is it a valid INI file?\n",
                                    optionsFilename);
                            }
                            hasOptions = false;
                        }
                    }

                    processorInput_t arg = {.in         = inData,
                                            .out        = outData,
                                            .inFilename = get_filename(inFile),
                                            .options    = hasOptions ? &options : NULL};

                    if (!readError)
                    {
                        result = processor->function(&arg);
                        if (verbose)
                        {
                            printf("[%s] FUNC %s -> %s\n", extMap->inExt, arg.inFilename, get_filename(outFile));
                        }
                    }

                    fclose(inHandle);

                    if (optionsModified)
                    {
                        deleteOptions(&options);
                    }

                    switch (processor->outFmt)
                    {
                        case FMT_FILE:
                        case FMT_FILENAME:
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

                    if (outHandle)
                    {
                        fclose(outHandle);
                    }

                    // And clean up the input file however necessary
                    switch (processor->inFmt)
                    {
                        case FMT_FILE:
                        case FMT_FILE_BIN:
                        case FMT_FILENAME:
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

                    if (readError || !result)
                    {
                        if (!deleteFile(outFile))
                        {
                            fprintf(stderr,
                                    "[WRN] Could not clean up invalid output file %s after failed proecessing\n",
                                    outFile);
                        }
                    }
                }
                else if (EXEC == processor->type)
                {
                    // 2048 chars ought to be enough for anybody!!
                    char buf[2048];
                    char* out = buf;

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
                                    out = strcpy(out, substStr) + strlen(substStr);
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

static const assetProcessor_t* findProcessor(const char* name)
{
    for (int i = 0; i < (sizeof(allAssetProcessors) / sizeof(*allAssetProcessors)); i++)
    {
        const assetProcessor_t* processor = allAssetProcessors[i];

        if (!strcmp(processor->name, name))
        {
            return processor;
        }
    }

    return NULL;
}

static void setupConfig(assetProcessor_t* execProcessors, size_t* procCount, fileProcessorMap_t* mappings,
                        size_t* mapCount, const processorOptions_t* options)
{
    const size_t maxProcs = *procCount;
    const size_t maxMaps  = *mapCount;

    size_t procsOut = 0;
    size_t mapsOut  = 0;

    assetProcessor_t pendingProc  = {0};
    fileProcessorMap_t pendingMap = {0};

    const char* lastSectionName = NULL;

    // Okay, this loop is KINDA ugly...
    const optPair_t* maxOpt = options->pairs + options->optionCount;
    // Notice the <= -- we're going to iterate **past** the last option!
    // Whoa, why would you do that!?
    // Well, it's because you can add the properties in a section in any order
    // And so we don't know ahead of time when the object will be complete.
    // All we can do is wait until the end of the section, then check if the
    // properties we DO have make a valid object.
    // BUT, there's also the end of the file! So to avoid a copy-and-paste
    // or function-ifying of the end-of-section logic, we just do one extra
    // iteration and skip most of the logic for the out-of-bounds option.
    // Okay that probably isn't really wacky enough to warrant this entire
    // paragraph but I already wrote it so oh well!
    for (const optPair_t* opt = options->pairs; opt <= maxOpt; opt++)
    {
        char* sectionName   = opt->section;
        const char* optName = opt->name;

        // Okay so it got a bit weirder since I wrote the last paragraph.
        // Turns out, the pending structs need to be flushed BEFORE we start
        // overwriting their data with the new section. Seems kinda obvious
        // now but yeah, so all this logic is now split into two sections with
        // the start and end checking the actual section and key/value data, and
        // with the middle flushing the structs based on the section name.
        if (opt == maxOpt || !lastSectionName || strcmp(sectionName, lastSectionName))
        {
            if (verbose)
            {
                if (opt == maxOpt)
                {
                    printf("EOF, flushing\n");
                }
                else
                {
                    printf("[%s] != [%s], flushing\n", sectionName, lastSectionName);
                }
            }

            bool validMap = false;

            // This is a different section than before, so "flush" any pending structs that are valid
            if (pendingMap.processor != NULL && pendingMap.outExt != NULL)
            {
                if (pendingMap.inExt != NULL)
                {
                    validMap = true;
                }
                else if (lastSectionName)
                {
                    // Use the section name
                    pendingMap.inExt = lastSectionName;

                    // Trim leading '.'
                    while (*pendingMap.inExt == '.')
                    {
                        pendingMap.inExt++;
                    }

                    // Well, it's valid now
                    validMap = true;
                }
            }

            bool validProc = (pendingProc.exec != NULL || pendingProc.function != NULL) && pendingProc.name != NULL;
            if (validProc && procsOut < maxProcs)
            {
                if (verbose)
                {
                    printf("CONF Loaded new %s processor '%s'\n", (pendingProc.type == FUNCTION) ? "FUNCTION" : "EXEC",
                           pendingProc.name);
                }

                memcpy(&execProcessors[procsOut++], &pendingProc, sizeof(assetProcessor_t));
                memset(&pendingProc, 0, sizeof(assetProcessor_t));
                validProc = false;
            }

            if (validMap && mapsOut < maxMaps)
            {
                if (verbose)
                {
                    printf("CONF Mapped *.%s -> *.%s to processor '%s'\n", pendingMap.inExt, pendingMap.outExt,
                           pendingMap.processor->name);
                }

                memcpy(&mappings[mapsOut++], &pendingMap, sizeof(fileProcessorMap_t));
                memset(&pendingMap, 0, sizeof(fileProcessorMap_t));
                validMap = false;
            }

            lastSectionName = sectionName;

            if (verbose)
            {
                putchar('\n');
            }
        }

        if (opt < maxOpt)
        {
            if (verbose)
            {
                printf("CONF %s = %s\n", opt->name, opt->value);
            }
            const char* keyName = strrchr(optName, '.');
            if (keyName == NULL)
            {
                keyName = optName;
            }
            else
            {
                keyName++;
            }

            // Now, check the key names...
            if (!strcasecmp("outExt", keyName))
            {
                // Out file extension
                pendingMap.outExt = opt->value;

                // Strip off a leading '.' because I'm nice
                while ('.' == *pendingMap.outExt && *(pendingMap.outExt + 1))
                {
                    pendingMap.outExt++;
                }
            }
            else if (!strcasecmp("inExt", keyName))
            {
                pendingMap.inExt = opt->value;

                // Strip off a leading '.' but this time only because I have to
                while ('.' == *pendingMap.inExt && *(pendingMap.inExt + 1))
                {
                    pendingMap.outExt++;
                }
            }
            else if (!strcasecmp("func", keyName))
            {
                // If the processor
                pendingMap.processor = findProcessor(opt->value);
            }
            else if (!strcasecmp("exec", keyName))
            {
                pendingProc.type = EXEC;
                pendingProc.name = opt->value;
                pendingProc.exec = opt->value;
                // We can't set the processor to &pendingProc, because it's on the stack
                // But we know that it WILL be added to the list next, so...
                pendingMap.processor = &execProcessors[procsOut];
            }
            else
            {
                // Unrecognized config key, is this an error?
                fprintf(stderr, "[WRN] Unrecognized config key [%s].%s in config\n", sectionName, keyName);
            }
        }
    }

    if (procCount)
    {
        *procCount = procsOut;
    }

    if (mapCount)
    {
        *mapCount = mapsOut;
    }

    if (verbose)
    {
        printf("CONF Loaded %" PRIu32 " processors and %" PRIu32 " extension mappings\n", (uint32_t)procsOut,
               (uint32_t)mapsOut);
    }
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
    const char* configFile        = NULL;
    const char* timestampFileName = NULL;

    opterr = 0;
    while ((c = getopt(argc, argv, "i:o:t:vc:h")) != -1)
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
            case 'c':
            {
                configFile = optarg;
                break;
            }
            case 'h':
            {
                print_usage();
                return 0;
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

    assetProcessor_t dynamicAssetProcessors[64];
    fileProcessorMap_t dynamicMappings[64];

    const size_t maxProcCount = sizeof(dynamicAssetProcessors) / sizeof(assetProcessor_t);
    const size_t maxMapCount  = sizeof(dynamicMappings) / sizeof(fileProcessorMap_t);

    size_t procCount = maxProcCount;
    size_t mapCount  = maxMapCount;

    processorOptions_t configOptions = {0};
    if (configFile)
    {
        if (getOptionsFromIniFile(&configOptions, configFile))
        {
            globalConfig = &configOptions;

            setupConfig(dynamicAssetProcessors, &procCount, dynamicMappings, &mapCount, globalConfig);
        }
    }

    loadedExtMappings     = dynamicMappings;
    loadedExtMappingCount = mapCount;

    if (ftw(inDirName, processFile, 99) == -1)
    {
        fprintf(stderr, "Failed to walk file tree\n");
        if (globalConfig)
        {
            deleteOptions(&configOptions);
            globalConfig = NULL;
        }
        return -1;
    }

    if (globalConfig)
    {
        deleteOptions(&configOptions);
        globalConfig = NULL;
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
