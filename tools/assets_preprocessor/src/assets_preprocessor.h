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
    /// @brief The input file extension supported by this processor
    const char* inExt;

    /// @brief The output file extension written by this processor
    const char* outExt;

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
