/**
 * @file swadgesona.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com), Luna Toon (Natascha Santaella, makeupbyluna@hotmail.com)
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
#include "nameList.h"
#include "wsgPalette.h"

//==============================================================================
// Enums
//==============================================================================

// Colors

/// @brief List of valid skin colors
typedef enum
{
    SKIN_ZERO,       ///< Lightest skin tone
    SKIN_ONE,        ///< Roseacea
    SKIN_TWO,        ///< Second lightest skin tone
    SKIN_THREE,      ///< Medium skin tone
    SKIN_FOUR,       ///< Second darkest natural skin tone
    SKIN_FIVE,       ///< Darkest natural skin tone
    SKIN_BLUE,       ///< Blue skin tone
    SKIN_GRAY,       ///< Grey skin tone
    SKIN_GREEN,      ///< Green skin tone
    SKIN_PURPLE,     ///< Purple skin tone
    SKIN_RED,        ///< Red skin tone
    SKIN_PINK,       ///< Pink skin tone
    SKIN_COLOR_COUNT ///< Total number of skin tones
} skinColor_t;

/// @brief List of valid hair colors
typedef enum
{
    HAIR_LPINK,      ///< Pink
    HAIR_DPINK,      ///< Dark pink
    HAIR_LAVENDER,   ///< Light purple
    HAIR_PURPLE,     ///< Purple
    HAIR_LBLUE,      ///< Light blue
    HAIR_DBLUE,      ///< Dark blue
    HAIR_LGREEN,     ///< Light green
    HAIR_DGREEN,     ///< Dark green
    HAIR_ORANGE,     ///< Orange
    HAIR_RED,        ///< Red
    HAIR_BLONDE,     ///< Blonde
    HAIR_COPPER,     ///< Copper 
    HAIR_WHITE,      ///< White
    HAIR_GRAY,       ///< Grey (base)
    HAIR_BROWN,      ///< Brown
    HAIR_BLACK,      ///< Black
    HAIR_COLOR_COUNT ///< Total number of hair colors
} hairColor_t;

/// @brief List of valid eye colors
typedef enum
{
    EYES_BLACK,     ///< Black eye color
    EYES_BLUE,      ///< Blue eye color
    EYES_BROWN,     ///< Brown eye color
    EYES_GRAY,      ///< Gray eye color
    EYES_GREEN,     ///< Green eye color
    EYES_PINK,      ///< Pink eye color
    EYES_PURPLE,    ///< Purple eye color
    EYES_RED,       ///< Red eye color
    EYES_YELLOW,    ///< Yellow eye color
    EYE_COLOR_COUNT ///< Total number of eye colors
} eyeColor_t;

/// @brief List of valid clothes colors
typedef enum
{
    CLOTHES_RED,
    CLOTHES_GREEN,
    CLOTHES_COLOR_COUNT
} clothesColor_t;

// Images

/// @brief Ear variations
typedef enum
{
    EARS_HUMAN,
    EARS_ELF,
    EARS_CAT,
    EARS_DOG,
    EARS_COW,
    EARS_BUNNY,
    EAR_COUNT
} earsShape_t;

/// @brief hairstyle variations
typedef enum
{
    HS_BOB_CUT,
    HS_KAREN,
    HS_JOSIAH,
    HS_DEB,
    HS_HALO,
    HAIR_STYLE_COUNT
} hairStyle_t;

/// @brief List of eyebrow options
typedef enum
{
    EYEBROW_THIN,
    EYEBROW_THICK,
    EYEBROW_COUNT
} eyebrowShape_t;

/// @brief List of eye shapes
typedef enum
{
    EYES_ANIME,
    EYES_BIG,
    EYE_SHAPE_COUNT
} eyeShape_t;

/// @brief List of nose options
typedef enum
{
    NOSE_MUPPET,
    NOSE_BEEEG,
    NOSE_COUNT
} noseShape_t;

/// @brief List of mouth options
typedef enum
{
    MOUTH_LIPSTICK1,
    MOUTH_LIPSTICK2,
    MOUTH_COUNT
} mouthShape_t;

/// @brief Accessories, beauty marks, etc options
typedef enum
{
    BM_MOLE1, // Different locations
    BM_MOLE2,
    BM_MOLE3,
    BM_EARRINGS,
    BM_FRECKLES,
    BM_MOUSTACHE1,
    BM_MOUSTACHE2,
    BM_BEARD_SCRUFFY,
    BM_BEARD_FLUFFY,
    BM_EYEPATCH_LEFT,
    BM_EYEPATCH_RIGHT,
    BM_GLASSES,
    BODY_MARKS_COUNT
} bodyMarks_t;

/// @brief The list of clothes options
typedef enum
{
    CLOTHES_BASIC,
    CLOTHES_MAGFEST,
    CLOTHES_OPTION_COUNT
} clothing_t;

//==============================================================================
// Structs
//==============================================================================

/// @brief This struct is saved to the NVS. It should contain the bare minimum required to reconstruct a swadgesona
typedef struct __attribute__((packed))
{
    // Color indexs - Use the grabPaletteFromIndex()
    skinColor_t skin : 4; // 10 options
    hairColor_t hairColor : 4;
    eyeColor_t eyeColor : 4; // 10 options
    clothesColor_t clothesColor : 4;

    // Facial features
    earsShape_t earShape;
    hairStyle_t hairStyle;
    eyebrowShape_t eyebrows;
    eyeShape_t eyeShape;
    noseShape_t noseShape;
    mouthShape_t mouthShape;
    bodyMarks_t bodyMarks;
    clothing_t clothes;

    // Name
    int32_t packedName;
} swadgesona_t;

/// @brief Larger data we don't want to refresh every loop
typedef struct
{
    swadgesona_t* sona;
    nameData_t name;
    wsgPalette_t eyePalette;
    wsgPalette_t hairPalette;
    wsgPalette_t skinPalette;
    wsgPalette_t clothesPalette;
} swadgesonaWrapper_t;

/// @brief Contains images and data we don't want to refresh each time
typedef struct
{
    wsg_t* ears;
    wsg_t* hairstyles;
    wsg_t* eyebrows;
    wsg_t* eyes;
    wsg_t* noses;
    wsg_t* mouths;
    wsg_t* bodymarks;
    wsg_t* clothes;
    wsg_t body;
    swadgesonaWrapper_t* sw;
} swadgesonaData_t;

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
 * @brief Loads all relevant data (images, etc). Call this before using drawSwadgesona() and make sure to clean up with
 * freeSwadgesonaData()
 *
 * @param ssd Data to initialize
 */
void initSwadgesonaDraw(swadgesonaData_t* ssd);

/**
 * @brief Frees the images and other data. Call when exiting a mode.
 *
 * @param ssd swadge data to free
 */
void freeSwadgesonaDraw(swadgesonaData_t* ssd);

/**
 * @brief Draws the swadgesona at the given coordinates
 *
 * @param ssd Swadgesona data object
 * @param s Swadgesona to draw
 * @param x x coordinate
 * @param y y coordinate
 * @param scale Scale to draw at
 */
void drawSwadgesona(swadgesonaData_t* ssd, swadgesona_t* s, int x, int y, int scale);