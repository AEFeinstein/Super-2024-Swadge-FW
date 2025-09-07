/**
 * @file swadgesona.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The data structures and Helper functions for utilizing Swadgesonas
 * @date 2025-09-06
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

// C
#include <stdlib.h>

// Swadge
#include "cnfs_image.h"
#include "nameList.h"
#include "wsgPalette.h"

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
    CLOTHES_RED,
    CLOTHES_COLOR_COUNT
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

// Images
/// @brief Accessories, beauty marks, etc options
typedef enum
{
    BME_NONE,
    BME_BEARD,
    BME_BLUSH,
    BME_BOTTOM,
    BME_BOTTOM_MOLE,
    BME_CHIN,
    BME_CHIN_PATCH,
    BME_CHIN_STRAP,
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
    BME_OLD,
    BME_PILLOW,
    BME_S_AND_P,
    BME_SCRAGGLY,
    BME_SMALL_CURL,
    BME_SMALL_STACHE,
    BME_SOUL_PATCH,
    BME_STACHE_AND_STRAP,
    BME_STRONGMAN,
    BME_THIN_CHIN,
    BME_THIS,
    BME_TIRED,
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
    EE_BOOPED,
    EE_CAT,
    EE_CLOSED,
    EE_CLOSED_LASHES,
    EE_CLOSED_LINER,
    EE_CRAZY,
    EE_CROSSES,
    EE_CUTE,
    EE_DOOFY,
    EE_EXASPERATED,
    EE_HEARTS,
    EE_LINER,
    EE_MAKEUP,
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
    HE_BALLET_BUN,
    HE_BOWL_CUT,
    HE_CHIBI_USA,
    HE_COTTON_CANDY,
    HE_CURLY,
    HE_CUTE,
    HE_DOLLY,
    HE_DOWN_DREADS,
    HE_FRANKENSTEIN,
    HE_HINATA,
    HE_JINX,
    HE_LONG,
    HE_LONG_PIGS,
    HE_MAIN_CHARACTER,
    HE_MAIN_VILLAIN,
    HE_MALE_PATTERN,
    HE_MCR,
    HE_MIDDLE_BANG,
    HE_MINAKO,
    HE_MOHAWK,
    HE_POMPADOUR,
    HE_RAVEN,
    HE_SHORT,
    HE_SHORT_PIGS,
    HE_SIDE_PUFFS,
    HE_SKULL,
    HE_SKULL_R,
    HE_SMALL_BUNS,
    HE_SPOCK,
    HE_STAR_PUFF_N_B,
    HE_STAR_PUFF,
    HE_TATTOO,
    HE_THING,
    HE_USAGI,
    HE_WAVY_HAWK,
    HE_WAVY_HAWK_R,
    HE_WAVY_LONG,
    HE_WEDNESDAY,
    HE_WET_CURLY,
    HE_WET_SHORT,
    HE_COUNT
} hairStyle_t;

/// @brief Hat options
typedef enum
{
    HAE_NONE,
    HAE_BEANIE,
    HAE_CHEF,
    HAE_COOL_HAT,
    HAE_COWBOY,
    HAE_GRAD_CAP,
    HAE_HEART,
    HAE_PUFFBALL,
    HAE_COUNT,
} hat_t;

/// @brief List of mouth options
typedef enum
{
    ME_AH,
    ME_ANGEL_BITE,
    ME_BAR_PIERCING,
    ME_BITE,
    ME_BITE_PIERCING,
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

//==============================================================================
// Structs
//==============================================================================

/// @brief This struct is saved to the NVS. It should contain the bare minimum required to reconstruct a swadgesona
typedef struct __attribute__((packed))
{
    // Name
    int32_t packedName; // 32 bits

    // Color indexs - Use the grabPaletteFromIndex()
    skinColor_t skin      : 4;
    hairColor_t hairColor : 5;
    eyeColor_t eyeColor   : 4;
    hatColor_t hatColor   : 4;
    clothsColor_t clothes : 4;

    // Facial features
    bodyMarks_t bodyMarks   : 5;
    earsShape_t earShape    : 4;
    eyebrowShape_t eyebrows : 5;
    eyeShape_t eyeShape     : 5;
    hairStyle_t hairStyle   : 6;
    hat_t hat               : 4;
    mouthShape_t mouthShape : 5;

    // 32 + 21 + 34 = 87 / 8 = 11 bytes
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
    // - Clothes: c410, c310
    // - Hat:     c533, c514, c513, c502

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
 * @param s Swadgesona data to save. provide the entire wrapper
 * @param idx Index of slot to put swadgesona into
 */
void saveSwadgesona(swadgesona_t* sw, int idx);

/**
 * @brief Loads a swadgesona from the NVS
 *
 * @param s Swadgesona data to load. Will be randomized if the data doesn't exist.
 * @param idx Index of slot to put swadgesona into
 */
void loadSwadgesona(swadgesona_t* sw, int idx);

/**
 * @brief Generates a random Swadgesona automatically.
 *
 * @param s Swadgesona to load data into.
 */
void generateRandomSwadgesona(swadgesona_t* sw);

// Generate Swadgesona image
/**
 * @brief Generates the image based on the included data
 *
 * @param sw Swadgesona wrapper used ot generate the image
 */
void generateSwadgesonaImage(swadgesona_t* sw);