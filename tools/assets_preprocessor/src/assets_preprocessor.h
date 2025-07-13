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
 * assets_preprocessor.c, all asset processors are included and then mapped onto file
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
 * about in the \link assetProc_options Options Files \endlink section. Options
 * are provided as part of the argument struct to the preprocessor function and can be
 * treated as a key-value store of strings, integers, or booleans. The usage of options
 * files by an asset preprocessor function is described in more detail in the
 * \link assetProc_structOptions Function Preprocessors \endlink section.
 *
 * \section assetProc_configUsage Configuration and Usage
 * This section covers the usage and configuration of the assets preprocessor. Existing
 * preprocessor functions cover most common use cases, so unless you want to parse a
 * new, specific file format into a different swadge-specific format using C, this section
 * will cover everything you need to know.
 *
 * \subsection assetProc_usage Basic Usage
 *
 * The asset processor is automatically started by the build process for both the emulator
 * and the firmware, but you can also build and execute it manually from the repository
 * root directory with:
 *
 * ```bash
 * make -C ./tools/assets_preprocessor
 * ./tools/assets_preprocessor/asssets_preprocessor -i ./assets -o ./assets_image -c ./assets.conf
 * ```
 *
 * If you are trying to debug an issue with an asset processor, adding `-v` to the command
 * will enable verbose logging which could be helpful.
 *
 * \subsection assetProc_config Config File
 *
 * The config file is what maps a file extension, such as `.png`, onto a specific asset
 * processor function, such as `wsg`, and an output extension. such as `.wsg`.
 * The asset preprocessor uses an <a href="https://en.wikipedia.org/wiki/INI_file">INI-style</a>
 * config file, and the path is specified by the command-line argument `-c`.
 *
 * Each section in the config file matches a single file extension. If you use the input
 * file extension as the name of the section, e.g. `[png]` or `[.png]`, then the section
 * name will also be used as the input file extension. Otherwise, you must specify the
 * input file extension with the `inExt` option, like `inExt = .png`. The output file
 * extension must always be set, using the `outExt` option, like `outExt = .wsg`. And
 * each section must also specify an asset processor, which can either be one of the
 * functions listed by passing the `-h` option (see \link assetProc_args Arguments \endlink
 * below), or a shell command. To use a shell command, use the `exec` option, like
 * `exec = python3 ./tools/custom_asset_proc.py %i %o`. To use a function, use the
 * `func` option, like `func = wsg` or `func = heatshrink`.
 *
 * Example config section
 * ```ini
 * ; Lines starting with a ; are ignored
 *
 * ; This section defines a function asset processor
 * [.png]
 * outExt = .wsg
 * ; This is the function name listed by -h, not the extension!
 * func = wsg
 *
 * ; This section defines a shell asset processor
 * [my-game-level]
 * inExt = glvl
 * outExt = glb
 * ; %i will be replaced with the input .glvl file
 * ; %o will be replaced with the output .glb file
 * exec = python3 ./tools/my_game_asset_proc.py "%i" "%o"
 * ```
 *
 * \subsection assetProc_options Asset Options Files
 *
 * In addition to the main config file, there is another type of file that can be used
 * to configure how a single asset file or a directory of asset files is processed.
 * Different asset processor functions may have different options or no options at all,
 * and the specific options supported by each one are listed in the next section.
 *
 * To apply options to a specific file, e.g. `myImage.png`, create another file in the
 * same directory, but with the extension `.opts` instead, e.g. `myImage.opts`. To apply
 * options to an entire directory and all its subdirectories, create a file called `.opts`
 * inside that directory.
 *
 * Only a single options file will be used when processing any particular file; options
 * files are never merged. When an input file is being processed, the assets preprocessor
 * first searches for the `<filename>.opts` file, and loads its options if it exists. If
 * that options file does not exist, then the preprocessor will search for the `.opts` file
 * in the same directory as the input file. If that does not exist, it will search the
 * directory containing the input file, and so on until the top-level input directory has
 * been searched. The first of these `.opts` file that exists will be used and no other
 * assets files will be searched.
 *
 * An options file is an INI-style file similar to the config file, but instead of each
 * section defining a file extension mapping, in an options file each section contains
 * a list of options for a specific asset processor. The section name, e.g. `[wsg]` or
 * `[heatshrink]` should match the name of the asset processor function being used.
 * Multiple sections may be included in a single options file, though this only makes
 * sense for `.opts` files in a directory containing multiple input file types.
 *
 * \subsubsection assetProc_optionsExample Options File Example
 *
 * ```ini
 * ; In an Options file, the section name should match the function name
 * [wsg]
 * dither = true
 * ```
 *
 * \subsubsection assetProc_funcs Function Asset Preprocessors
 *
 * For all available asset processing functions, run the asset processor with the
 * `-h` option. Here is a list of the currently available processors and a brief
 * description of them, along with any options they support.
 *
 * \paragraph assetProc_bin bin
 * Copies the input file directly to the output file with no changes.
 *
 * \paragraph assetProc_chart chart
 * Processes the input file as a Clone Hero chart, which can be created by
 * <a href="https://efhiii.github.io/midi-ch/">this tool</a>. See also the
 * <a
 * href="https://github.com/TheNathannator/GuitarGame_ChartFormats/blob/main/doc/FileFormats/.chart/Core%20Infrastructure.md">
 * .chart file spec</a>.
 *
 * \paragraph assetProc_font font
 * Processes special font PNG files, which can be created by the
 * <a href="https://github.com/AEFeinstein/Super-2024-Swadge-FW/blob/main/tools/font_maker/README.md">
 * font_maker</a> tool. The output file can be loaded with \ref loadFont().
 *
 * \paragraph assetProc_heatshrink heatshrink
 * Compresses the input file using <a href="https://github.com/atomicobject/heatshrink">
 * heatshrink</a> compression. Can be loaded with \ref readHeatshrinkFile().
 *
 * \paragraph assetProc_json json
 * Validates the input JSON file and compresses it with heatshrink, by default.
 * The file can be loaded with \ref loadJson().
 *
 * Supports the option `compress`, which is true by default. If set to false,
 * the file will not be compressed, and can be loaded with \ref cnfsReadFile()
 * instead.
 *
 * \paragraph assetProc_text text
 * Removes any non-ASCII and unsupported characters in the input
 * file and writes it to the output.
 *
 * \paragraph assetProc_wsg wsg
 * Processes image files and converts them to the WSG (web-safe graphic) format.
 * These image files can be loaded with \ref loadWsg(). Any colors in the image
 * will be reduced to fit the web-safe color palette, along with one fully
 * transparent color, \ref paletteColor_t::cTransparent.
 *
 * Supports the option `dither`, which is false by default. If set to true,
 * images will be dithered when reducing their colors to the web-safe palette,
 * which may improve the appearance of larger images.
 *
 * \subsubsection assetProc_execs Exec Asset Preprocessors
 *
 * Unlike the function based asset preprocessors, exec asset preprocessors do not need any
 * C code and instead run a separate program to process each asset. This means that you
 * could, for example, write a Python script that parses a text file and uses the
 * <a href="https://docs.python.org/3/library/struct.html">struct</a> module to output a
 * more compact, Swadge-friendly format, and run that script with an `exec` processor. An
 * example
 *
 * Here's an example of a simple python program that reads separate lines of text, parses
 * and validates them, and writes them into a 21-byte struct representation. This program,
 * if saved at `tools/simple_processor.py`, could be used to process assets with
 * `exec = python ./tools/simple_processor.py "%i" "%o"`.
 *
 * ```python
 * #!/usr/bin/env python3
 * """
 * Parse a file of the format:
 *
 * name Name here
 * id 12345
 * color c235
 *
 * to a struct with char name[16], uint32_t id, and paletteColor_t color
 * """
 * import sys
 * import struct
 *
 * if __name__ == "__main__":
 *     with open(sys.argv[1], "r") as in_file:
 *         with open(sys.argv[2], "wb") as out_file:
 *             name = None
 *             id = None
 *             color = 0
 *             for line in in_file.readlines():
 *                 k, v = line.split()
 *                 if k == "name":
 *                     name = v
 *                 elif k == "id":
 *                     id = int(v)
 *                 elif k == "color":
 *                     r, g, b = v[1], v[2], v[3]
 *                     color = r * 36 + g * 6 + b
 *
 *             if id is None or id < 0 or not name or len(name) > 15 or color < 0 or color > 216:
 *                 print("Invalid input file")
 *                 sys.exit(1)
 *
 *             out_file.write(struct.pack(">16s I B", name, id, color))
 * ```
 *
 * Several placeholders are available to be used in the command string in order to
 * fill in file path information, and are listed in a table below.
 *
 * \subsubsection assetProc_placeholders Command String Placeholders
 * | Placeholder | Replacement                                |
 * |-------------|--------------------------------------------|
 * | \%i         | Full path to the input file                |
 * | \%f         | Filename portion of the input file path    |
 * | \%o         | Full path to the output file               |
 * | \%a         | Input file extension, without leading `.`  |
 * | \%b         | Output file extension, without leading `.` |
 * | \%\%        | Literal `%` character                      |
 *
 *
 *
 * \subsection assetProc_args Command-line Arguments
 *
 */

