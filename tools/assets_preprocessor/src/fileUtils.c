#include "fileUtils.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>

#if defined(WINDOWS) || defined(__WINDOWS__) || defined(_WINDOWS) || defined(WIN32) || defined(WIN64) \
    || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(_MSC_VER)
#include <windows.h>
#include <winbase.h>
#endif

static bool parseIni(FILE* file, size_t* count, processorOptions_t* opts, size_t* textLength,  char** text);

/**
 * @brief Return the total size of the given file by opening it and seeking to the end
 *
 * @param fname The path to the file to measure
 * @return long The total size of the file in bytes
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
 * @brief Returns true if the file at the given path exists
 *
 * @param fname The path to the file to check
 * @return true if the file exists
 * @return false if the file does not exist or otherwise cannot be read
 */
bool doesFileExist(const char* fname)
{
    return 0 == access(fname, F_OK);
}

/**
 * @brief Get the filename part of a file path, after the last '/'
 *
 * @param filename The path to return the filename of
 * @return const char* A pointer to the filename within filename
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
 * @brief Returns true if the file `sourceFile` has a last-modified time after that of `destFile`, or if `destFile` does
 * not exist.
 *
 * @param sourceFile The path to the "source" file, from which `destFile` is generated
 * @param destFile The path to the "destination" file path, which should be regenerated if older than `sourceFile`.
 * @return true sourceFile was modified after destFile, so destFile should be updated
 * @return false destFile was modified after sourceFile, so destFile does not need to be updated
 */
