#include "sudoku_processor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assets_preprocessor.h"


bool process_sudoku(processorInput_t* arg);

const assetProcessor_t sudokuProcessor = {
    .name = "sudoku",
    .type = FUNCTION,
    .function = process_sudoku,
    .inFmt = FMT_FILE,
    .outFmt = FMT_FILE_BIN,
};

bool process_sudoku(processorInput_t* arg)
{
    uint8_t cells[81];

    int valueCount = 0;
    int n = 0;

    int cur;
    bool startOfLine = true;
    bool comment = false;
    while (-1 != (cur = getc(arg->in.file)))
    {
        switch (cur)
        {
            case '.':
            case '0':
            case 'X':
            {
                if (!comment)
                {
                    if (n >= 81)
                    {
                        return false;
                    }

                    // advance cell, don't write
                    cells[n++] = 0;
                }

                startOfLine = false;
                break;
            }

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                if (!comment)
                {
                    if (n >= 81)
                    {
                        return false;
                    }
                    cells[n++] = (cur - '0');
                    valueCount++;
                }

                startOfLine = false;
                break;
            }

            // As described on https://www.sudocue.net/fileformats.php
            case 'A': // Author
            case 'D': // Description
            case 'C': // Comment
            case 'B': // Date published
            case 'S': // Source
            case 'L': // Level
            case 'U': // Source URL
            {
                if (comment && startOfLine)
                {
                    // no comment handling th
                }
                startOfLine = false;
            }

            case '#':
            {
                if (startOfLine)
                {
                    comment = true;
                }

                startOfLine = false;
                break;
            }

            case '\r':
            case '\n':
            {
                comment = false;
                startOfLine = true;
                break;
            }

            default:
            {
                startOfLine = false;
                break;
            }
        }
    }

    if (n != 81)
    {
        return false;
    }

    FILE* outFile = arg->out.file;

    // version
    putc(0, outFile);
    // mode: normal
    putc(0, outFile);
    // grid size: 9x9
    putc(9, outFile);
    // base: 9
    putc(9, outFile);

    // box format: 0, standard
    putc(0, outFile);
    // grid format: 0, mapped
    putc(0, outFile);
    // note format: 0, mapped
    putc(0, outFile);
    // flag format: 0, mapped
    putc(0, outFile);

    //// BOXES -- NOTHING TO WRITE

    //// GRID

    // Holds the nibble for the cell value
    uint8_t buf = 0;
    bool pending = false;

    // Write grid count
    putc(valueCount, outFile);

    // Write grid
    for (int i = 0; i < n; i++)
    {
        if (cells[i] != 0)
        {
            int r = i / 9;
            int c = i % 9;

            if (pending)
            {
                // Write second location of pair
                putc(((r & 0x0F) << 4) | (c & 0x0F), outFile);
                buf |= (cells[i] - 1);
                // Write accompanying numerical values
                putc(buf, outFile);
                pending = false;
            }
            else
            {
                // Write first location of pair
                putc(((r & 0x0F) << 4) | (c & 0x0F), outFile);
                buf = (cells[i] - 1) << 4;
                pending = true;
            }
        }
    }

    // Write any remaining value
    if (pending)
    {
        // Write an empty location
        putc(0, outFile);

        // Write the pending value
        putc(buf, outFile);
    }

    //// NOTES
    // Note count: 0!
    putc(0, outFile);
    // It's real easy

    //// FLAGS

    // Flag count: same as  count, they're all givens
    putc(valueCount, outFile);

    for (int i = 0; i < n; i++)
    {
        if (cells[i] != 0)
        {
            int r = i / 9;
            int c = i % 9;
            // Write location
            putc(((r & 0x0F) << 4) | (c & 0x0F), outFile);
            // Write flag: 1 (LOCKED) since this is a "given" value
            putc(1, outFile);
        }
    }

    // That's it!
    return true;
}