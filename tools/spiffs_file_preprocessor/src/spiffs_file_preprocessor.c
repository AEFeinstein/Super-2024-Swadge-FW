#include <ctype.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "image_processor.h"
#include "font_processor.h"
#include "json_processor.h"
#include "bin_processor.h"
#include "txt_processor.h"
#include "midi_processor.h"
#include "rmd_processor.h"

const char* outDirName = NULL;

void print_usage(void);
bool endsWith(const char* filename, const char* suffix);

/**
 * @brief TODO
 *
 */
void print_usage(void)
{
    printf("Usage:\n  spiffs_file_preprocessor\n    -i INPUT_DIRECTORY\n    -o OUTPUT_DIRECTORY\n");
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
static int processFile(const char* fpath, const struct stat* st __attribute__((unused)), int tflag)
{
    switch (tflag)
    {
        case FTW_F: // file
        {
            if (endsWith(fpath, ".font.png"))
            {
                process_font(fpath, outDirName);
            }
            else if (endsWith(fpath, ".png"))
            {
                process_image(fpath, outDirName);
            }
            else if (endsWith(fpath, ".json"))
            {
                process_json(fpath, outDirName);
            }
            else if (endsWith(fpath, ".bin"))
            {
                process_bin(fpath, outDirName);
            }
            else if (endsWith(fpath, ".txt"))
            {
                process_txt(fpath, outDirName);
            }
            else if (endsWith(fpath, ".mid") || endsWith(fpath, ".midi"))
            {
                process_midi(fpath, outDirName);
            }
            else if (endsWith(fpath, ".rmd"))
            {
                process_rmd(fpath, outDirName);
            }
            break;
        }
        case FTW_D: // directory
        {
            break;
        }
        default:
        // case FTW_SL: // symlink
        case FTW_NS:  // failed
        case FTW_DNR: // failed
        {
            return -1;
        }
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
#if defined(_WIN32)
        mkdir(outDirName);
#elif defined(__linux__) || defined(__CYGWIN__) || defined(__APPLE_)
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