bool isSourceFileNewer(const char* sourceFile, const char* destFile)
{
    long long srcMtime  = 0;
    long long destMtime = 0;

    // Just use stat()
    struct stat statVal = {0};
    errno               = 0;
    int statResult      = stat(sourceFile, &statVal);
    if (statResult == 0)
    {
        srcMtime = statVal.st_mtime;
    }
    else if (errno != ENOENT)
    {
        fprintf(stderr, "Cannot stat() file %s: %s (%d)\n", sourceFile, strerror(errno), errno);
    }

    memset(&statVal, 0, sizeof(struct stat));
    errno      = 0;
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

/**
 * @brief Delete the file at the given path in a cross-platform way
 *
 * @param path The file to delete
 * @return true if the file was successfully deleted
 * @return false if there was an error deleting the file
 */
bool deleteFile(const char* path)
{
#if defined(WINDOWS) || defined(__WINDOWS__) || defined(_WINDOWS) || defined(WIN32) || defined(WIN64) \
    || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(_MSC_VER)
        return DeleteFile(path);
#elif defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__) || defined(__CYGWIN__) \
    || defined(__APPLE__)

    errno = 0;
    int result = unlink(path);
    if (0 != result)
    {
        fprintf(stderr, "Failed to delete %s: %s (%d)\n", path, strerror(errno), errno);
    }

    return 0 == result;
#else
    return false;
#endif

}

// Uncomment this to heavily debug the INI file parsing
//#define INI_DEBUG

#ifdef INI_DEBUG
#define iniPrintf(fmt, ...) printf("[ini] " fmt "\n", __VA_ARGS__)
#define iniPuts(str) puts("[ini] " str)
#else
#define iniPrintf(fmt, ...)
#define iniPuts(str)
#endif

/**
 * @brief Performs one of two passes of the parsing of an INI file.
 * The first step should be called with \ref count and \ref textLength set, and with \ref opts
 * and \ref text as NULL. This will calculate the memory size needed to hold the file data. The
 * second step should be called with all arguments passed in, and will actually perform the
 * allocation of memory. Note that if no options
 *
 * @param file
 * @param[in,out] count A pointer that will be set to the number of options found in the file
 * @param[out] opts A pointer to the options that will be filled with the contents of the INI file
 * @param[in,out] textLength A pointer to be set to the size of the buffer required to store the keys and values in this file
 * @param[out] text A pointer to a string to be set to the text buffer allocated for keys and values
 * @return true If the INI file was parsed successfully
 * @return false If the INI file is invalid or another error occurred
 */
static bool parseIni(FILE* file, size_t* count, processorOptions_t* opts, size_t* textLength,  char** text)
{
    if (!file)
    {
        return false;
    }

    bool measuring = (opts == NULL || text == NULL);
    iniPrintf("%s INI file", (measuring ? "Measuring" : "Loading"));

    // pointer to the start of the last section name we wrote
    // so we don't write the same one twice
    char* lastSectionOut = NULL;

    char* textOut = NULL;
    optPair_t* optsOut = NULL;

    const char* textOutEnd = NULL;

    if (!measuring && opts && text && count && textLength)
    {
        if (*textLength != 0)
        {
            iniPrintf("Allocating text buffer of %" PRIu32 " bytes", (uint32_t)*textLength);
            textOut = calloc(1, *textLength);

            if (!textOut)
            {
                return false;
            }

            textOutEnd = textOut + *textLength;
        }
        else
        {
            iniPuts("No text to allocate!");
        }

        if (*count != 0)
        {
            iniPrintf("Allocating opts buffer of %z" PRIu32 " items", (uint32_t)*count);
            optsOut = calloc(*count, sizeof(optPair_t));

            if (!optsOut)
            {
                if (textOut)
                {
                    free(textOut);
                }
                return false;
            }
        }
        else
        {
            iniPuts("No opts to allocate!");
        }
    }

    char* textOutCur = textOut;

#define STATE_PRE_KEY  0
#define STATE_COMMENT  1
#define STATE_KEY      2
#define STATE_POST_KEY 3
#define STATE_PRE_VAL  4
#define STATE_VAL      5
#define STATE_SECTION  6

    size_t textChars = 0;
    size_t pairs = 0;

    char prevSectionName[128] = {0};
    char sectionName[128] = {0};

    char key[128] = {0};
    char val[256] = {0};

    bool escaping = false;

    int state = STATE_PRE_KEY;

    if (measuring)
    {
        lastSectionOut = prevSectionName;
    }

    int ch;
    while (-1 != (ch = getc(file)))
    {
        if (!escaping && ch == '\\')
        {
            escaping = true;
            continue;
        }
        if (ch == '\r')
        {
            // sneaky carriage returnses
            // we hates them!!!
            continue;
        }

        switch (state)
        {
            case STATE_PRE_KEY:
            {
                iniPrintf("PRE_KEY %c", ch);
                if (escaping || isalnum(ch) || ch == '_' || ch == '-' || ch == '.')
                {
                    if (!escaping || ch != '\n')
                    {
                        // Add to the key
                        key[0] = ch;
                        key[1] = '\0';
                        state = STATE_KEY;
                    } else
                    {
                        key[0] = '\0';
                    }
                    escaping = false;
                }
                else if (ch == '[')
                {
                    sectionName[0] = '\0';
                    state = STATE_SECTION;
                }
                else if (ch == ';' || ch == '#')
                {
                    state = STATE_COMMENT;
                }
                // Don't need to handle newlines here because it leads to the same state
                break;
            }

            case STATE_COMMENT:
            {
                iniPrintf("COMMENT %c", ch);
                if (ch == '\n')
                {
                    if (escaping)
                    {
                        // I guess allow escaping and line continues to the
                        escaping = false;
                    }
                    else
                    {
                        state = STATE_PRE_KEY;
                    }
                }
                break;
            }

            case STATE_KEY:
            {
                iniPrintf("KEY %c", ch);
                if (escaping || isalnum(ch) || ch == '_' || ch == '-' || ch == '.')
                {
                    if (!escaping || ch != '\n')
                    {
                        // just don't actually _include_ the newline if it's escaped
                        char* out = key + strlen(key);
                        *out++ = ch;
                        *out++ = '\0';

                        iniPrintf("Key now %s", key);
                    }
                    escaping = false;
                }
                else if (isblank(ch) || ch == ':' || ch == '=')
                {
                    // Key is done
                    if (*sectionName)
                    {
                        // Account for the length of the '<section>.' prefix
                        textChars += strlen(sectionName) + 1;
                    }
                    textChars += strlen(key) + 1;

                    if (isblank(ch))
                    {
                        // we haven't found a : or = yet
                        state = STATE_POST_KEY;
                    }
                    else
                    {
                        // we just found the : or =, skip POST_KEY
                        state = STATE_PRE_VAL;
                    }
                }
                else
                {
                    // ERROR: invalid key
                    return false;
                }

                break;
            }

            case STATE_POST_KEY:
            {
                iniPrintf("POST_KEY %c", ch);
                // We're done with the key and looking for the delimiter
                if (ch == ':' || ch == '=')
                {
                    state = STATE_PRE_VAL;
                }
                else if (ch == '\n')
                {
                    if (!escaping)
                    {
                        // ERROR: key without =value
                        return false;
                    }
                    escaping = false;
                }
                break;
            }

            case STATE_PRE_VAL:
            {
                iniPrintf("PRE_VAL %c", ch);
                if (escaping || (ch != '\n' && ch != ';' && ch != '#' && !isblank(ch)))
                {
                    char* out = &val[strlen(val)];
                    *out++ = ch;
                    *out = '\0';
                    escaping = false;
                    state = STATE_VAL;
                }
                else if (ch == '\n')
                {
                    // ERROR: key= without value
                    return false;
                }
                break;
            }

            case STATE_VAL:
            {
                iniPrintf("VAL  %c", ch);
                if (escaping || (ch != '\n' && ch != ';' && ch != '#'))
                {
                    char* out = &val[strlen(val)];
                    *out++ = ch;
                    *out = '\0';
                    escaping = false;
                }
                else
                {
                    // End of the line
                    // Trim any trailing whitespace
                    char* out = &val[strlen(val)];
                    while (--out >= val)
                    {
                        if (isblank(*out))
                        {
                            *out = '\0';
                        }
                        else
                        {
                            break;
                        }
                    }

                    // Now create the
                    if (measuring)
                    {
                        if (strcmp(lastSectionOut, sectionName))
                        {
                            textChars += strlen(sectionName) + 1;
                            strncpy(prevSectionName, sectionName, sizeof(prevSectionName));
                        }

                        textChars += strlen(key) + 1;
                        textChars += strlen(val) + 1;
                    }
                    else if (optsOut && textOut)
                    {
                        size_t sectionLen = 0;

                        if (!lastSectionOut || strcmp(lastSectionOut, sectionName))
                        {
                            sectionLen += strlen(sectionName) + 2;
                        }

                        iniPrintf("PREK: sect=\"%s\", key=\"%s\", val=\"%s\"", sectionName, key, val);

                        if (textOutCur + sectionLen + strlen(key) + 2 + strlen(val) + 2 > textOutEnd)
                        {
                            // uhh we're out of text room!!!
                            fprintf(stderr, "[ERR] Not enough buffer allocated for text while parsing INI. Exiting early\n");
                            free(textOut);
                            free(optsOut);
                            return false;
                        }

                        if (sectionLen > 0)
                        {
                            optsOut[pairs].section = textOutCur;
                            lastSectionOut = textOutCur;

                            textOutCur = strcpy(textOutCur, sectionName) + strlen(sectionName);
                            *textOutCur++ = '\0';
                        }
                        else
                        {
                            optsOut[pairs].section = lastSectionOut;
                        }

                        optsOut[pairs].name = textOutCur;

                        textOutCur = strcpy(textOutCur, key) + strlen(key);

                        // Leave a space between them
                        *textOutCur++ = '\0';

                        optsOut[pairs].value = textOutCur;
                        textOutCur = strcpy(textOutCur, val) + strlen(val);
                        *textOutCur++ = '\0';

                        iniPrintf("PAIR: key=\"%s\", val=\"%s\"", optsOut[pairs].name, optsOut[pairs].value);

                        key[0] = '\0';
                        val[0] = '\0';

                        // Move into position for the start of the next pair
                    }

                    pairs++;
                    iniPuts("pairs++");

                    if (ch == ';' || ch == '#')
                    {
                        state = STATE_COMMENT;
                    }
                    else
                    {
                        state = STATE_PRE_KEY;
                    }
                }
                break;
            }

            case STATE_SECTION:
            {
                iniPrintf("SECTION %c", ch);
                if (escaping || ch != ']')
                {
                    char* out = &sectionName[strlen(sectionName)];
                    *out++ = ch;
                    *out = '\0';

                    escaping = false;
                }
                else
                {
                    // found a close bracket while not escaping
                    // just pretend we went to a new line already
                    // might be weird if someone has a messed up ini
                    // but they can just like not do that maybe?
                    state = STATE_PRE_KEY;
                }
                break;
            }
        }
    }

    if (measuring)
    {
        iniPrintf("Counted %" PRIu32 " pairs and %" PRIu32 " chars of text buffer", (uint32_t)pairs, (uint32_t)textChars);
        if (count)
        {
            *count = pairs;
        }

        if (textLength)
        {
            *textLength = textChars;
        }
    }
    else
    {
        opts->optionCount = pairs;
        opts->pairs = optsOut;
        *text = textOut;
    }

    fseek(file, 0, SEEK_SET);

    return true;
}

/**
 * @brief Parse the file with the given name as an INI file and write its data to options
 *
 * @param options An uninitialized options array to write INI file data to
 * @param file The path to the INI file to read
 * @return true If the INI file was successfully parsed
 * @return false If there was an error parsing the INI file or it could not be read
 */
bool getOptionsFromIniFile(processorOptions_t* options, const char* file)
{
    FILE* fh = fopen(file, "r");
    if (!fh)
    {
        return false;
    }

    size_t count, textLength;

    bool result = false;

    iniPuts("============================");
    iniPrintf("Loading %s", file);
    iniPuts("============================");

    // Do two passes to avoid bigger allocations than necessary
    if (parseIni(fh, &count, NULL, &textLength, NULL))
    {
        // Don't bother re-parsing the INI file if there are no options in it,
        // even if it's valid!
        if (count > 0)
        {
            char* text = NULL;
            if (parseIni(fh, &count, options, &textLength, &text))
            {
                result = true;
            }
        }
    }
    iniPuts("============================");
    iniPuts("");

    fclose(fh);
    return result;
}

/**
 * @brief Writes the &lt;section&gt;.&lt;key&gt; value to the given buffer and returns the key.
 *
 * If the given option has no section, the original key may be returned without using
 * the buffer.
 *
 * @param buf The buffer to write the key to if necessary
 * @param n The maximum number of characters to write to buf
 * @param option The option to write the key name of
 * @return char* A pointer to buf or option->name
 */
const char* getFullOptionKey(char* buf, size_t n, const optPair_t* option)
{
    if (option->section)
    {
        snprintf(buf, n, "%s.%s", option->section, option->name);
        return buf;
    }
    else
    {
        return option->name;
    }
}

/**
 * @brief Deallocates memory for a processorOptions_t
 *
 * @param options A pointer to the processorOptions_t to delete memory for
 */
void deleteOptions(processorOptions_t* options)
{
    if (options->pairs && options->optionCount > 0)
    {
        char* text = options->pairs[0].section;
        optPair_t* pairs = options->pairs;

        free(text);
        free(pairs);
    }

    options->pairs = NULL;
    options->optionCount = 0;
}

/**
 * @brief Retrieve an option by name and return its value as a string, or NULL if the key is not found
 *
 * @param options The options list to search
 * @param name The key name to search for
 * @return const char* The value of the found key, or NULL
 */
const char* getStrOption(const processorOptions_t* options, const char* name)
{
    if (!options || !options->pairs)
    {
        return NULL;
    }

    char fullKey[256];

    for (const optPair_t* pair = options->pairs; pair < options->pairs + options->optionCount; pair++)
    {
        if (!strcmp(name, getFullOptionKey(fullKey, sizeof(fullKey), pair)))
        {
            return pair->value;
        }
    }

    return NULL;
}

/**
 * @brief Retrieve an option by name from the given options list and parse it as an integer,
 * using a default value if it cannot be found or is not an integer
 *
 * @param options The options list to search
 * @param name The key name to search for
 * @param defaultVal A value to return if the key cannot be found or the value is not an integer
 * @return int The parsed value, or the default value
 */
int getIntOption(const processorOptions_t* options, const char* name, int defaultVal)
{
    if (!options || !options->pairs)
    {
        return defaultVal;
    }

    char fullKey[256];

    for (const optPair_t* pair = options->pairs; pair < options->pairs + options->optionCount; pair++)
    {
        if (!strcmp(name, getFullOptionKey(fullKey, sizeof(fullKey), pair)))
        {
            char* end = NULL;
            int result = strtol(pair->value, &end, 0);

            if (!result && end == pair->value)
            {
                // No valid number parsed
                return defaultVal;
            }

            // Number parsed!
            return result;
        }
    }

    // Key not found
    return defaultVal;
}

/**
 * @brief Retrieve an option by name from the given options list and parse it as a boolean,
 * using a default value if it cannot be found or is not a boolean
 *
 * The following values are considered as `true`: `yes`, `true`, `1`, `y`, `t`, `on`
 * And the following values are considered as `false`: `no`, `false`, `0`, `n`, `f`, `off`
 * Matching is case-insensitive. If the option value matches none of these strings, the
 * default value will be returned instead.
 *
 * @param options The options list to search
 * @param name The name of the key to search
 * @param defaultVal A value to return if the value is not found or not a boolean
 * @return true
 * @return false
 */
bool getBoolOption(const processorOptions_t* options, const char* name, bool defaultVal)
{
    static const char* trueStrs[] = {
        "yes",
        "true",
        "1",
        "y",
        "t",
        "on",
    };
    static const char* falseStrs[] = {
        "no",
        "false",
        "0",
        "n",
        "f",
        "off",
    };

    if (!options || !options->pairs)
    {
        return defaultVal;
    }

    char fullKey[256];

    for (const optPair_t* pair = options->pairs; pair < options->pairs + options->optionCount; pair++)
    {
        if (!strcmp(name, getFullOptionKey(fullKey, sizeof(fullKey), pair)))
        {
            if (!*pair->value)
            {
                // quick return for empty strings
                return defaultVal;
            }

            // search for a 'truthy' value and return true if we find one
            for (int i = 0; i < sizeof(trueStrs) / sizeof(*trueStrs); i++)
            {
                if (!strcasecmp(pair->value, trueStrs[i]))
                {
                    return true;
                }
            }
            for (int i = 0; i < sizeof(falseStrs) / sizeof(*falseStrs); i++)
            {
                if (!strcasecmp(pair->value, falseStrs[i]))
                {
                    return false;
                }
            }

            // no explicitly true/false values matched, so return the default
            return defaultVal;
        }
    }

    // key not found, so return the default
    return defaultVal;
}

/**
 * @brief Searches the given options list for a key with the given name
 *
 * @param options The options list to search
 * @param name The key name to search for
 * @return true if the key was contained in the options list
 * @return false if the key was not found
 */
bool hasOption(const processorOptions_t* options, const char* name)
{
    if (!options || !options->pairs)
    {
        return false;
    }

    char fullKey[256];

    for (const optPair_t* pair = options->pairs; pair < options->pairs + options->optionCount; pair++)
    {
        if (!strcmp(name, getFullOptionKey(fullKey, sizeof(fullKey), pair)))
        {
            return true;
        }
    }

    return false;
}
