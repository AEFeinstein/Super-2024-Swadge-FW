/*! \file palette.h
 *
 * \section palette_design Design Philosophy
 *
 * Even though the TFT supports 16 bit color, a 16 bit frame-buffer is too big to have in RAM alongside games and such.
 * Instead, the 8 bit <a href="https://www.rapidtables.com/web/color/Web_Safe.html">Web Safe palette</a> is used, where
 * each RGB channel has six options for a total of 216 colors. The ::paletteColor_t enum has values for all colors in
 * the form of cRGB, where R, G, and B each range from 0 to 5. For example, ::c500 is full red.
 * ::cTransparent is a special value for a transparent pixel.
 */

#ifndef _PALETTE_H_
#define _PALETTE_H_

#include <stdint.h>

// Converts RGB values (0-31, 5 bits per color) to combined 16-bit palette colors
// Technically green is 6 bits per color but idk
#define RGB_TO_16BIT_PALETTE(r,g,b) ((((g) & 0b111) << 13) | (((b) & 0b11111) << 8) | (((r) & 0b11111) << 3) | (((g) >> 3) & 0b111))

extern uint16_t paletteColors[];

/**
 * @brief All 216 possible colors, named like cRGB, plus transparent, plus 39 extended colors
 */
typedef enum __attribute__((packed))
{
    c000,         ///< r = 0, g = 0, b = 0
    c001,         ///< r = 0, g = 0, b = 1
    c002,         ///< r = 0, g = 0, b = 2
    c003,         ///< r = 0, g = 0, b = 3
    c004,         ///< r = 0, g = 0, b = 4
    c005,         ///< r = 0, g = 0, b = 5
    c010,         ///< r = 0, g = 1, b = 0
    c011,         ///< r = 0, g = 1, b = 1
    c012,         ///< r = 0, g = 1, b = 2
    c013,         ///< r = 0, g = 1, b = 3
    c014,         ///< r = 0, g = 1, b = 4
    c015,         ///< r = 0, g = 1, b = 5
    c020,         ///< r = 0, g = 2, b = 0
    c021,         ///< r = 0, g = 2, b = 1
    c022,         ///< r = 0, g = 2, b = 2
    c023,         ///< r = 0, g = 2, b = 3
    c024,         ///< r = 0, g = 2, b = 4
    c025,         ///< r = 0, g = 2, b = 5
    c030,         ///< r = 0, g = 3, b = 0
    c031,         ///< r = 0, g = 3, b = 1
    c032,         ///< r = 0, g = 3, b = 2
    c033,         ///< r = 0, g = 3, b = 3
    c034,         ///< r = 0, g = 3, b = 4
    c035,         ///< r = 0, g = 3, b = 5
    c040,         ///< r = 0, g = 4, b = 0
    c041,         ///< r = 0, g = 4, b = 1
    c042,         ///< r = 0, g = 4, b = 2
    c043,         ///< r = 0, g = 4, b = 3
    c044,         ///< r = 0, g = 4, b = 4
    c045,         ///< r = 0, g = 4, b = 5
    c050,         ///< r = 0, g = 5, b = 0
    c051,         ///< r = 0, g = 5, b = 1
    c052,         ///< r = 0, g = 5, b = 2
    c053,         ///< r = 0, g = 5, b = 3
    c054,         ///< r = 0, g = 5, b = 4
    c055,         ///< r = 0, g = 5, b = 5
    c100,         ///< r = 1, g = 0, b = 0
    c101,         ///< r = 1, g = 0, b = 1
    c102,         ///< r = 1, g = 0, b = 2
    c103,         ///< r = 1, g = 0, b = 3
    c104,         ///< r = 1, g = 0, b = 4
    c105,         ///< r = 1, g = 0, b = 5
    c110,         ///< r = 1, g = 1, b = 0
    c111,         ///< r = 1, g = 1, b = 1
    c112,         ///< r = 1, g = 1, b = 2
    c113,         ///< r = 1, g = 1, b = 3
    c114,         ///< r = 1, g = 1, b = 4
    c115,         ///< r = 1, g = 1, b = 5
    c120,         ///< r = 1, g = 2, b = 0
    c121,         ///< r = 1, g = 2, b = 1
    c122,         ///< r = 1, g = 2, b = 2
    c123,         ///< r = 1, g = 2, b = 3
    c124,         ///< r = 1, g = 2, b = 4
    c125,         ///< r = 1, g = 2, b = 5
    c130,         ///< r = 1, g = 3, b = 0
    c131,         ///< r = 1, g = 3, b = 1
    c132,         ///< r = 1, g = 3, b = 2
    c133,         ///< r = 1, g = 3, b = 3
    c134,         ///< r = 1, g = 3, b = 4
    c135,         ///< r = 1, g = 3, b = 5
    c140,         ///< r = 1, g = 4, b = 0
    c141,         ///< r = 1, g = 4, b = 1
    c142,         ///< r = 1, g = 4, b = 2
    c143,         ///< r = 1, g = 4, b = 3
    c144,         ///< r = 1, g = 4, b = 4
    c145,         ///< r = 1, g = 4, b = 5
    c150,         ///< r = 1, g = 5, b = 0
    c151,         ///< r = 1, g = 5, b = 1
    c152,         ///< r = 1, g = 5, b = 2
    c153,         ///< r = 1, g = 5, b = 3
    c154,         ///< r = 1, g = 5, b = 4
    c155,         ///< r = 1, g = 5, b = 5
    c200,         ///< r = 2, g = 0, b = 0
    c201,         ///< r = 2, g = 0, b = 1
    c202,         ///< r = 2, g = 0, b = 2
    c203,         ///< r = 2, g = 0, b = 3
    c204,         ///< r = 2, g = 0, b = 4
    c205,         ///< r = 2, g = 0, b = 5
    c210,         ///< r = 2, g = 1, b = 0
    c211,         ///< r = 2, g = 1, b = 1
    c212,         ///< r = 2, g = 1, b = 2
    c213,         ///< r = 2, g = 1, b = 3
    c214,         ///< r = 2, g = 1, b = 4
    c215,         ///< r = 2, g = 1, b = 5
    c220,         ///< r = 2, g = 2, b = 0
    c221,         ///< r = 2, g = 2, b = 1
    c222,         ///< r = 2, g = 2, b = 2
    c223,         ///< r = 2, g = 2, b = 3
    c224,         ///< r = 2, g = 2, b = 4
    c225,         ///< r = 2, g = 2, b = 5
    c230,         ///< r = 2, g = 3, b = 0
    c231,         ///< r = 2, g = 3, b = 1
    c232,         ///< r = 2, g = 3, b = 2
    c233,         ///< r = 2, g = 3, b = 3
    c234,         ///< r = 2, g = 3, b = 4
    c235,         ///< r = 2, g = 3, b = 5
    c240,         ///< r = 2, g = 4, b = 0
    c241,         ///< r = 2, g = 4, b = 1
    c242,         ///< r = 2, g = 4, b = 2
    c243,         ///< r = 2, g = 4, b = 3
    c244,         ///< r = 2, g = 4, b = 4
    c245,         ///< r = 2, g = 4, b = 5
    c250,         ///< r = 2, g = 5, b = 0
    c251,         ///< r = 2, g = 5, b = 1
    c252,         ///< r = 2, g = 5, b = 2
    c253,         ///< r = 2, g = 5, b = 3
    c254,         ///< r = 2, g = 5, b = 4
    c255,         ///< r = 2, g = 5, b = 5
    c300,         ///< r = 3, g = 0, b = 0
    c301,         ///< r = 3, g = 0, b = 1
    c302,         ///< r = 3, g = 0, b = 2
    c303,         ///< r = 3, g = 0, b = 3
    c304,         ///< r = 3, g = 0, b = 4
    c305,         ///< r = 3, g = 0, b = 5
    c310,         ///< r = 3, g = 1, b = 0
    c311,         ///< r = 3, g = 1, b = 1
    c312,         ///< r = 3, g = 1, b = 2
    c313,         ///< r = 3, g = 1, b = 3
    c314,         ///< r = 3, g = 1, b = 4
    c315,         ///< r = 3, g = 1, b = 5
    c320,         ///< r = 3, g = 2, b = 0
    c321,         ///< r = 3, g = 2, b = 1
    c322,         ///< r = 3, g = 2, b = 2
    c323,         ///< r = 3, g = 2, b = 3
    c324,         ///< r = 3, g = 2, b = 4
    c325,         ///< r = 3, g = 2, b = 5
    c330,         ///< r = 3, g = 3, b = 0
    c331,         ///< r = 3, g = 3, b = 1
    c332,         ///< r = 3, g = 3, b = 2
    c333,         ///< r = 3, g = 3, b = 3
    c334,         ///< r = 3, g = 3, b = 4
    c335,         ///< r = 3, g = 3, b = 5
    c340,         ///< r = 3, g = 4, b = 0
    c341,         ///< r = 3, g = 4, b = 1
    c342,         ///< r = 3, g = 4, b = 2
    c343,         ///< r = 3, g = 4, b = 3
    c344,         ///< r = 3, g = 4, b = 4
    c345,         ///< r = 3, g = 4, b = 5
    c350,         ///< r = 3, g = 5, b = 0
    c351,         ///< r = 3, g = 5, b = 1
    c352,         ///< r = 3, g = 5, b = 2
    c353,         ///< r = 3, g = 5, b = 3
    c354,         ///< r = 3, g = 5, b = 4
    c355,         ///< r = 3, g = 5, b = 5
    c400,         ///< r = 4, g = 0, b = 0
    c401,         ///< r = 4, g = 0, b = 1
    c402,         ///< r = 4, g = 0, b = 2
    c403,         ///< r = 4, g = 0, b = 3
    c404,         ///< r = 4, g = 0, b = 4
    c405,         ///< r = 4, g = 0, b = 5
    c410,         ///< r = 4, g = 1, b = 0
    c411,         ///< r = 4, g = 1, b = 1
    c412,         ///< r = 4, g = 1, b = 2
    c413,         ///< r = 4, g = 1, b = 3
    c414,         ///< r = 4, g = 1, b = 4
    c415,         ///< r = 4, g = 1, b = 5
    c420,         ///< r = 4, g = 2, b = 0
    c421,         ///< r = 4, g = 2, b = 1
    c422,         ///< r = 4, g = 2, b = 2
    c423,         ///< r = 4, g = 2, b = 3
    c424,         ///< r = 4, g = 2, b = 4
    c425,         ///< r = 4, g = 2, b = 5
    c430,         ///< r = 4, g = 3, b = 0
    c431,         ///< r = 4, g = 3, b = 1
    c432,         ///< r = 4, g = 3, b = 2
    c433,         ///< r = 4, g = 3, b = 3
    c434,         ///< r = 4, g = 3, b = 4
    c435,         ///< r = 4, g = 3, b = 5
    c440,         ///< r = 4, g = 4, b = 0
    c441,         ///< r = 4, g = 4, b = 1
    c442,         ///< r = 4, g = 4, b = 2
    c443,         ///< r = 4, g = 4, b = 3
    c444,         ///< r = 4, g = 4, b = 4
    c445,         ///< r = 4, g = 4, b = 5
    c450,         ///< r = 4, g = 5, b = 0
    c451,         ///< r = 4, g = 5, b = 1
    c452,         ///< r = 4, g = 5, b = 2
    c453,         ///< r = 4, g = 5, b = 3
    c454,         ///< r = 4, g = 5, b = 4
    c455,         ///< r = 4, g = 5, b = 5
    c500,         ///< r = 5, g = 0, b = 0
    c501,         ///< r = 5, g = 0, b = 1
    c502,         ///< r = 5, g = 0, b = 2
    c503,         ///< r = 5, g = 0, b = 3
    c504,         ///< r = 5, g = 0, b = 4
    c505,         ///< r = 5, g = 0, b = 5
    c510,         ///< r = 5, g = 1, b = 0
    c511,         ///< r = 5, g = 1, b = 1
    c512,         ///< r = 5, g = 1, b = 2
    c513,         ///< r = 5, g = 1, b = 3
    c514,         ///< r = 5, g = 1, b = 4
    c515,         ///< r = 5, g = 1, b = 5
    c520,         ///< r = 5, g = 2, b = 0
    c521,         ///< r = 5, g = 2, b = 1
    c522,         ///< r = 5, g = 2, b = 2
    c523,         ///< r = 5, g = 2, b = 3
    c524,         ///< r = 5, g = 2, b = 4
    c525,         ///< r = 5, g = 2, b = 5
    c530,         ///< r = 5, g = 3, b = 0
    c531,         ///< r = 5, g = 3, b = 1
    c532,         ///< r = 5, g = 3, b = 2
    c533,         ///< r = 5, g = 3, b = 3
    c534,         ///< r = 5, g = 3, b = 4
    c535,         ///< r = 5, g = 3, b = 5
    c540,         ///< r = 5, g = 4, b = 0
    c541,         ///< r = 5, g = 4, b = 1
    c542,         ///< r = 5, g = 4, b = 2
    c543,         ///< r = 5, g = 4, b = 3
    c544,         ///< r = 5, g = 4, b = 4
    c545,         ///< r = 5, g = 4, b = 5
    c550,         ///< r = 5, g = 5, b = 0
    c551,         ///< r = 5, g = 5, b = 1
    c552,         ///< r = 5, g = 5, b = 2
    c553,         ///< r = 5, g = 5, b = 3
    c554,         ///< r = 5, g = 5, b = 4
    c555,         ///< r = 5, g = 5, b = 5
    cTransparent, ///< Transparent, will not draw over other pixels
    cx00,
    cx01,
    cx02,
    cx03,
    cx04,
    cx05,
    cx06,
    cx07,
    cx08,
    cx09,
    cx10,
    cx11,
    cx12,
    cx13,
    cx14,
    cx15,
    cx16,
    cx17,
    cx18,
    cx19,
    cx20,
} paletteColor_t;

void extendPalette(const uint16_t palette[39]);

#endif
