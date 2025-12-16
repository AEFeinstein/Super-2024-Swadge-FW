/**
 * @file swadgesona.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The data structures and Helper functions for utilizing Swadgesonas
 * @date 2025-10-5
 *
 * @copyright Copyright (c) 2025
 *
 */

/*! \file swadgesona.h
 *
 * \section sona_overview Overview
 *
 * Swadgesonas are a Mii-like character that can be edited and personalized. The Swadgesonas are made to be as small as
 * possible to be easily sent via swadgepass, only 12 bytes, and contain all the data that another swadge running the
 * same software needs to reconstruct the sona.
 *
 * Sonas are 64x64 pixels. Recommend to scale them up when space permits to see the details.
 *
 * The customizable aspects of a sona are:
 * - Skin color
 * - Hair style and color
 * - Eye shape and color
 * - Eyebrow shape
 * - Ear type (Cat, Dog, Human, Elf, etc.)
 * - Mouth shape
 * - Glasses and colour
 * - Hats and hat colors
 * - Some beauty marks
 *
 * The Swadgesonas are assembled by the system, so all the user needs to do is provide a space to load the sona into.
 * The Sonas can easily be loaded and saved into the NVS and also regenerated if modifications have been made. Lastly, a
 * random swadgesona can easily be generated to avoid always seeing the default when loading one up before data is
 * initialized.
 *
 * \code {.c}
// Data
swadgesona_t sw; // The swadgesona object
int saveSlot = 4; // You can save to any number of slots. They take 12 bytes each, so make sure there's space!
bool drawBody = true; // If the body should be drawn. If set to false, you get a floating head!

// Load from file
loadSwadgesona(&sw, saveSlot);

// Save to a slot
saveSwadgesona(&sw, saveSlot);

// Regenerate swadgesona image. Only need ot call this when modifying the sona, when loading a sona this function is
// automatically called ***with*** a body. If you need a body without, use this with the drawBody set to false.
generateSwadgesonaImage(&sw, drawBody);

// Draw the sona
drawWsgSimpleScaled(&sw->image, x, y, 3, 3); // Scaled to 3 times

// Generate a random swadgesona
generateRandomSwadgesona(&sw);

 * \endcode
 *
 * Refer to the enums below for the current list of options.
 *
 * When adding new options, here's the process:
 * - Add new item to relevant enum
 * - Add new CNFS image to the .c file at the appropriate place in the array
 * - Calculate the number of bits required to save the sonas in this .h file.
 * - If adding a color, make sure to add the conversions to the palette generation function
 *
 * If any help is required, try contacting the MAGFest discord, MAGFest slack, or Jeremy Stintzcum (Johnny Wycliffe) for
 * direct assistance. There are plenty of people willing to help!
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

// C
#include <stdlib.h>

// Swadge
#include "cnfs_image.h"
#include "nameList.h"
#include "wsgPalette.h"

extern const char spSonaNVSKey[];

//==============================================================================
// Enums
//==============================================================================

// Colors
/// @brief List of palette changes
typedef enum
{
    // Skins
    SKIN_ZERO,
    SKIN_ONE,
    SKIN_TWO,
    SKIN_THREE,
    SKIN_FOUR,
    SKIN_FIVE,
    SKIN_BLUE,
    SKIN_GRAY,
    SKIN_GREEN,
    SKIN_PINK,
    SKIN_PURPLE,
    SKIN_RED,
    SKIN_ALBINO,
    SKIN_MAUVE,
    SKIN_COLOR_COUNT
} skinColor_t;

/// @brief List of valid hair colors
typedef enum
{
    HAIR_GRAY,
    HAIR_BLONDE,
    HAIR_ORANGE,
    HAIR_RED,
    HAIR_DARK_RED,
    HAIR_BROWN,
    HAIR_BLACK,
    HAIR_WHITE,
    HAIR_PINK,
    HAIR_HOT_PINK,
    HAIR_PURPLE,
    HAIR_DARK_PURPLE,
    HAIR_TEAL,
    HAIR_BLUE,
    HAIR_GREEN,
    HAIR_DARK_GREEN,
    HAIR_COLOR_COUNT
} hairColor_t;

/// @brief List of valid eye colors
typedef enum
{
    EYES_BLACK,
    EYES_BLUE,
    EYES_BROWN,
    EYES_GRAY,
    EYES_GREEN,
    EYES_PINK,
    EYES_PURPLE,
    EYES_RED,
    EYES_YELLOW,
    EYE_COLOR_COUNT ///< Total number of eye colors
} eyeColor_t;

/// @brief List of valid clothes colors
typedef enum
{
    C_BLACK,
    C_BLUE,
    C_BROWN,
    C_CYAN,
    C_DARK_PINK,
    C_DARK_PURPLE,
    C_GRAY,
    C_GREEN,
    C_HOT_PINK,
    C_HOTTER_PINK,
    C_LIGHT_TEAL,
    C_LIME,
    C_OFF_BLACK,
    C_ORANGE,
    C_PEACH,
    C_PINK,
    C_PURPLE,
    C_RED,
    C_TAN,
    C_TEAL,
    C_WHITE,
    C_YELLOW,
    C_COUNT
} clothsColor_t;

/// @brief List of valid hat colors
typedef enum
{
    HA_BLUE,
    HA_CYAN,
    HA_DARK_BLUE,
    HA_DARK_GREEN,
    HA_GRAY,
    HA_GREEN,
    HA_HOT_PINK,
    HA_MAUVE,
    HA_ORANGE,
    HA_PALE_BLUE,
    HA_PALE_YELLOW,
    HA_PINK,
    HA_PURPLE,
    HA_YELLOW,
    HA_COLOR_COUNT
} hatColor_t;

/// @brief List of valid glasses colors
typedef enum
{
    GC_BLACK, // c000
    GC_BROWN, // c211
    GC_RED,   // c410
    GC_COUNT
} glassesColor_t;

// Images
/// @brief Accessories, beauty marks, etc options
typedef enum
{
    BME_NONE,
    BME_AVATAR,
    BME_BEARD,
    BME_BLUSH,
    BME_BOTTOM,
    BME_BOTTOM_MOLE,
    BME_CHIN,
    BME_CHIN_PATCH,
    BME_CHIN_STRAP,
    BME_CHOKER,
    BME_COP,
    BME_COWBOY,
    BME_EYE_MOLE,
    BME_FRECKLES,
    BME_FULL_SCRAGGLY,
    BME_HALF_STACHE,
    BME_HEART_STACHE,
    BME_LESS_WISE,
    BME_MAGICIAN,
    BME_MARILYN,
    BME_NECK_BLOOD,
    BME_OLD,
    BME_PILLOW,
    BME_S_AND_P,
    BME_SCRAGGLY,
    BME_SMALL_CURL,
    BME_SMALL_STACHE,
    BME_SOUL_PATCH,
    BME_SPIKED_NECKLACE,
    BME_STACHE_AND_STRAP,
    BME_STRONGMAN,
    BME_THIN_CHIN,
    BME_THIS,
    BME_TIRED,
    BME_VITILIGO,
    BME_WISE_MAN,
    BME_COUNT
} bodyMarks_t;

/// @brief Ear variations
typedef enum
{
    EAE_HUMAN,
    EAE_BIG_HOOPS,
    EAE_BUNNY,
    EAE_CAT,
    EAE_DOG,
    EAE_DOWN_COW,
    EAE_DWARF,
    EAE_EARRINGS,
    EAE_ELF,
    EAE_LEFT,
    EAE_MEDIUM_HOOP,
    EAE_OPEN_COW,
    EAE_RIGHT,
    EAE_SMALL_HOOP,
    EAE_COUNT
} earsShape_t;

/// @brief List of eyebrow options
typedef enum
{
    EBE_ARCHED,
    EBE_BUSHY,
    EBE_CONCERN,
    EBE_CUT_LEFT,
    EBE_CUT_LEFT_PIERCING,
    EBE_CUT_RIGHT,
    EBE_CUT_RIGHT_PIERCING,
    EBE_DOT,
    EBE_DOWNTURNED,
    EBE_HMMM,
    EBE_MISCHIEVOUS,
    EBE_ODD,
    EBE_PUFFY,
    EBE_SLIGHT_CONCERN,
    EBE_THICC,
    EBE_THIN,
    EBE_TINY,
    EBE_COUNT
} eyebrowShape_t;

/// @brief List of eye shapes
typedef enum
{
    EE_ANGRY,
    EE_ANGY,
    EE_BABY,
    EE_BIG,
    EE_BIG_LINER,
    EE_BLOOD,
    EE_BOOPED,
    EE_CAT,
    EE_CLOSED,
    EE_CLOSED_LASHES,
    EE_CLOSED_LINER,
    EE_CRAZY,
    EE_CRYING,
    EE_CROSSES,
    EE_CUTE,
    EE_DOOFY,
    EE_EXASPERATED,
    EE_HEARTS,
    EE_LINER,
    EE_MAKEUP,
    EE_MY_EYES,
    EE_RANDOMIZER,
    EE_SEXY,
    EE_SEXY_LASHES,
    EE_SLEEPING,
    EE_SMALL_LASHES,
    EE_SQUINTING,
    EE_SQUINTING_LASHES,
    EE_STARE,
    EE_STARING,
    EE_SWIRLS,
    EE_THIN,
    EE_WIDE,
    EE_COUNT
} eyeShape_t;

/// @brief hairstyle variations
typedef enum
{
    HE_NONE,
    HE_BALLET_BUN,
    HE_BOWL_CUT,
    HE_CHIBI_USA,
    HE_CURLY,
    HE_CUTE,
    HE_CUTE_BANGS,
    HE_DOLLY,
    HE_DOWN_DREADS,
    HE_DOWN_DREADS_R,
    HE_FRANKENSTEIN,
    HE_FRO,
    HE_HINATA,
    HE_JINX,
    HE_LONG,
    HE_LONG_PIGS,
    HE_MAIN_CHARACTER,
    HE_MAIN_CHARACTER_R,
    HE_MAIN_VILLAIN,
    HE_MAIN_VILLAIN_R,
    HE_MALE_PATTERN,
    HE_MCR,
    HE_MINAKO,
    HE_MOHAWK,
    HE_POMPADOUR,
    HE_PONYTAIL,
    HE_PONYTAIL_NO_BANGS,
    HE_RAVEN,
    HE_SHORT,
    HE_SHORT_PIGS,
    HE_SIDE_PUFFS,
    HE_SIDE_PUFFS_R,
    HE_SKULL,
    HE_SKULL_R,
    HE_SMALL_BUNS,
    HE_SPOCK,
    HE_STAR_PUFF_N_B,
    HE_STAR_PUFF,
    HE_STAR_PUFFS_R,
    HE_TATTOO,
    HE_THING,
    HE_USAGI,
    HE_VBANG,
    HE_WAVY_HAWK,
    HE_WAVY_HAWK_R,
    HE_WAVY_SHORT,
    HE_WAVY_LONG,
    HE_WEDNESDAY,
    HE_WEDNESDAY_R,
    HE_WET_CURLY,
    HE_WET_SHORT,
    HE_COUNT
} hairStyle_t;

/// @brief Hat options
typedef enum
{
    HAE_NONE,
    HAE_ANGEL,
    HAE_BATTRICE,
    HAE_BEANIE,
    HAE_BIGMA,
    HAE_BLITZO_WSG,
    HAE_CHAOS_GOBLIN,
    HAE_CHEF,
    HAE_COOL_HAT,
    HAE_COWBOY,
    HAE_DEVIL,
    HAE_GARBOTNIK,
    HAE_GRAD_CAP,
    HAE_HEART,
    HAE_HOMESTUCK,
    HAE_KINETIC_DONUT,
    HAE_MET_HELMET,
    HAE_MILLIE,
    HAE_MINI_HOMESTUCK,
    HAE_MOXXIE,
    HAE_PUFFBALL,
    HAE_PULSE,
    HAE_SANS,
    HAE_SAWTOOTH,
    HAE_TALL_HOMESTUCK,
    HAE_TENNA,
    HAE_TINY_HOMESTUCK,
    HAE_TRON,
    HAE_TV_HEAD,
    HAE_VEROSIKA,
    HAE_WIDE_HOMESTUCK,
    HAE_COUNT
} hat_t;

/// @brief List of mouth options
typedef enum
{
    ME_AH,
    ME_ANGEL_BITE,
    ME_BAR_PIERCING,
    ME_BITE,
    ME_BITE_PIERCING,
    ME_CENSOR,
    ME_CONCERN,
    ME_CONTENT,
    ME_DROOL,
    ME_HALF_SMILE,
    ME_KET,
    ME_KISSES,
    ME_LIP,
    ME_LITTLE_DROOL,
    ME_MISCHIEF,
    ME_MLEM,
    ME_NO_CUPID_BOW,
    ME_OH,
    ME_OH_PIERCING,
    ME_OPEN_SMILE,
    ME_POUTY,
    ME_SAD,
    ME_SAD_PIERCING,
    ME_SATISFIED,
    ME_SMILE,
    ME_STOIC,
    ME_TONGUE,
    ME_TONGUE_PIERCING,
    ME_UHM,
    ME_VAMPIRE,
    ME_YELLING,
    ME_COUNT
} mouthShape_t;

/// @brief Glasses options
typedef enum
{
    G_NONE,
    G_3D,
    G_ANIME,
    G_BANDAGE,
    G_BIG,
    G_BIGANGLE,
    G_BIGANGLE_SUN,
    G_BIGSQUARE,
    G_BIGSQUARE_SUN,
    G_BLACK_SUN,
    G_CUTE_PATCH,
    G_EGGMAN,
    G_GOEORDI,
    G_LINDA,
    G_LINDA_SUN,
    G_LOW,
    G_LOW_SUN,
    G_PATCH,
    G_RAYBAN,
    G_RAYBAN_SUN,
    G_READING,
    G_SCOUTER,
    G_SMALL,
    G_SQUARE,
    G_SQUARE_SUN,
    G_SQUIRTLE,
    G_THINANGLE,
    G_THINANGLE_SUN,
    G_UPTURNED,
    G_UPTURNED_SUN,
    G_WIDENOSE,
    G_WIDENOSE_SUN,
    G_COUNT
} glasses_t;

//==============================================================================
// Structs
//==============================================================================

/// @brief This struct is saved to the NVS. It should contain the bare minimum required to reconstruct a swadgesona
typedef struct __attribute__((packed))
{
    // Name
    int32_t packedName; // 32 bits

    // Color indexs - Use the grabPaletteFromIndex()
    skinColor_t skin            : 4;
    hairColor_t hairColor       : 5;
    eyeColor_t eyeColor         : 4;
    hatColor_t hatColor         : 4;
    clothsColor_t clothes       : 5;
    glassesColor_t glassesColor : 2;

    // Facial features
    bodyMarks_t bodyMarks   : 6;
    earsShape_t earShape    : 4;
    eyebrowShape_t eyebrows : 5;
    eyeShape_t eyeShape     : 6;
    hairStyle_t hairStyle   : 6;
    hat_t hat               : 5;
    mouthShape_t mouthShape : 5;
    glasses_t glasses       : 6;

    // 24 + 43 + 32 = 99 bits / 8 = 13 bytes
} swadgesonaCore_t;

/// @brief Larger data for use of use
typedef struct
{
    swadgesonaCore_t core; ///< Data that is saved to the NVS
    nameData_t name;       ///< The name of the swadgesona
    wsgPalette_t pal;      ///< Palette for changing colors of images
    // Palette color spaces:
    // - Hair:    c333, c222, c111
    // - Skin:    c544, c422
    // - Eyes:    c130, c010
    // - Clothes: c401, c301, c000
    // - Hat:     c533, c514, c513, c502
    // - Glasses: c410

    // Finished image
    wsg_t image;
} swadgesona_t;

//==============================================================================
// Function Prototypes
//==============================================================================

// Data
/**
 * @brief Saves the Swadgesona to the NVS in the given slot.
 *
 * @param sw Swadgesona data to save. provide the entire wrapper
 * @param idx Index of slot to put swadgesona into
 */
