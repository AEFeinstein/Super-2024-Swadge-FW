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
 * excluding data that the Swadge does not use from the original asset, or validating
 * that the asset file is properly formatted before copying it to an asset.
 *
 * Each asset processor handles a specific type of file. Some processors, like
 * raw_processor.h and bin_processor.h, are generic and could be used by various file
 * extensions. An asset processor can be mapped to any number of pairs of file input
 * and output extensions. Multiple input extensions can even be mapped to the
 * same extension if needed. Asset processors follow a similar pattern to mode
 * structs, where each asset processor struct is defined in its own file and made
 * available to other files via an `extern` variable in its header file. Then, in
 * asset_preprocessor.c, all asset processors are included and then mapped onto file
 * extensions in one place.
 *
 * In order to reduce the need for repetitive path manipulation, file opening, and
 * error handling logic in every processor, the asset preprocessor ensures that the
 * input and output files exist before each asset processor is called. Each processor
 * can be configured to receive and output data in several formats. This can either be
 * a raw `FILE*` handle, or a buffer containing file data in-memory.
 *
 * Individual asset files and entire directory trees can also be configured with options
 * to be used by file processors. This is done with `.opts` files, which you can read more
 * about using in the \link md_tools_2assets__preprocessor_2README README \endlink. Options
 * are provided as part of the argument struct to the preprocessor function and can be
 * treated as a key-value store of strings, integers, or booleans. The usage of options
 * files by an asset preprocessor function is described in more detail below.
 *
 * \section assetProc_structure Structure
 *
 * \subsection assetProc_structFuncs Function Preprocessors
 *
 * Most asset processors are implemented as C functions. This is the recommended way to
 * implement asset preprocessors as it's the best way to make sure that the asset
 * processor is compatible with all platforms and doesn't have any hidden dependencies.
 * It also provides the most flexibility in handling data
 *
 * To use a raw `FILE*` handle, set assetProcessor_t::inFmt to ::FMT_FILE or ::FMT_FILE_BIN.
 * For an input file, the file will be opened with mode `"r"` or `"rb"` respectively. For
 * an output file, the file will be opened with mode `"w"` or `"wb"` respectively. The file
 * handle will be passed in processorFileData_t::file, and the asset processor can then use
 * standard C `stdio.h` functions like `fread()`, `fwrite()`, `fgetc()`, `fputc()`, and
 * others. The asset processor should not close the file handle itself; this will be done
 * automatically.
 *
 * To receive data in a buffer, set assetProcessor_t::inFmt to ::FMT_DATA or ::FMT_TEXT.
 * The first will open the file in binary mode (`"rb"`) and the latter will open it in
 * text mode (`"r"`). If ::FMT_DATA is used, the entire file is read and its data is
 * stored in processorFileData_t's `data` field, and its length stored in `length`.
 *  If ::FMT_TEXT is used, the entire file is read and its data is stored in
 * processorFileData_t's `text` field as a NUL-terminated string, with the size of the
 * text buffer (including the NUL terminator) stored in `textSize`. The asset processor
 * is permitted to modify these buffers, but should not free or unassign them from the
 * input struct.
 *
 * To output data to a file through a buffer, the asset processor must allocate its own
 * output buffer using, e.g. `malloc()` or `calloc()`, and assign that buffer to its output
 * processorInput_t::out's `data` or `text` fields, depending on the output format
 * of the asset processor defined by assetProcessor_t::outFmt. The corresponding
 * `length` or `textSize` fields, respectively, must also be set in processorInput_t::out.
 * The data buffer will automatically be freed after the processor finishes. It is also
 * possible to reuse the input buffer by modifying its data in-place and then assigning
 * it directly to the output buffer. The original pointer to the input buffer should not
 * be unset when doing this as it may prevent the data from being freed properly.
 *
 * In addition to using a file handle or a simple data buffer, text files can additionally
 * be processed using ::FMT_LINES. This reads the input file as a series of lines, terminated
 * by either `\n` or `\r\n`, and constructs an array of strings which point to each line,
 * without the trailing newline. The asset processor can then simply loop over each line in
 * processorFileData_t's `lines`, which will contain `lineCount` entries. As with ::FMT_DATA
 * or ::FMT_TEXT, this data may also be freely modified in-place and can be assigned directly
 * into processorInput_t::out.
 *
 * To use FMT_LINES when outputting data, the asset processor must allocate two buffers;
 * one `char**` for the list of string pointers (`lines`), and one `char*` for the entire
 * string data. Additionally, the first entry in `lines` _must_ be a pointer to the very
 * beginning of the text buffer in order to ensure that it can be properly freed. The
 * processor must set processorInput_t::out's `lines` and also set `lineCount` to the number
 * of items in `lines`. Output data sent in this way will be written to the output file with
 * each line separated by a newline character, `\n`, and with one trailing newline at the very
 * end of the file.
 *
 * Here is a summary of the various input and output options available and how to use them
 * for input and output. `arg` refers to the processorInput_t* passed as the argument to a
 * processFn_t.
 */

