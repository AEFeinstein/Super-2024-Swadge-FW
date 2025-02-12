#include "fileUtils.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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
    return 0 == access(fname, F_OK);
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


/**
 * @brief Returns true if the file `sourceFile` has a last-modified time after that of `destFile`, or if `destFile` does not exist.
 * 
 * @param sourceFile The path to the "source" file, from which `destFile` is generated
 * @param destFile The path to the "destination" file path, which should be regenerated if older than `sourceFile`.
 * @return true sourceFile was modified after destFile, so destFile should be updated
 * @return false destFile was modified after sourceFile, so destFile does not need to be updated
 */
bool isSourceFileNewer(const char* sourceFile, const char* destFile)
{

    long long srcMtime = 0;
    long long destMtime = 0;

    // Just use stat()
    struct stat statVal = {0};
    errno = 0;
    int statResult = stat(sourceFile, &statVal);
    if (statResult == 0)
    {
        srcMtime = statVal.st_mtime;
    }
    else if (errno != ENOENT)
    {
        fprintf(stderr, "Cannot stat() file %s: %s (%d)\n", sourceFile, strerror(errno), errno);
    }

    memset(&statVal, 0, sizeof(struct stat));
    errno = 0;
    statResult = stat(destFile, &statVal);
    if (statResult == 0)
    {
        destMtime = statVal.st_mtime;
    }
    else if (errno != ENOENT)
    {
        fprintf(stderr, "Cannot stat() file %s: %s (%d)\n", destFile, strerror(errno), errno);
    }

    return srcMtime > destMtime;
}