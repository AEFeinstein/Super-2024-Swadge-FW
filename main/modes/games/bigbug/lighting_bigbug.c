//==============================================================================
// Includes
//==============================================================================

#include "lighting_bigbug.h"

//==============================================================================
// Functions
//==============================================================================
inline uint8_t bb_midgroundLighting(vec_t* lookup, int32_t* garbotnikRotation)
{
    return 0;
}

inline uint8_t bb_foregroundLighting(vec_t* lookup, int32_t* garbotnikRotation)
{
    uint8_t brightness = 0;
    
    if (garbotnikRotation < -720)
    {
        int32_t r = 0;
        lookup.x += 20; // Shift lookup texture left for red channel
        // if within bounds of the headlamp texture...
        if (lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106)
        {
            uint32_t rgbCol
                = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
            r = (rgbCol >> 16) & 0xFF;
        }
        int32_t g = 0;
        lookup.x += 36; // Shift lookup texture left again for green channel.
        if (lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106)
        {
            uint32_t rgbCol
                = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
            g = (rgbCol >> 8) & 0xFF;
        }
        // >> 16 & 0xFF gets red   channel
        // >> 8  & 0xFF gets green channel
        // >>    & 0xFF gets blue  channel
        brightness += (r + ((g - r) * (garbotnikRotation->x + 1440)) / 720) / 51;
        lookup.x -= 56;
    }
    else if (garbotnikRotation->x < 0)
    {
        int32_t shift = ((-(-garbotnikRotation->x << DECIMAL_BITS) / 384) * 30) >> DECIMAL_BITS;
        lookup.x -= shift; // Shift lookup texture left for green channel
        if (lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106)
        {
            uint32_t rgbCol
                = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
            brightness += ((rgbCol >> 8) & 0xFF) / 51;
        }
        lookup.x += shift;
    }
    else if (garbotnikRotation->x < 720)
    {
        int32_t shift = (((garbotnikRotation->x << DECIMAL_BITS) / 384) * 30) >> DECIMAL_BITS;
        lookup.x -= shift; // Shift lookup texture left for green channel
        if (lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106)
        {
            uint32_t rgbCol
                = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
            brightness += ((rgbCol >> 8) & 0xFF) / 51;
        }
        lookup.x += shift;
    }
    else
    {
        int32_t b = 0;
        lookup.x -= 20; // Shift lookup texture left for blue channel.
        if (lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106)
        {
            uint32_t rgbCol
                = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
            b = rgbCol & 0xFF;
        }
        int32_t g = 0;
        lookup.x -= 36; // Shift lookup texture left for green channel
        // if within bounds of the headlamp texture...
        if (lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106)
        {
            uint32_t rgbCol
                = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
            g = (rgbCol >> 8) & 0xFF;
        }
        // >> 16 & 0xFF gets red   channel
        // >> 8  & 0xFF gets green channel
        // >>    & 0xFF gets blue  channel
        brightness += (g + ((b - g) * (garbotnikRotation->x - 720)) / 720) / 51;
        lookup.x += 56;
    }

    return brightness;
}