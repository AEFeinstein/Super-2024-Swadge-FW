#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wcast-qual"
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

#include "stb_image.h"

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif

#include "assets_preprocessor.h"
#include "greyscale_processor.h"

#include "fileUtils.h"

bool process_greyscale(processorInput_t* arg);

const assetProcessor_t greyscaleProcessor
    = {.name = "gs", .type = FUNCTION, .function = process_greyscale, .inFmt = FMT_FILE_BIN, .outFmt = FMT_FILE_BIN};

bool process_greyscale(processorInput_t* arg)
{
    /* Load the source PNG */
    int w, h, n;
    unsigned char* data = stbi_load_from_file(arg->in.file, &w, &h, &n, 4);

    if (NULL != data)
    {
        uint16_t w_out = w;
        uint16_t h_out = h;
        fwrite( &w_out, 2, 1, arg->out.file );
        fwrite( &h_out, 2, 1, arg->out.file );

        uint8_t pixelBuffer[h][w];
        int x, y;
        for( y = 0; y < h; y++ )
        for( x = 0; x < w; x++ )
        {
            int pv = 0;
            int ch;
            int chuse = n;

            // Don't blend alpha.
            if( chuse == 4 ) { chuse = 3; }

            // Flip Y.
            for( ch = 0; ch < chuse; ch++ )
                pv += data[(x+(h-y-1)*w)*4+ch];

            pv = (pv + chuse/2) / chuse;
            pixelBuffer[y][x] = pv;
        }

        /* Free stbi memory */
        stbi_image_free(data);

        fwrite( pixelBuffer, w*h, 1, arg->out.file );
        return true;
    }

    return false;
}