// clang-format off

 /*! \file assets_preprocessor.h
 * | Format       | In Data        | In Length           | In Mode | Out Data         | Out Length           | Out Mode |
 * |--------------|----------------|---------------------|---------|------------------|----------------------|----------|
 * | FMT_DATA     | `arg->in.data` | `arg->in.length`    | `rb`    | `arg->out.data`  | `arg->out.length`    | `wb`     |
 * | FMT_TEXT     | `arg->in.text` | `arg->in.textSize`  | `r`     | `arg->out.text`  | `arg->out.textSize`  | `w`      |
 * | FMT_FILE_BIN | `arg->in.file` |                     | `rb`    | `arg->out.file`  |                      | `wb`     |
 * | FMT_FILE     | `arg->in.file` |                     | `r`     | `arg->out.file`  |                      | `w`      |
 * | FMT_LINES    | `arg->in.lines`| `arg->in.lineCount` | `r`     | `arg->out.lines` | `arg->out.lineCount` | `w`      |
 */

// clang-format on

/*! \file assets_preprocessor.h
 * \subsubsection assetProc_structOptions Options
 * For information on configuring asset processor options for asset files and directories,
 * see the \ref md_tools_2assets__preprocessor_2README.
 *
 *
 * \subsection assetProc_structExecs Exec Preprocessors
 *
 * In addition to the function-based implementation used by most processors, they may also
 * be defined as an processorType_t::EXEC processor, which executes some other program (e.g.
 * a Python script) in order to process an asset. It's very important to keep in mind that
 * this program must be available on all platforms in order for assets to process properly.
 * If an executable that will be called by an exec processor must be compiled, it must be
 * added to the makefile in order to ensure it is built before the asset processor runs.
 *
 * An EXEC processor is defined by a command string in its assetProcessor_t::exec field.
 * For example, `.exec = "cp %i %o"` would copy the input file directly to its output.
 * Several placeholders are available to be used in this string in order to fill in file
 * path information, and are listed in a table below.
 *
 * | Placeholder | Replacement                                |
 * |-------------|--------------------------------------------|
 * | \%i         | Full path to the input file                |
 * | \%f         | Filename portion of the input file path    |
 * | \%o         | Full path to the output file               |
 * | \%a         | Input file extension, without leading '.'  |
 * | \%b         | Output file extension, without leading '.' |
 * | \%\%        | Literal '%' character                      |
 *
 * \subsection assetProc_examples Function Processor Examples
 *
 * Below is an example file which defines several asset processors using various input and
 * output methods. Keep in mind that input and output formats can be matched so that a
 * processor can always use whichever format is most convenient.
 *
 * \code{.h}
 * #pragma once
 *
 * #include "assets_preprocessor.h"
 *
 * // Export the asset processors so they can be included from asset_processor.c
 * extern const assetProcessor_t fileInFileOutProcessor;
 * extern const assetProcessor_t dataInDataOutProcessor;
 * extern const assetProcessor_t textInDataOutProcessor;
 * extern const assetProcessor_t linesInFileOutProcessor;
 * extern const assetProcessor_t optionsFileProcessor;
 *
 * \endcode
 *
 * \code{.c}
 * #include "assets_preprocessor.h"
 * #include "example_processor.h"
 * #include "heatshrink_util.h"
 * #include "fileUtils.h"
 *
 * #include <stdbool.h>
 * #include <stddef.h>
 * #include <stdint.h>
 * #include <stdio.h>
 *
 * bool fileInFileOutFunc(processorInput_t* arg)
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
 *     .name = "file-file",
 *     .type = FUNCTION,
 *     .function = fileInFileOutFunc,
 *     .inFmt = FMT_FILE,
 *     .outFmt = FMT_FILE,
 * };
 *
 * bool dataInDataOutFunc(processorInput_t* arg)
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
 *     .name = "data-data",
 *     .type = FUNCTION,
 *     .function = dataItDataOutFunc,
 *     .inFmt = FMT_DATA,
 *     .outFmt = FMT_DATA,
 * };
 *
 * bool textInDataOutFunc(processorInput_t* arg)
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
 *     .name = "lines-bin",
 *     .type = FUNCTION,
 *     .function = textInDataOutFunc,
 *     .inFmt = FMT_TEXT,
 *     .outFmt = FMT_DATA
 * };
 *
 * bool linesInDataOutFunc(processorInput_t* arg)
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
 *     .name = "lines-bin"
 *     .type = FUNCTION,
 *     .function = linesInDataOutFunc,
 *     .inFmt = FMT_LINES,
 *     .outFmt = FMT_FILE_BIN
 * };
 *
 * bool optionsFileFunc(processorInput_t* arg)
 * {
 *     int version = getIntOption(arg->options, "file-with-options.version", 0);
 *     if (version != 0)
 *     {
 *         fputc(version, arg->out.file);
 *     }
 *     const char* prefix = getStrOption(arg->options, "file-with-options.prefix");
 *     if (NULL != prefix)
 *     {
 *         fputs(prefix, arg->out.file);
 *         fputc(0, arg->out.file);
 *     }
 *
 *     if (geBoolOption(arg->options, "file-with-options.compress", true))
 *     {
 *         return writeHeatshrinkFileHandle(arg->in.data, arg->in.length, arg->out.file);
 *     }
 *     else
 *     {
 *         return 0 != fwrite(arg->in.data, arg->in.length, 1, args->out.file);
 *     }
 * }
 *
 * const assetProcessor_t optionsFileProcessor = {
 *     .name = "file-with-options",
 *     .type = FUNCTION,
 *     .function = optionsFileFunc,
 *     .inFmt = FMT_DATA,
 *     .outFmt = FMT_FILE_BIN,
 * };
 *
 * \endcode
 *
 *
 * \section assetProc_adding Adding a New Processor
 * To create a new asset processor function, follow these steps.
 *
 * 1. Create new `<type>_processor.h` and `.c` files in `/tools/assets_preprocessor/src/` -- see
 * [above](#assetProc_examples) for an example.
 * 2. Include the newly-added `.h` file in asset_preprocessor.c
 * 3. Add an entry to fileProcessorMap in assets_preprocessor.c for each file extension the processor should handle
 * 4. Update \link md_tools_2assets__preprocessor_2README#config-file `assets.conf` \endlink to configure the file
 * extensions to be handled by the new processor.
 * 5. Document the new processor in the \link md_tools_2assets__preprocessor_2README README \endlink, especially if it
 * might be used by other modes in the future.
 * 6. Run `make clean all`
 *
 */

