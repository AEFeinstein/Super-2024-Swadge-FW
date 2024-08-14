#include "emu_utils.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>

#if defined(EMU_WINDOWS)
    #include <direct.h>
    #define SUPPORT_LINKS 0
#else
    #define SUPPORT_LINKS 1
#endif

/**
 * @brief Create a directory
 *
 * @param path The path of the directory
 * @return int The return value of mkdir or _mkdir, depending on the platform
 */
int makeDir(const char* path)
{
#if defined(EMU_WINDOWS)
    return _mkdir(path);
#else
    return mkdir(path, 0777);
#endif
}

/**
 * @brief Recursively create directories containing a file
 *
 * @param path The path of the file
 * @return true If the directories already exist or were created successfully
 * @return false If the directories do not exist and one or more could not be created
 */
bool makeDirs(const char* path)
{
    char buffer[1024];
    const char* cur = path;

    buffer[0] = '\0';

    if (*cur == '~')
    {
        char* home = getenv("HOME");
        if (home)
        {
            strncpy(buffer, home, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
        }

        cur++;
    }

    // We want to make the base dir first
    do
    {
        // Ignore an empty string
        if (buffer[0])
        {
            struct stat statbuf = {0};
            int statResult      = stat(buffer, &statbuf);
            if (0 == statResult)
            {
                if ((statbuf.st_mode & S_IFREG) == S_IFREG)
                {
                    // File
                    // Can't do anything about that.
                    return false;
                }
#if SUPPORT_LINKS
                else if ((statbuf.st_mode & S_IFLNK) == S_IFLNK)
                {
                    // Symbolic link
                    // Ugh. Whatever.
                    char tmp[1024];
                    strncpy(tmp, buffer, sizeof(tmp));
                    readlink(tmp, buffer, sizeof(buffer) - strlen(buffer) - 1);
                    // printf("Symbolic Link: %s --> %s\n", tmp, buffer);
                }
#endif
                else if ((statbuf.st_mode & S_IFDIR) != S_IFDIR)
                {
                    // Not supported
                    printf("Unknown file type in path: %s (%d)\n", buffer, statbuf.st_mode);
                    return false;
                }
            }
            else
            {
                if (statResult == -1 || statResult == ENOENT)
                {
                    // Doesn't exist! Let's change that
                    if (0 != makeDir(buffer))
                    {
                        printf("Couldn't create directory %s\n", buffer);
                        // Failed
                        return false;
                    }
                }
            }
        }

        // Okay, file checked, advance to next part of path
        if (!*cur)
        {
            break;
        }

        while (*cur == '/')
        {
            int len         = strlen(buffer);
            buffer[len]     = '/';
            buffer[len + 1] = '\0';

            cur++;
        }

        char* next = strchr(cur, '/');

        if (next)
        {
            int length = (next - cur + 1);
            if (length > (sizeof(buffer) - strlen(buffer) - 1))
            {
                length = sizeof(buffer) - strlen(buffer) - 1;
            }

            strncpy(buffer + strlen(buffer), cur, length);

            cur += length;
        }
        else
        {
            // No more slashes, next is the file, so no thanks all is well
            break;
        }
    } while (true);

    return true;
}

void expandPath(char* buffer, size_t length, const char* path)
{
    const char* cur = path;
    char* out       = buffer;

    *out = '\0';

    if (*cur == '~')
    {
        char* home = getenv("HOME");
        if (home)
        {
            while (*home && out < (buffer + length))
            {
                *out++ = *home++;
            }
            *out = '\0';
        }

        cur++;
    }

    while (*cur && out < (buffer + length))
    {
        *out++ = *cur++;
    }
    *out = '\0';
}

/**
 * @brief Write a timestamp-based filename into the given buffer, formatted as "<prefix><timestamp>.<ext>"
 *
 * @param dst The buffer to write the timestamp into
 * @param n The maximum number of characters to write into dst
 * @param prefix The filename prefix to write before the timestamp
 * @param ext The file extension to write after the timestamp and a dot
 * @return const char* A pointer to the beginning of dst
 */
const char* getTimestampFilename(char* dst, size_t n, const char* prefix, const char* ext)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    // Turns out time_t doesn't printf well, so stick it in something that does
    uint64_t timeSec    = (uint64_t)ts.tv_sec;
    uint64_t timeMillis = (uint64_t)ts.tv_nsec / 1000000;

    uint32_t tries = 0;
    do
    {
        snprintf(dst, n, "%s%" PRIu64 "%03" PRIu64 "%s%s", prefix, timeSec, timeMillis, ((ext && *ext) ? "." : ""),
                 ext);
        // Increment millis by one in case the file already exists
        timeMillis++;

        // Might as well handle the edge cases to avoid weird stuff
        if (timeMillis >= 1000000)
        {
            timeMillis %= 1000000;
            timeSec++;
        }

        // If the file exists, keep trying, up to 5 times, then just give up and overwrite it?
    } while (0 == access(dst, R_OK) && ++tries < 5);

    return dst;
}
