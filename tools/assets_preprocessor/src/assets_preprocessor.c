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

/**
 * @brief A mapping of file extensions that should be compressed using heatshrink without any other processing
 *
 */
static const char* rawFileTypes[][2] = {
    {"mid", "mid"},
    {"midi", "mid"},
    {"raw", "raw"},
};

static const assetProcessor_t processors[] = {
    {.inExt = "font.png", .outExt = "font.wsg", .type = FUNCTION, .function = process_font},
    {.inExt = "png",      .outExt = "wsg",      .type = FUNCTION, .function = process_image},
    {.inExt = "chart",    .outExt = "cch",      .type = FUNCTION, .function = process_chart},
    {.inExt = "json",     .outExt = "json",     .type = FUNCTION, .function = process_json},
    {.inExt = "bin",      .outExt = "bin",      .type = FUNCTION, .function = process_bin},
    {.inExt = "txt",      .outExt = "txt",      .type = FUNCTION, .function = process_txt},
    {.inExt = "rmd",      .outExt = "rmh",      .type = FUNCTION, .function = process_rmd},
    {.inExt = "mid",      .outExt = "mid",      .type = FUNCTION, .function = process_raw},
    {.inExt = "midi",     .outExt = "mid",      .type = FUNCTION, .function = process_raw},
    {.inExt = "raw",      .outExt = "raw",      .type = FUNCTION, .function = process_raw},
};

const char* outDirName = NULL;

void print_usage(void);
bool endsWith(const char* filename, const char* suffix);

/**
 * @brief TODO
 *
 */
void print_usage(void)
{
    printf("Usage:\n  assets_preprocessor\n    -i INPUT_DIRECTORY\n    -o OUTPUT_DIRECTORY\n");
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
        char extBuf[16] = {0};
        char outFile[256] = {0};

        for (int i = 0; i < (sizeof(processors) / sizeof(*processors)); i++)
        {
            snprintf(extBuf, sizeof(extBuf), ".%s", processors[i].inExt);
            if (endsWith(inFile, extBuf))
            {
                // This is the matching processor!
                // Calculate the outFile name (replace the extension) and

                strcat(outFile, outDirName);
                strcat(outFile, "/");
                strcat(outFile, get_filename(inFile));

                // Clip off the input file extension
                outFile[strlen(outFile) - strlen(processors[i].inExt)] = '\0';

                // Add the output extension
                strcat(outFile, processors[i].outExt);

                if (!isSourceFileNewer(inFile, outFile))
                {
                    break;
                }
                else if (doesFileExist(outFile))
                {
                    printf("[assets-preprocessor] %s modified! Regenerating %s\n", get_filename(inFile), get_filename(outFile));
                }

                bool result = false;

                switch (processors[i].type)
                {
                    case FUNCTION:
                    {
                        result = processors[i].function(inFile, outFile);
                        break;
                    }

                    case EXEC:
                    {
                        result = false;
                        break;
                    }
                }

                if (!result)
                {
                    fprintf(stderr, "[assets-preprocessor] Error! Failed to process %s!\n", get_filename(inFile));
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
    const char* inDirName = NULL;

    opterr = 0;
    while ((c = getopt(argc, argv, "i:o:")) != -1)
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

    return 0;
}
