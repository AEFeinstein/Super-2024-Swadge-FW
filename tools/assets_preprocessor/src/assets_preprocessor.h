/*
    const char* inExt: The extension of the file to be transformed, e.g. png
    const char* outExt: The extension of the file that will be created, e.g. .wsg
    enum processType_t type: The type of processing the file gets. Right now would just be FUNCTION, but maybe we'd have
   another for EXEC (to use for external tools) bool (*processFn)(const char* inFile, const char* outFile): If type is
   FUNCTION, this would be a pointer to the function called to process the file. The in and out file paths are already
   set up, so no filename parsing is needed inside the handler. (if EXEC is implemented) const char* execCmd: The
   command to run for processing this file. If it exits abnormally the file is considered failed. The in and out file
   arguments would be populated in string arguments, so "cp %1$s %2$s" would be a valid one. This might be a good
   uniform way to handle things like the sokoban python script.
*/

#ifndef _ASSETS_PREPROCESSOR_H_
#define _ASSETS_PREPROCESSOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

/*! \file assets_preprocessor.h
 *
 * \section assetProc_design Design Philosophy
 * 
 * The asset preprocessor is used to take asset files from their original formats and
 * convert them into formats designed to work with the Swadge. This makes it possible
 * to save assets in the repository using popular and well-supported file formats,
 * such as PNG for images, but to use a custom format that's more Swadge-friendly when
 * the data is loaded. This might involve compressing the data using heatshrink,
 * excluding data from the original asset that the Swadge does not use, or validating
 * that the asset file is properly formatted.
 * 
 * Each asset processor handles a specific type of file. Some processors, like
 * raw_processor.h and bin_processor.h, are generic and can be used by multiple file
 * extensions. An asset processor can be mapped to any number of pairs of file input
 * and output extensions. Multiple input extensions can even be mapped to the
 * same extension.
 *
 * 
 * \section assetProc_capabilities
 * Before adding a new asset type, it's helpful to know what the capabilities of an asset
 * processor are. Most asset processors are just a function that takes input file data and
 * writes output file data, and returns true on success or false on failure. Asset processors
 * will receive a single argument with file data passed as either a file handle, an array
 * of bytes, a null-terminated string, or a list of lines of text. Each asset processor
 * can configure one of these formats for input data, and one for output data, based on
 * whichever is most convenient for the type of data being processed. The asset processor
 * function returns the output file either by writing to the output handle, or allocating
 * a byte array or string and returning it via modifying the argument's out member. Output
 * data returned this way will automatically be freed; the function should not close the
 * input or output file handles or free the input file data. The function is also free to
 * directly modify the input file data buffers and to assign the input data to the output
 * argument's data pointer.
 * 
 * \subsection assetProc_cap_ex Function Processor Examples
 * 
 * \code{.c}
 * #include "assets_preprocessor.h"
 * 
 * bool fileInFileOutFunc(assetProcessorInput_t* arg)
 * {
 *     // Read the input file straight into the output file, but replace 'e' with 'o'
 *     int ch;
 *     while (-1 != (getch(args->in.file)))
 *     {
 *         if (ch == 'e')
 *         {
 *             // No, it's an O now!
 *             putc('o', args->out.file);
 *         }
 *         else
 *         {
 *             // Output the input character
 *             putc(ch, args->out.file);
 *         }
 *     }
 *     return true;
 * }
 * 
 * const assetProcessor_t fileInFileOutProcessor = {
 *     .type = FUNCTION,
 *     .function = fileInFileOutFunc,
 *     .inFmt = FMT_FILE,
 *     .outFmt = FMT_FILE,
 * };
 * 
 * bool dataInDataOutFunc(assetProcessorInput_t* arg)
 * {
 *     // Modify the input buffer and return it to the output
 *     // Replace any zeroes with 255s
 *     for (size_t n = 0; n < arg->in.length; n++)
 *     {
 *         uint8_t curData = arg->in.data[n];
 *         if (curData == 0)
 *         {
 *             arg->in.data[n] = 0xFF;
 *         }
 *     }
 * 
 *     arg->out.data = arg->in.data;
 *     arg->out.length = arg->in.length;
 * 
 *     return true;
 * }
 * 
 * const assetProcessor_t dataInDataOutProcessor = {
 *     .type = FUNCTION,
 *     .function = dataItDataOutFunc,
 *     .inFmt = FMT_DATA,
 *     .outFmt = FMT_DATA,
 * };
 * 
 * bool textInDataOutFunc(assetProcessorInput_t* arg)
 * {
 *     int items = 0;
 *     char* cur = arg->in.text;
 *     while (*cur)
 *     {
 *         if (*cur == ',')
 *         {
 *             items++;
 *         }
 * 
 *         cur++;
 *     }
 * 
 *     // Don't forget the last item
 *     if (cur > arg->in.text && *(cur-1) != ',')
 *     {
 *         items++;
 *     }
 * 
 *     size_t dataSize = items * sizeof(int);
 *     uint8_t* itemData = malloc(dataSize);
 * 
 *     if (!itemData)
 *     {
 *         // Malloc error!
 *         return false;
 *     }
 *     
 *     uint8_t* out = itemData;
 *     while (*cur)
 *     {
 *         if (*cur == ',')
 *         {
 *             cur++;
 *             continue;
 *         }
 * 
 *         char* endptr = NULL;
 *         int result = strtol(cur, &endptr, 10);
 *         if (!result && endPtr == cur)
 *         {
 *             // Invalid data format, expecting a number!
 *             return false;
 *         }
 * 
 *         *out++ = (result >> 24) & 0xFF;
 *         *out++ = (result >> 16) & 0xFF;
 *         *out++ = (result >> 8) & 0xFF;
 *         *out++ = (result) & 0xFF;
 * 
 *         cur = endptr;
 *     }
 * 
 *     // It's OK to return the malloc()'d data, it will be freed
 *     arg->out.data = itemData;
 *     arg->out.length = dataSize;
 * 
 *     return true;
 * }
 * 
 * const assetProcessor_t textInDataOutProcessor = {
 *     .type = FUNCTION,
 *     .function = textInDataOutFunc,
 *     .inFmt = FMT_TEXT,
 *     .outFmt = FMT_DATA
 * };
 * 
 * bool linesInDataOutFunc(assetProcessorInput_t* arg)
 * {
 *     for (int i = 0; i < arg->in.lineCount; i++)
 *     {
 *         errno = 0;
 *         int val = strtol(arg->in.lines[i], NULL, 10);
 *         if (0 != errno || val > 255 || val < 0)
 *         {
 *             return false;
 *         }
 * 
 *         putc(val & 0xFF, arg->out.file);
 *     }
 * 
 *     return true;
 * }
 * 
 * const assetProcessor_t linesInFileOutProcessor = {
 *     .type = FUNCTION,
 *     .function = linesInDataOutFunc,
 *     .inFmt = FMT_LINES,
 *     .outFmt = FMT_FILE_BIN
 * };
 * 
 * \endcode
 * 
 * 
 * \section assetProc_adding
 * To create a new asset processor, follow these steps.
 * 
 * 1. Create new `<type>_processor.h` and `.c` files in /tools/assets_preprocessor/src/  
 * (see below for example file)
 * 
 * 2. Edit fileProcessorMap in assets_preprocessor.c to 
 * 
 */