// clang-format off

/*! \file assets_preprocessor.h
 *
 * | Flag | Description                                                              |
 * |------|--------------------------------------------------------------------------|
 * | `-i` | Input directory which contains assets to process. Always required.       |
 * | `-o` | Output directory where processed assets are written. Always required.    |
 * | `-c` | Configuration file. Optional, but it won't do much without it.           |
 * | `-t` | Timestamp file. File will be updated any time an asset changes. Optional |
 * | `-v` | Verbose mode. Outputs a lot more information during processing.          |
 * | `-h` | Display usage information, and list available processor function names.  |
 */

// clang-format on

/*! \file assets_preprocessor.h
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
 * To use ::FMT_LINES when outputting data, the asset processor must allocate two buffers;
 * one `char**` for the list of string pointers (`lines`), and one `char*` for the entire
 * string data. Additionally, the first entry in `lines` _must_ be a pointer to the very
 * beginning of the text buffer in order to ensure that it can be properly freed. The
 * processor must set processorInput_t::out's `lines` and also set `lineCount` to the number
 * of items in `lines`. Output data sent in this way will be written to the output file with
 * each line separated by a newline character, `\n`, and with one trailing newline at the very
 * end of the file.
 *
 * If you need actual filenames, and cannot use FILE objects, then you can use FMT_FILENAME
 * which uses the .fileName parameter with the filename, instead of the contents of the file
 * data.
 *
 * Here is a summary of the various input and output options available and how to use them
 * for input and output. `arg` refers to the processorInput_t * passed as the argument to a
 * \ref processFn_t.
 */

