/**
 * @file swadgesona.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The data structures and Helper functions for utilizing Swadgesonas
 * @date 2025-05-02
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================
#include "stdlib.h"
#include "wsgPalette.h"

//==============================================================================
// Defines
//==============================================================================
#define SKIN_MAX_COLORS 3
#define HAIR_MAX_COLORS 3

//==============================================================================
// Enums
//==============================================================================
// TODO: Add all the ENUMs required fro the art assets
typedef enum
{
    HUMAN_EARS,
    ELF_EARS,
    CAT_EARS,
    DOG_EARS,
    COW_EARS,
    BUNNY_EARS,
    EAR_COUNT
} earsShape_t;

typedef enum 
{
    BOB_CUT,
    KAREN,
    JOSIAH,
    DEB,
    HALO,
    HAIR_COUNT
} hairStyle_t;

// Colors
typedef enum
{
    BLUE,
    GREEN,
    PURPLE,
    PEACH,
    SKIN_COLOR_COUNT
} skinColor_t;

typedef enum
{
    BLONDE,
    HAIR_COLOR_COUNT
} hairColor_t;

// Shape
typedef enum 
{
    ANIME_EYES,
    EYE_SHAPE_COUNT
} eyeShape_t;

typedef enum 
{
    THIN,
    THICK,
    EYEBROW_COUNT
} eyebrowShape_t;

typedef enum 
{
    MUPPET_NOSE,
    BEEEG_NOSE,
    NOSE_COUNT
} noseShape_t;

typedef enum 
{
    LIPSTICK1,
    LIPSTICK2,
    MOUTH_COUNT
} mouthShape_t;

typedef enum 
{
    MOLE1, // Different locations
    MOLE2,
    MOLE3,
    EARRINGS,
    FRECKLES,
    MOUSTACHE1,
    MOUSTACHE2,
    BEARD_SCRUFFY,
    BEARD_FLUFFY,
    EYEPATCH_LEFT,
    EYEPATCH_RIGHT,
    GLASSES,
    BODY_MARKS_COUNT
} bodyMarks_t;

// Clothes
// TODO: Decide if we're using this. Keeping in reserve.
typedef enum 
{
    BASIC,
    CLOTHES_OPTION_COUNT
} clothing_t;

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    // Color indexs
    // Each index specifies a palette, not a specific color
    // Use the grabPaletteFromIndex()
    skinColor_t skin;
    hairColor_t hairColor;
    // Normal paletteColor
    paletteColor_t eyeColor;
    
    // Clothes
    clothing_t clothes;
    paletteColor_t clothesColor;

    // Facial features
    earsShape_t earShape;
    eyeShape_t eyeShape;
    hairStyle_t hairStyle;
    eyebrowShape_t eyebrows;
    noseShape_t noseShape;
    mouthShape_t mouthShape;
    bodyMarks_t bodyMarks;

    // Name
    // Idx into namelist
    // First three are indexs, last one is random end number
    uint8_t name[4];
} swadgesona_t;

//==============================================================================
// Function Prototypes
//==============================================================================

// Data
/**
 * @brief Saves the Swadgesona to teh NVS in the given slot.
 * 
 * @param s Swadgesona data to save
 * @param idx Index of slot to put swadgesona into
 */
void saveSwadgesona(swadgesona_t* s, int idx);

/**
 * @brief Loads a swadgesona from the NVS
 * 
 * @param s Swadgesona data to load. Will be randomized if the data doesn't exist.
 * @param idx Index of slot to put swadgesona into
 */
void loadSwadgesona(swadgesona_t* s, int idx);

/**
 * @brief Generates a random Swadgesona automatically.
 * 
 * @param s Swadgesona to load data into.
 */
void generateRandomSwadgesona(swadgesona_t* s);

// Drawing 
/**
 * @brief Grabs the relevant colors based on the provided index for the skin
 * 
 * @param palette Palette to swap
 * @param idx Index to switch colors on
 */
void grabSkinPaletteFromIdx(wsgPalette_t* palette, int idx);

/**
 * @brief Grabs the relevant colors based on the provided index for the hair
 * 
 * @param palette Palette to swap
 * @param idx Index to switch colors on
 */
void grabHairPaletteFromIdx(wsgPalette_t* palette, int idx);

/**
 * @brief Draws the swadgesona at the given coordinates
 * 
 * @param s Swadgsona to draw
 * @param x x coordinate
 * @param y y coordinate
 */
void drawSwadgesona(swadgesona_t s, int x, int y);