/**
 * @brief Specifies which type of asset processor is being defined
 * 
 */
typedef enum
{
    /// @brief Processor that calls a function pointer to process assets
    FUNCTION,

    /// @brief Processor that calls an external executable to process assets
    EXEC,
} processorType_t;

/**
 * @brief The format that this asset processor accepts or returns its data in
 *
 */
typedef enum
{
    /// @brief An opened file handle is passed for the file
    FMT_FILE,
    /// @brief An opened file handle (in binary mode) is passed for the file
    FMT_FILE_BIN,
    /// @brief Raw binary data is read from this file and passed
    FMT_DATA,
    /// @brief Raw string data is read from this file and passed
    FMT_TEXT,
    /// @brief String data is separated into lines and passed as an array
    FMT_LINES,
} processorFormat_t;

/**
 * @brief Holds processor input or output data
 *
 */
typedef union
{
    FILE* file;
    struct {
        uint8_t* data;
        size_t length;
    };
    struct {
        char** lines;
        size_t lineCount;
    };
    struct {
        char* text;
        size_t textSize;
    };
} processorFileData_t;

/**
 * @brief Holds the input and output data for a single file processing operation
 *
 * Note that the processor must only use the input/output formats it was configured for
 *
 */
typedef struct
{
    /// @brief Holds the input data in whichever format was configured for the processor
    const processorFileData_t in;

    /// @brief Holds the output data in whichever format was configured for the processor
    processorFileData_t out;

    /// @brief Holds the input filename for convenience and error reporting
    const char* inFilename;

    /// @brief Holds a pointer to any extra processor-specific data.
    void* data;
} processorInput_t;

typedef bool (*processFn_t)(processorInput_t* arg);

typedef struct
{
    /// @brief The format this processor accepts its input data in. Ignored for exec
    processorFormat_t inFmt;

    /// @brief The format this processor returns its output data in. Ignored for exec
    processorFormat_t outFmt;

    /// @brief The type of this asset processor
    processorType_t type;

    union
    {
        /// @brief A function to call for processing matching files
        processFn_t function;

        /// @brief An executable command to call for processing matching files
        const char* exec;
    };
} assetProcessor_t;

typedef struct
{
    const char* inExt;
    const char* outExt;
    const assetProcessor_t* processor;
    const void* options;
} fileProcessorMap_t;

#endif