void saveSwadgesona(swadgesona_t* sw, int idx);

/**
 * @brief Loads a swadgesona from the NVS
 *
 * @param sw Swadgesona data to load. Will be randomized if the data doesn't exist.
 * @param idx Index of slot to put swadgesona into
 */
void loadSwadgesona(swadgesona_t* sw, int idx);

/**
 * @brief Copy one swadgesona to another
 *
 * @param to The swadgesona to copy to
 * @param from The source swadgesona
 */
void copySwadgesona(swadgesona_t* to, swadgesona_t* from);

/**
 * @brief Generates a random Swadgesona automatically.
 *
 * @param sw Swadgesona to load data into.
 */
void generateRandomSwadgesona(swadgesona_t* sw);

// Generate swadgesona image
/**
 * @brief Generates the image based on the included data
 *
 * @param sw Swadgesona wrapper used ot generate the image
 * @param drawBody Whether or not to draw the shirt/neck
 */
void generateSwadgesonaImage(swadgesona_t* sw, bool drawBody);

/**
 * @brief Loads the swadgepass sona
 *
 * @param sw Data out. Is set to NULL if nothing is loaded
 */
void loadSPSona(swadgesonaCore_t* sw);

// Get indexes
/**
 * @brief Get the hair CNFS index from the swadgesona for drawing behind custom bodies
 *
 * @param sw Swadgesona to extract wsg from
 * @return cnfsFileIdx_t index into the CNFS system where the hairstyle is at.
 */
cnfsFileIdx_t getHairWSG(swadgesona_t* sw);
