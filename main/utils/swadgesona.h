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
    BM_BEARD,
    BM_BLUSH,
    BM_BOTTOM,
    BM_BOTTOM_MOLE,
    BM_CHIN,
    BM_CHIN_PATCH,
    BM_CHIN_STRAP,
    BM_COP,
    BM_COWBOY,
    BM_EYE_MOLE,
    BM_FRECKLES,
    BM_FULL_SCRAGGLY,
    BM_HALF_STACHE,
    BM_HEART_STACHE,
    BM_LESS_WISE,
    BM_MAGICIAN,
    BM_MARILYN,
    BM_OLD,
    BM_PILLOW,
    BM_S_AND_P,
    BM_SCRAGGLY,
    BM_SMALL_CURL,
    BM_SMALL_STACHE,
    BM_SOUL_PATCH,
    BM_STACHE_AND_STRAP,
    BM_STRONGMAN,
    BM_THIN_CHIN,
    BM_THIS,
    BM_TIRED,
    BM_WISE_MAN,
    BM_COUNT
} bodyMarks_t;

/// @brief Ear variations
typedef enum
{
    EA_HUMAN,
    EA_BIG_HOOPS,
    EA_BUNNY,
    EA_CAT,
    EA_DOG,
    EA_DOWN_COW,
    EA_DWARF,
    EA_EARRINGS,
    EA_ELF,
    EA_LEFT,
    EA_MEDIUM_HOOP,
    EA_OPEN_COW,
    EA_RIGHT,
    EA_SMALL_HOOP,
    EA_COUNT
} earsShape_t;

/// @brief List of eyebrow options
typedef enum
{
    EB_ARCHED,
    EB_BUSHY,
    EB_CONCERN,
    EB_CUT_LEFT,
    EB_CUT_LEFT_PIERCING,
    EB_CUT_RIGHT,
    EB_CUT_RIGHT_PIERCING,
    EB_DOT,
    EB_DOWNTURNED,
    EB_HMMM,
    EB_MISCHIEVOUS,
    EB_ODD,
    EB_PUFFY,
    EB_SLIGHT_CONCERN,
    EB_THICC,
    EB_THIN,
    EB_TINY,
    EB_COUNT
} eyebrowShape_t;

/// @brief List of eye shapes
typedef enum
{
    E_ANGRY,
    E_ANGY,
    E_BABY,
    E_BIG,
    E_BIG_LINER,
    E_BOOPED,
    E_CAT,
    E_CLOSED,
    E_CLOSED_LASHES,
    E_CLOSED_LINER,
    E_CRAZY,
    E_CROSSES,
    E_CUTE,
    E_DOOFY,
    E_EXASPERATED,
    E_HEARTS,
    E_LINER,
    E_MAKEUP,
    E_SEXY,
    E_SEXY_LASHES,
    E_SLEEPING,
    E_SMALL_LASHES,
    E_SQUINTING,
    E_SQUINTING_LASHES,
    E_STARE,
    E_STARING,
    E_SWIRLS,
    E_THIN,
    E_WIDE,
    E_COUNT
} eyeShape_t;

/// @brief hairstyle variations
typedef enum
{
    H_BALLET_BUN,
    H_BOWL_CUT,
    H_CHIBI_USA,
    H_COTTON_CANDY,
    H_CURLY,
    H_CUTE,
    H_DOLLY,
    H_DOWN_DREADS,
    H_FRANKENSTEIN,
    H_HINATA,
    H_JINX,
    H_LONG,
    H_LONG_PIGS,
    H_MAIN_CHARACTER,
    H_MAIN_VILLAIN,
    H_MALE_PATTERN,
    H_MCR,
    H_MIDDLE_BANG,
    H_MINAKO,
    H_MOHAWK,
    H_POMPADOUR,
    H_RAVEN,
    H_SHORT,
    H_SHORT_PIGS,
    H_SIDE_PUFFS,
    H_SKULL,
    H_SKULL_R,
    H_SMALL_BUNS,
    H_SPOCK,
    H_STAR_PUFF_N_B,
    H_STAR_PUFF,
    H_TATTOO,
    H_THING,
    H_USAGI,
    H_WAVY_HAWK,
    H_WAVY_HAWK_R,
    H_WAVY_LONG,
    H_WEDNESDAY,
    H_WET_CURLY,
    H_WET_SHORT,
    H_COUNT
} hairStyle_t;

/// @brief Hat options
typedef enum
{
    HA_NONE,
    HA_BEANIE,
    HA_CHEF,
    HA_COOL_HAT,
    HA_COWBOY,
    HA_GRAD_CAP,
    HA_HEART,
    HA_PUFFBALL,
    HA_COUNT,
} hat_t;

/// @brief List of mouth options
typedef enum
{
    M_AH,
    M_ANGEL_BITE,
    M_BAR_PIERCING,
    M_BITE,
    M_BITE_PIERCING,
    M_CONCERN,
    M_CONTENT,
    M_DROOL,
    M_HALF_SMILE,
    M_KET,
    M_KISSES,
    M_LIP,
    M_LITTLE_DROOL,
    M_MISCHIEF,
    M_MLEM,
    M_NO_CUPID_BOW,
    M_OH,
    M_OH_PIERCING,
    M_OPEN_SMILE,
    M_POUTY,
    M_SAD,
    M_SAD_PIERCING,
    M_SATISFIED,
    M_SMILE,
    M_STOIC,
    M_TONGUE,
    M_TONGUE_PIERCING,
    M_UHM,
    M_VAMPIRE,
    M_YELLING,
    M_COUNT
} mouthShape_t;

//==============================================================================
// Structs
//==============================================================================

/// @brief This struct is saved to the NVS. It should contain the bare minimum required to reconstruct a swadgesona
typedef struct __attribute__((packed))
{
    // Name
    int32_t packedName;

    // Color indexs - Use the grabPaletteFromIndex()
    skinColor_t skin      : 4; // 12 options
    hairColor_t hairColor : 5; // 16 options
    eyeColor_t eyeColor   : 4; // 9 options
    hatColor_t hatColor   : 4; // 14 options

    // Facial features
    bodyMarks_t bodyMarks   : 5; // Count: 30 
    earsShape_t earShape    : 4; // Count: 14 
    eyebrowShape_t eyebrows : 5; // Count: 17 
    eyeShape_t eyeShape     : 5; // Count: 29 
    hairStyle_t hairStyle   : 6; // Count: 40 
    hat_t hat               : 4; // Count: 8 
    mouthShape_t mouthShape : 5; // Count: 30 
} swadgesonaCore_t;

/// @brief Larger data for use of use
typedef struct
{
    swadgesonaCore_t* core; ///< Data that is saved to the NVS
    nameData_t name;        ///< The name of the swadgesona
    wsgPalette_t pal;       ///< Palette for changing colors of images
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