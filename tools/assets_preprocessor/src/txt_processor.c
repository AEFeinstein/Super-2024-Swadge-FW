#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "txt_processor.h"
#include "heatshrink_encoder.h"
#include "fileUtils.h"

long remove_chars(char* str, long len, char c);
bool process_txt(processorInput_t* arg);

const assetProcessor_t textProcessor =
{
    .name = "text",
    .type = FUNCTION,
    .function = process_txt,
    .inFmt = FMT_TEXT,
    .outFmt = FMT_TEXT,
};

/**
 * @brief Removes all instances of a given char from a string. Modifies the string in-place and sets a new null
 * terminator, if needed. Also strips non-ASCII characters
 *
 * @param str string to remove chars from
 * @param len number of chars in the string, including null terminator
 * @param c char to remove
 * @return long number of chars in the new string, including null terminator
 */
long remove_chars(char* str, long len, char c)
{
    char *strReadPtr = str, *strWritePtr = str;
    long newLen = 1;
    for (long i = 0; i < len && *strReadPtr; i++)
    {
        *strWritePtr = *strReadPtr++;
        // Keep only printable characters, newlines, and everything except c
        bool keep = (*strWritePtr != c) && (isprint(*strWritePtr) || '\n' == *strWritePtr);
        if (keep)
        {
            newLen++;
            strWritePtr++;
        }
    }
    *strWritePtr = '\0';

    return newLen;
}

bool process_txt(processorInput_t* arg)
{
    /* Read input file */
    long newSz = remove_chars(arg->in.text, arg->in.textSize, '\r');
    arg->out.text = arg->in.text;
    arg->out.textSize = newSz;

    return true;
}
