#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "fileUtils.h"

/**
 * TODO
 *
 * @param fname
 * @return long
 */
long getFileSize(const char* fname)
{
    FILE* fp = fopen(fname, "rb");
    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fclose(fp);
    return sz;
}

/**
 * @brief TODO
 *
 * @param fname
 * @return true
 * @return false
 */
bool doesFileExist(const char* fname)
{
    int fd = open(fname, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        /* failure */
        if (errno == EEXIST)
        {
            /* the file already existed */
            close(fd);
            return true;
        }
    }

    /* File does not exist */
    close(fd);
    return false;
}

/**
 * @brief Get the filename ext object
 *
 * @param filename
 * @return const char*
 */
const char* get_filename(const char* filename)
{
    const char* slash = strrchr(filename, '/');
    if (!slash || slash == filename)
    {
        return "";
    }
    return slash + 1;
}