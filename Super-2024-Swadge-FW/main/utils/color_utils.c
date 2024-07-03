//==============================================================================
// Includes
//==============================================================================

#include "color_utils.h"

//==============================================================================
// Constant Data
//==============================================================================

/**
 * @brief A table that can be used for gamma correction.
 *
 * Maps a uint8_t value to a gamma-corrected uint8_t.
 */
const uint32_t gamma_correction_table[256]
    = {0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
       2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,
       6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  12,  12,  12,  13,  13,  14,
       14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  23,  23,  24,  24,  25,
       26,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  34,  34,  35,  36,  36,  37,  38,  39,  40,  40,
       41,  42,  43,  44,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
       61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  74,  75,  76,  77,  78,  79,  80,  82,  83,  84,
       85,  86,  88,  89,  90,  91,  92,  94,  95,  96,  98,  99,  100, 101, 103, 104, 106, 107, 108, 110, 111, 112,
       114, 115, 117, 118, 120, 121, 122, 124, 125, 127, 128, 130, 131, 133, 134, 136, 138, 139, 141, 142, 144, 146,
       147, 149, 150, 152, 154, 155, 157, 159, 160, 162, 164, 166, 167, 169, 171, 173, 174, 176, 178, 180, 182, 183,
       185, 187, 189, 191, 193, 195, 197, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224, 226,
       228, 230, 232, 235, 237, 239, 241, 243, 245, 247, 249, 252, 254, 255};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This converts a hue, saturation, and value (HSV) into 32 bit RGB (0xBBGGRR)
 *
 * @param hue Hue, the color, 0..255
 * @param sat Saturation, how intense the color is, 0..255
 * @param val Value, how bright the color is, 0..255
 * @param applyGamma Whether or not to apply gamma to the output
 * @return An RGB color (0xBBGGRR)
 */
uint32_t EHSVtoHEXhelper(uint8_t hue, uint8_t sat, uint8_t val, bool applyGamma)
{
    const uint8_t SIXTH1 = 43;
    const uint8_t SIXTH2 = 85;
    const uint8_t SIXTH3 = 128;
    const uint8_t SIXTH4 = 171;
    const uint8_t SIXTH5 = 213;
    uint16_t or = 0, og = 0, ob = 0;

    // move in rainbow order RYGCBM as hue from 0 to 255

    if (hue < SIXTH1) // Ok: Red->Yellow
    {
        or = 255;
        og = (hue * 255) / (SIXTH1);
    }
    else if (hue < SIXTH2) // Ok: Yellow->Green
    {
        og = 255;
        or = 255 - (hue - SIXTH1) * 255 / SIXTH1;
    }
    else if (hue < SIXTH3) // Ok: Green->Cyan
    {
        og = 255;
        ob = (hue - SIXTH2) * 255 / (SIXTH1);
    }
    else if (hue < SIXTH4) // Ok: Cyan->Blue
    {
        ob = 255;
        og = 255 - (hue - SIXTH3) * 255 / SIXTH1;
    }
    else if (hue < SIXTH5) // Ok: Blue->Magenta
    {
        ob = 255;
        or = (hue - SIXTH4) * 255 / SIXTH1;
    }
    else // Magenta->Red
    {
        or = 255;
        ob = 255 - (hue - SIXTH5) * 255 / SIXTH1;
    }

    // uint16_t rv = val;
    // if( rv > 128 )
    // {
    //     rv++;
    // }
    uint16_t rs = sat;
    if (rs > 128)
    {
        rs++;
    }

    // or, og, ob range from 0...255 now.
    // Apply saturation giving OR..OB == 0..65025
    or = or *rs + 255 * (256 - rs);
    og = og * rs + 255 * (256 - rs);
    ob = ob * rs + 255 * (256 - rs);
    or >>= 8;
    og >>= 8;
    ob >>= 8;
    // back to or, og, ob range 0...255 now.
    // Need to apply saturation and value.
    or = (or *val) >> 8;
    og = (og * val) >> 8;
    ob = (ob * val) >> 8;
    //  printf( "  hue = %d r=%d g=%d b=%d rs=%d rv=%d\n", hue, or, og, ob, rs, rv );
    if (applyGamma)
    {
        or = gamma_correction_table[or];
        og = gamma_correction_table[og];
        ob = gamma_correction_table[ob];
    }
    return or | (og << 8) | ((uint32_t)ob << 16);
    // return og | ( or << 8) | ((uint32_t)ob << 16); //grb
}

/**
 * @brief This converts a hue, saturation, and value (HSV) into led_t color
 *
 * @param hue Hue, the color, 0..255
 * @param sat Saturation, how intense the color is, 0..255
 * @param val Value, how bright the color is, 0..255
 * @param applyGamma Whether or not to apply gamma to the output
 * @return An led_t set to the RGB color
 */
led_t LedEHSVtoHEXhelper(uint8_t hue, uint8_t sat, uint8_t val, bool applyGamma)
{
    uint32_t r = EHSVtoHEXhelper((uint8_t)hue, sat, val, applyGamma);
    led_t ret;
    ret.r = r & 0xff;
    ret.g = (r >> 8) & 0xff;
    ret.b = (r >> 16) & 0xff;
    return ret;
}

/**
 * @brief This converts a hue, saturation, and value (HSV) into paletteColor_t color
 *
 * @param hue Hue, the color, 0..255
 * @param sat Saturation, how intense the color is, 0..255
 * @param val Value, how bright the color is, 0..255
 * @return The paletteColor_t closest to the HSV color
 */
paletteColor_t paletteHsvToHex(uint8_t hue, uint8_t sat, uint8_t val)
{
    return RGBtoPalette(EHSVtoHEXhelper(hue, sat, val, false));
}

/**
 * @brief Find the palette color closest to the given 32 bit RGB color
 *
 * @param rgb A 32 bit RGB color (0xBBGGRR)
 * @return The closest palette color to the given RGB color
 */
paletteColor_t RGBtoPalette(uint32_t rgb)
{
    uint8_t r = (rgb >> 0) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = (rgb >> 16) & 0xFF;
    r         = (r * 5) / 255;
    g         = (g * 5) / 255;
    b         = (b * 5) / 255;
    return (paletteColor_t)(r * 36) + (g * 6) + b;
}

/**
 * @brief Find the 32 bit RGB color for the given palette color
 *
 * @param pal A palette color
 * @return The 32 bit RGB representation of the palette color (0xBBGGRR)
 */
uint32_t paletteToRGB(paletteColor_t pal)
{
    uint8_t b = ((pal % 6) * 255) / 5;
    uint8_t g = (((pal / 6) % 6) * 255) / 5;
    uint8_t r = ((pal / 36) * 255) / 5;
    return (r << 16) | (g << 8) | (b);
}