// clang-format off

 /*! \file assets_preprocessor.h
 * | Format         | In Data        | In Length           | In Mode | Out Data         | Out Length           | Out Mode |
 * |----------------|----------------|---------------------|---------|------------------|----------------------|----------|
 * | ::FMT_DATA     | `arg->in.data` | `arg->in.length`    | `rb`    | `arg->out.data`  | `arg->out.length`    | `wb`     |
 * | ::FMT_TEXT     | `arg->in.text` | `arg->in.textSize`  | `r`     | `arg->out.text`  | `arg->out.textSize`  | `w`      |
 * | ::FMT_FILE_BIN | `arg->in.file` |                     | `rb`    | `arg->out.file`  |                      | `wb`     |
 * | ::FMT_FILE     | `arg->in.file` |                     | `r`     | `arg->out.file`  |                      | `w`      |
 * | ::FMT_FILENAME | `arg->in.fileName` |                 |         | `arg->out.fileName`  |                  |          |
 * | ::FMT_LINES    | `arg->in.lines`| `arg->in.lineCount` | `r`     | `arg->out.lines` | `arg->out.lineCount` | `w`      |
 */

// clang-format on

/*! \file assets_preprocessor.h
 * \subsubsection assetProc_structOptions Options
 * Options specified for the processed file will be available in
 * \ref processorInput_t::options, and the value of a particular option can
 * be retrieved using the functions \ref getStrOption(), \ref getIntOption(), and
 * \ref getBoolOption(). Note that \ref processorInput_t::options may be `NULL`, but
 * these functions will perform correctly when given a `NULL` \ref processorOptions_t
 * pointer. The function \ref hasOption() may also be used to check whether an option
 * was present.
 *
 * When retrieving an option the full name, including section, must be specified, with
 * a `.` separating the section and key name. The section name should always match the
 * name in \ref assetProcessor_t::name to reduce confusion. For example, a processor
 * function named `myfunc` which uses an option called `compact` should call
 * `getBoolOption(arg->options, "myfunc.compact", true)`, which would correspond to
 * a `.opts` file which contains this section:
 *
 * ```ini
 * [myfunc]
 * compact = false
 * ```
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
 * // Export the asset processors so they can be included from assets_processor.c
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
 * If you only want to associate a new file type to an existing preprocessor, you can
 * do so by editing the config file, which is covered in the \link assetProc_configUsage Config
 * section \endlink.
 *
 * To create a new asset processor function, follow these steps.
 *
 * 1. Create new `<type>_processor.h` and `.c` files in `/tools/assets_preprocessor/src/` -- see
 * [above](#assetProc_examples) for an example.
 * 2. Include the newly-added `.h` file in assets_preprocessor.c
 * 3. Update \link assetProc_config `assets.conf` \endlink to configure the file
 * extensions to be handled by the new processor.
 * 4. Document the new processor \link assetProc_funcs here \endlink, especially if it
 * might be used by other modes in the future.
 * 5. Run `make clean all`
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
    /// @brief Only provide/use filenames, instead of opening/use them as a file.
    FMT_FILENAME,
} processorFormat_t;

/**
 * @brief Holds processor input or output data
 *
 */
typedef union
{
    /**
     * @brief Holds file handle for ::FMT_FILE or ::FMT_FILE_BIN formats
     *
     */
    FILE* file;

    /**
     * @brief Holds data for ::FMT_DATA format
     */
    struct
    {
        /// @brief Buffer holding binary data
        uint8_t* data;
        /// @brief Length of binary data
        size_t length;
    };

    /**
     * @brief Holds data for ::FMT_LINES format
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
     * @brief Holds data for ::FMT_TEXT format
     *
     */
    struct
    {
        /// @brief Buffer holding text data
        char* text;
        /// @brief The size of the text data, including NUL terminator
        size_t textSize;
    };

	const char * fileName;
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
     * @brief The key name
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

    /// @brief Extra options passed to the processor for these files
    const processorOptions_t* options;
} fileProcessorMap_t;

#endif
