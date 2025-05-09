/*
    const char* inExt: The extension of the file to be transformed, e.g. png
    const char* outExt: The extension of the file that will be created, e.g. .wsg
    enum processType_t type: The type of processing the file gets. Right now would just be FUNCTION, but maybe we'd have another for EXEC (to use for external tools)
    bool (*processFn)(const char* inFile, const char* outFile): If type is FUNCTION, this would be a pointer to the function called to process the file. The in and out file paths are already set up, so no filename parsing is needed inside the handler.
    (if EXEC is implemented) const char* execCmd: The command to run for processing this file. If it exits abnormally the file is considered failed. The in and out file arguments would be populated in string arguments, so "cp %1$s %2$s" would be a valid one. This might be a good uniform way to handle things like the sokoban python script.
*/

#ifndef _ASSETS_PREPROCESSOR_H_
#define _ASSETS_PREPROCESSOR_H_

typedef enum
{
    /// @brief Processor that calls a function pointer to process assets
    FUNCTION,
    
    /// @brief Processor that calls an external executable to process assets
    EXEC,
} processorType_t;

typedef bool (*processFn_t)(const char* inFile, const char* outFile);

typedef struct
{
    /// @brief The input file extension supported by this processor
    const char* inExt;
    
    /// @brief The output file extension written by this processor
    const char* outExt;

    /// @brief The type of this asset processor
    processorType_t type;

    union {
        /// @brief A function to call for processing matching files
        processFn_t function;

        /// @brief An executable to call for processing matching files
        const char* exec;
    };
} assetProcessor_t;

#endif