/**
 * @brief Specifies which type of asset processor is being defined
 */
typedef enum
{
    /// @brief Processor that calls a function pointer to process assets
    FUNCTION,

    /// @brief Processor that executes a shell command to process assets
    EXEC,
} processorType_t;

/**
 * @brief The format that this asset processor accepts or returns its data in
 */
typedef enum
{
    /// @brief An opened file handle is passed for the file
    FMT_FILE,
    /// @brief An opened file handle (in binary mode) is passed for the file
    FMT_FILE_BIN,
    /// @brief A buffer containing a file's raw binary data is passed
    FMT_DATA,
    /// @brief A text file's data is passed as an ordinary string
    FMT_TEXT,
    /// @brief A text file's data is passed as an array of one string per line
    FMT_LINES,
} processorFormat_t;

/**
 * @brief Holds processor input or output data
 *
 */
typedef union
{
    /**
     * @brief Holds file handle for FMT_FILE or FMT_FILE_BIN formats
     *
     */
    FILE* file;

    /**
     * @brief Holds data for FMT_DATA format
     */
    struct
    {
        /// @brief Buffer holding binary data
        uint8_t* data;
        /// @brief Length of binary data
        size_t length;
    };

    /**
     * @brief Holds data for FMT_LINES format
     *
     */
    struct
    {
        /// @brief Array of string pointers for each line
        char** lines;
        /// @brief The number of string pointers in the array
        size_t lineCount;
    };

    /**
     * @brief Holds data for FMT_TEXT format
     *
     */
    struct
    {
        /// @brief Buffer holding text data
        char* text;
        /// @brief The size of the text data, including NUL terminator
        size_t textSize;
    };
} processorFileData_t;

/**
 * @brief Holds a single key-value pair
 */
typedef struct
{
    /**
     * @brief The section name only
     *
     */
    char* section;

    /**
     * @brief The key name, as "<section-name>.<key>"
     */
    char* name;

    /**
     * @brief The value string
     */
    char* value;
} optPair_t;

/**
 * @brief Holds a list of key-value option pairs
 * @see getStrOption()
 * @see getIntOption()
 * @see getBoolOption()
 * @see hasOption()
 */
typedef struct
{
    /**
     * @brief The number of options contained in this list
     */
    size_t optionCount;

    /**
     * @brief The array of options
     */
    optPair_t* pairs;
} processorOptions_t;

/**
 * @brief Holds the input and output data for a single file processing operation
 *
 * Note that the processor must only use the input/output formats it was configured for
 * with \ref assetProcessor_t::inFmt and \ref assetProcessor_t::outFmt
 */
typedef struct
{
    /// @brief Holds the input data in whichever format was configured for the processor
    const processorFileData_t in;

    /// @brief Holds the output data in whichever format was configured for the processor
    processorFileData_t out;

    /// @brief Holds the input filename for convenience and error reporting
    const char* inFilename;

    /// @brief Holds a pointer to any configuration options in use for this file
    const processorOptions_t* options;
} processorInput_t;

/**
 * @brief A function that performs asset processing on a file
 * @return true if the asset processed successfully
 * @return false if asset processing failed
 */
typedef bool (*processFn_t)(processorInput_t* arg);

/**
 * @brief Defines an asset processor
 */
typedef struct
{
    /// @brief A name for this asset processor, if referenced
    const char* name;

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

/**
 * @brief Associates an input and output extension to a processor
 */
typedef struct
{
    /// @brief The input file extension to match
    const char* inExt;
    /// @brief The output file extension to write
    const char* outExt;
    /// @brief A pointer to the processor used to transform the files
    const assetProcessor_t* processor;
} fileProcessorMap_t;

#endif
