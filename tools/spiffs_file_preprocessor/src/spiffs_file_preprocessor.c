#include <unistd.h>
#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__) || defined(__APPLE__)
    #define USE_CRASH_HANDLER 1
#else
    #define USE_CRASH_HANDLER 0
#endif

#if USE_CRASH_HANDLER
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
#include <execinfo.h>
#include <signal.h>
#include <link.h>
#include <dlfcn.h>
#include <stddef.h>
#include <time.h>
#endif

#include <ctype.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "image_processor.h"
#include "font_processor.h"
#include "json_processor.h"
#include "bin_processor.h"
#include "txt_processor.h"
#include "rmd_processor.h"
#include "raw_processor.h"

/**
 * @brief A mapping of file extensions that should be compressed using heatshrink without any other processing
 *
 */
static const char* rawFileTypes[][2] = {
    { "mid",  "mid"},
    { "midi", "mid"},
    { "raw", "raw"},
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
    printf("Usage:\n  spiffs_file_preprocessor\n    -i INPUT_DIRECTORY\n    -o OUTPUT_DIRECTORY\n");
}

#if USE_CRASH_HANDLER
static void init_crashSignals(void);
static void signalHandler_crash(int signum, siginfo_t* si, void* vcontext);
#endif

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
            else if (endsWith(fpath, ".rmd"))
            {
                process_rmd(fpath, outDirName);
            }
            else
            {
                char extBuf[16];
                for (int i = 0; i < (sizeof(rawFileTypes) / sizeof(rawFileTypes[0])); i++)
                {
                    snprintf(extBuf, sizeof(extBuf), ".%s", rawFileTypes[i][0]);

                    if (endsWith(fpath, extBuf))
                    {
                        // printf("Processing %s to raw .%s file\n", fpath, rawFileTypes[i][1]);
                        process_raw(fpath, outDirName, rawFileTypes[i][1]);
                        break;
                    }
                }
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

#if USE_CRASH_HANDLER
    init_crashSignals();
#endif

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
#if defined(WINDOWS) || defined(__WINDOWS__) || defined(_WINDOWS) \
                     || defined(WIN32)       || defined(WIN64) \
                     || defined(_WIN32)      || defined(_WIN64) \
                     || defined(__WIN32__)   || defined(__TOS_WIN__) \
                     || defined(_MSC_VER)
        mkdir(outDirName);
#elif defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__) || defined(__CYGWIN__) || defined(__APPLE__)
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

#if USE_CRASH_HANDLER
/**
 * @brief Initialize a crash handler, only for Linux and MacOS
 */
static void init_crashSignals(void)
{
    const int sigs[] = {SIGSEGV, SIGBUS, SIGILL, SIGSYS, SIGABRT, SIGFPE, SIGIOT, SIGTRAP};
    for (int i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
    {
        struct sigaction action;
        memset(&action, 0, sizeof(struct sigaction));
        action.sa_flags     = SA_SIGINFO;
        action.sa_sigaction = signalHandler_crash;
        sigaction(sigs[i], &action, NULL);
    }
}

/**
 * @brief Print a backtrace when a crash is caught, only for Linux and MacOS
 *
 * @param signum
 * @param si
 * @param vcontext
 */
static void signalHandler_crash(int signum, siginfo_t* si, void* vcontext)
{
    // Get the backtrace
    void* array[128];
    size_t size = backtrace(array, sizeof(array));

    // Only write a file if there's a backtrace
    if (0 < size)
    {
        char msg[512] = {'\0'};
        ssize_t result;

        char fname[64] = {0};
        sprintf(fname, "crash-%ld.txt", time(NULL));
        int dumpFileDescriptor
            = open(fname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

        if (-1 != dumpFileDescriptor)
        {
            snprintf(msg, sizeof(msg), "Signal %-2d received!\nsigno: %-2d\nerrno: %-2d\ncode:  %-2d\n", signum,
                     si->si_signo, si->si_errno, si->si_code);
            result = write(dumpFileDescriptor, msg, strnlen(msg, sizeof(msg)));
            (void)result;

            memset(msg, 0, sizeof(msg));
            strncat(msg, "sifields: ", sizeof(msg) - 1);
    #if defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__)
            for (int i = 0; i < __SI_PAD_SIZE; i++)
    #else
            // Seems to be hardcoded on MacOS
            for (int i = 0; i < 7; i++)
    #endif
            {
                char tmp[8];
    #if defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__)
                snprintf(tmp, sizeof(tmp), "%02X ", si->_sifields._pad[i]);
    #else
                snprintf(tmp, sizeof(tmp), "%02X ", (int)si->__pad[i]);
    #endif
                tmp[sizeof(tmp) - 1] = '\0';
                strncat(msg, tmp, sizeof(msg) - strlen(msg) - 1);
            }
            strncat(msg, "\n", sizeof(msg) - strlen(msg) - 1);
            result = write(dumpFileDescriptor, msg, strnlen(msg, sizeof(msg)));
            (void)result;

            /* This dumps the backtrace to a file, but it doesn't resolve addresses to function names */
            // backtrace_symbols_fd(array, size, dumpFileDescriptor);
            // result = write(dumpFileDescriptor, msg, strnlen(msg, sizeof(msg)));
            // (void)result;

            // Boolean to write the header first
            bool catHdr = false;

            // For each address in the stack trace
            for (size_t i = 0; i < size; i++)
            {
                // Get more information about the address
                Dl_info dli;
                dladdr(array[i], &dli);

                // If the addr2line header isn't written yet
                if (!catHdr)
                {
                    // Write it
                    snprintf(msg, sizeof(msg) - 1, "addr2line -fpriCe %s ", dli.dli_fname);
                    catHdr = true;
                }

                // Calculate the offset relative to the file
                char sign;
                ptrdiff_t offset;
                if (array[i] >= (void*)dli.dli_fbase)
                {
                    sign   = '+';
                    offset = array[i] - dli.dli_fbase;
                }
                else
                {
                    sign   = '-';
                    offset = dli.dli_fbase - array[i];
                }

                // Concatenate each address
                char tmp[32] = {0};
                snprintf(tmp, sizeof(tmp) - 1, "%c%#tx ", sign, offset);
                strncat(msg, tmp, sizeof(msg) - strlen(msg) - 1);
            }

            // Execute addr2line and write the output to the logfile and the terminal
            FILE* fp;
            char path[128];
            fp = popen(msg, "r");
            if (fp != NULL)
            {
                // Print this to the terminal and file
                snprintf(msg, sizeof(msg) - 1, "\nCRASH BACKTRACE\n");
                write(dumpFileDescriptor, msg, strlen(msg));
                printf("%s", msg);

                snprintf(msg, sizeof(msg) - 1, "===============\n");
                write(dumpFileDescriptor, msg, strlen(msg));
                printf("%s", msg);

                /* Read the output a line at a time - output it. */
                while (fgets(path, sizeof(path), fp) != NULL)
                {
                    write(dumpFileDescriptor, path, strlen(path));
                    printf("%s", path);
                }

                /* Flush the terminal */
                fflush(stdout);

                /* close */
                pclose(fp);
            }

            // Close the file
            close(dumpFileDescriptor);
        }
    }

    // Exit
    _exit(1);
}

#endif