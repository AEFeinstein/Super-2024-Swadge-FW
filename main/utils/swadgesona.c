/**
 * @file swadgesona.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The data structures and Helper functions for utilizing Swadgesonas
 * @date 2025-10-5
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "swadgesona.h"

// C
#include <stdio.h>
#include <inttypes.h>

// ESP
#include <esp_log.h>
#include <esp_random.h>

// Swadge
#include "fs_wsg.h"
#include "hdw-nvs.h"
#include "wsgCanvas.h"

//==============================================================================
// Defines
//==============================================================================

#define SWSN_HEIGHT 64
#define SWSN_WIDTH  64

//==============================================================================
// Consts
//==============================================================================

static const char* const nvsStr[] = {"swadgesona", "swadgesona-"};
const char spSonaNVSKey[]         = "spSona";

static const cnfsFileIdx_t bodymarksWsgs[] = {
    BM_AVATAR_WSG,       BM_BEARD_WSG,      BM_BLUSH_WSG,           BM_BOTTOM_MOLE_WSG,      BM_BOTTOM_WSG,
    BM_CHIN_WSG,         BM_CHIN_PATCH_WSG, BM_CHIN_STRAP_WSG,      BM_CHOKER_WSG,           BM_COP_WSG,
    BM_COWBOY_WSG,       BM_EYE_MOLE_WSG,   BM_FRECKLES_WSG,        BM_FULL_SCRAGGLY_WSG,    BM_HALF_STACHE_WSG,
    BM_HEART_STACHE_WSG, BM_LESS_WISE_WSG,  BM_MAGICIAN_WSG,        BM_MARILYN_WSG,          BM_NECK_BLOOD_WSG,
    BM_OLD_WSG,          BM_PILLOW_WSG,     BM_SAND_P_WSG,          BM_SCRAGGLY_WSG,         BM_SMALL_CURL_WSG,
    BM_SMALL_STACHE_WSG, BM_SOUL_PATCH_WSG, BM_SPIKED_NECKLACE_WSG, BM_STACHE_AND_STRAP_WSG, BM_STRONGMAN_WSG,
    BM_THIN_CHIN_WSG,    BM_THIS_WSG,       BM_TIRED_WSG,           BM_VITILIGO_WSG,         BM_WISEMAN_WSG,
};
static const cnfsFileIdx_t earWsgs[] = {
    EA_BIG_HOOP_WSG, EA_BUNNY_WSG,    EA_CAT_WSG,        EA_DOG_WSG,  EA_DOWN_COW_WSG,
    EA_DWARF_WSG,    EA_EARRINGS_WSG, EA_ELF_WSG,        EA_LEFT_WSG, EA_MEDIUM_HOOP_WSG,
    EA_OPEN_COW_WSG, EA_RIGHT_WSG,    EA_SMALL_HOOP_WSG,
};
static const cnfsFileIdx_t eyebrowsWsgs[] = {
    EB_ARCHED_WSG,
    EB_BUSHY_WSG,
    EB_CONCERN_WSG,
    EB_CUT_LEFT_WSG,
    EB_CUT_LEFT_PIERCING_WSG,
    EB_CUT_RIGHT_WSG,
    EB_CUT_RIGHT_PIERCING_WSG,
    EB_DOT_WSG,
    EB_DOWNTURNED_WSG,
    EB_HMM_WSG,
    EB_MISCHIEVOUS_WSG,
    EB_ODD_WSG,
    EB_PUFFY_WSG,
    EB_SLIGHT_CONCERN_WSG,
    EB_THICC_WSG,
    EB_THIN_WSG,
    EB_TINY_WSG,
};
static const cnfsFileIdx_t eyeWsgs[] = {
    E_ANGRY_WSG,        E_ANGY_WSG,
    E_BABY_WSG,         E_BIG_WSG,
    E_BIG_LINER_WSG,    E_BLOOD_WSG,
    E_BOOPED_WSG,       E_CAT_WSG,
    E_CLOSED_WSG,       E_CLOSED_LASHES_WSG,
    E_CLOSED_LINER_WSG, E_CRAZY_WSG,
    E_CRYING_WSG,       E_CROSSES_WSG,
    E_CUTE_WSG,         E_DOOFY_WSG,
    E_EXASPERATED_WSG,  E_HEARTS_WSG,
    E_LINER_WSG,        E_MAKEUP_WSG,
    E_SEXY_WSG,         E_SEXY_LASHES_WSG,
    E_SLEEPING_WSG,     E_SMALL_WLASHES_WSG,
    E_SQUINTING_WSG,    E_SQUINTING_LASHES_WSG,
    E_STARE_WSG,        E_STARING_WSG,
    E_SWIRLS_WSG,       E_THIN_WSG,
    E_WIDE_WSG,
};
static const cnfsFileIdx_t hairWsgs[] = {
    H_BALLET_BUN_WSG,
    H_BOWL_CUT_WSG,
    H_CHIBIUSA_WSG,
    H_CURLY_WSG,
    H_CUTE_WSG,
    H_CUTE_BANGS_WSG,
    H_DOLLY_WSG,
    H_DOWN_DREADS_WSG,
    H_DOWN_DREADS_R_WSG,
    H_FRANKEY_STEIN_WSG,
    H_FRO_WSG,
    H_HINATA_WSG,
    H_JINX_WSG,
    H_LONG_WSG,
    H_LONG_PIGS_WSG,
    H_MAIN_CHARACTER_WSG,
    H_MAIN_CHARACTER_R_WSG,
    H_MAIN_VILLAIN_WSG,
    H_MAIN_VILLAIN_R_WSG,
    H_MALE_PATTERN_WSG,
    H_MCR_WSG,
    H_MINAKO_WSG,
    H_MOHAWK_WSG,
    H_POMPADOUR_WSG,
    H_RAVEN_WSG,
    H_SHORT_WSG,
    H_SHORT_PIGS_WSG,
    H_SIDE_PUFFS_WSG,
    H_SIDE_PUFFS_R_WSG,
    H_SKULL_WSG,
    H_SKULL_R_WSG,
    H_SMALL_BUNS_WSG,
    H_SPOCK_WSG,
    H_STAR_PUFF_NB_WSG,
    H_STAR_PUFFS_WSG,
    H_STAR_PUFFS_R_WSG,
    H_TATTOO_WSG,
    H_THING_WSG,
    H_VBANG_WSG,
    H_USAGI_WSG,
    H_WAVY_HAWK_WSG,
    H_WAVY_HAWK_R_WSG,
    H_WAVY_SHORT_WSG,
    H_WAVY_LONG_WSG,
    H_WEDNESDAY_WSG,
    H_WEDNESDAY_R_WSG,
    H_WET_CURLY_WSG,
    H_WET_SHORT_WSG,
};
static const cnfsFileIdx_t hatWsgs[] = {
    HA_ANGEL_WSG,         HA_BATTRICE_WSG,       HA_BEANIE_WSG,         HA_BIGMA_WSG,
    HA_CHEF_WSG,          HA_COOL_HAT_WSG,       HA_COWBOY_WSG,         HA_DEVIL_WSG,
    HA_GARBOTNIK_WSG,     HA_GRAD_CAP_WSG,       HA_HEART_WSG,          HA_HOMESTUCK_WSG,
    HA_KINETIC_DONUT_WSG, HA_MET_HELMET_WSG,     HA_MINI_HOMESTUCK_WSG, HA_PUFFBALL_BEANIE_WSG,
    HA_PULSE_WSG,         HA_SAWTOOTH_WSG,       HA_TALL_HOMESTUCK_WSG, HA_TINY_HOMESTUCK_WSG,
    HA_TRON_WSG,          HA_WIDE_HOMESTUCK_WSG,
};
static const cnfsFileIdx_t mouthWsgs[] = {
    M_AH_WSG,
    M_ANGEL_BITE_WSG,
    M_BAR_PIERCING_WSG,
    M_BITE_WSG,
    M_BITE_PIERCING_WSG,
    M_CENSOR_WSG,
    M_CONCERN_WSG,
    M_CONTENT_WSG,
    M_DROOL_WSG,
    M_HALF_SMILE_WSG,
    M_KET_WSG,
    M_KISSES_WSG,
    M_LIP_WSG,
    M_LITTLE_DROOL_WSG,
    M_MISCHIEF_WSG,
    M_MLEM_WSG,
    M_NO_CUPID_BOW_WSG,
    M_OH_WSG,
    M_OH_PIERCING_WSG,
    M_OPEN_SMILE_WSG,
    M_POUTY_WSG,
    M_SAD_WSG,
    M_SAD_PIERCING_WSG,
    M_SATISFIED_WSG,
    M_SMILE_WSG,
    M_STOIC_WSG,
    M_TONGUE_WSG,
    M_TONGUE_PIERCING_WSG,
    M_UHM_WSG,
    M_VAMPIRE_WSG,
    M_YELLING_WSG,
};
static const cnfsFileIdx_t glassesWsgs[] = {
    G_3D_WSG,
    G_ANIME_WSG,
    G_BANDAGE_WSG,
    G_BIG_WSG,
    G_BIG_ANGLE_WSG,
    G_BIG_ANGLE_SUN_WSG,
    G_BIG_SQUARE_WSG,
    G_BIG_SQUARE_SUN_WSG,
    G_BLACK_SUN_WSG,
    G_CUTE_PATCH_WSG,
    G_EGGMAN_WSG,
    G_GOEORDI_WSG,
    G_LINDA_WSG,
    G_LINDA_SUN_WSG,
    G_LOW_WSG,
    G_LOW_SUN_WSG,
    G_PATCH_WSG,
    G_RAY_BAN_WSG,
    G_RAY_BAN_SUN_WSG,
    G_READING_WSG,
    G_SCOUTER_WSG,
    G_SMALL_WSG,
    G_SQUARE_WSG,
    G_SQUARE_SUN_WSG,
    G_SQUIRTLE_SQUAD_WSG,
    G_THIN_ANGLE_WSG,
    G_THIN_ANGLE_SUN_WSG,
    G_UPTURNED_WSG,
    G_UPTURNED_SUN_WSG,
    G_WIDE_NOSE_WSG,
    G_WIDE_NOSE_SUN_WSG,
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    COLOR_HAIR,
    COLOR_SKIN,
    COLOR_EYES,
    COLOR_CLOTHES,
    COLOR_HAT,
    COLOR_GLASSES,
} paletteSwap_t;

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Grabs the relevant colors based on the provided index for the skin
 *
 * @param palette Palette to swap
 * @param ps Which part of the palette to swap
 * @param idx Index to switch colors on
 */
static void _getPaletteFromIdx(wsgPalette_t* palette, paletteSwap_t ps, int idx);

//==============================================================================
// Functions
//==============================================================================

// Data
void saveSwadgesona(swadgesona_t* sw, int idx)
{
    // Generate NVS key
    char nvsTag[NVS_KEY_NAME_MAX_SIZE];
    snprintf(nvsTag, NVS_KEY_NAME_MAX_SIZE - 1, "%s%" PRIu8, nvsStr[1], idx);
    if (!writeNamespaceNvsBlob(nvsStr[0], nvsTag, &sw->core, sizeof(swadgesonaCore_t)))
    {
        ESP_LOGE("SONA", "Swadgesona failed to save");
    }
}

void loadSwadgesona(swadgesona_t* sw, int idx)
{
    char nvsTag[NVS_KEY_NAME_MAX_SIZE];
    size_t len = sizeof(sw->core);
    snprintf(nvsTag, NVS_KEY_NAME_MAX_SIZE - 1, "%s%" PRIu8, nvsStr[1], idx);

    if (!readNamespaceNvsBlob(nvsStr[0], nvsTag, &sw->core, &len))
    {
        ESP_LOGE("SONA", "Swadgesona failed to Load/does not exist");
        generateRandomSwadgesona(sw);
    }

    // Ensure the image is generated and the name is generated
    generateSwadgesonaImage(sw, true);
    setUsernameFrom32(&sw->name, sw->core.packedName);
}

void copySwadgesona(swadgesona_t* to, swadgesona_t* from)
{
    to->core.skin       = from->core.skin;
    to->core.hairColor  = from->core.hairColor;
    to->core.eyeColor   = from->core.eyeColor;
    to->core.clothes    = from->core.clothes;
    to->core.hatColor   = from->core.hatColor;
    to->core.bodyMarks  = from->core.bodyMarks;
    to->core.earShape   = from->core.earShape;
    to->core.eyebrows   = from->core.eyebrows;
    to->core.eyeShape   = from->core.eyeShape;
    to->core.hairStyle  = from->core.hairStyle;
    to->core.hat        = from->core.hat;
    to->core.mouthShape = from->core.mouthShape;
    to->core.glasses    = from->core.glasses;
    to->name            = from->name;
    setUsernameFromND(&to->name);
    generateSwadgesonaImage(to, true);
}

void generateRandomSwadgesona(swadgesona_t* sw)
{
    sw->core.skin         = esp_random() % SKIN_COLOR_COUNT;
    sw->core.hairColor    = esp_random() % HAIR_COLOR_COUNT;
    sw->core.eyeColor     = esp_random() % EYE_COLOR_COUNT;
    sw->core.clothes      = esp_random() % C_COUNT;
    sw->core.hatColor     = esp_random() % HA_COLOR_COUNT;
    sw->core.glassesColor = esp_random() % GC_COUNT;
    sw->core.bodyMarks    = esp_random() % BME_COUNT;
    sw->core.earShape     = esp_random() % EAE_COUNT;
    sw->core.eyebrows     = esp_random() % EBE_COUNT;
    sw->core.eyeShape     = esp_random() % EE_COUNT;
    sw->core.hairStyle    = esp_random() % HE_COUNT;
    sw->core.hat          = esp_random() % HAE_COUNT;
    sw->core.mouthShape   = esp_random() % ME_COUNT;
    sw->core.glasses      = esp_random() % G_COUNT;

    // Generate name
    generateRandUsername(&sw->name);
    sw->core.packedName = GET_PACKED_USERNAME(sw->name);

    generateSwadgesonaImage(sw, true);
}

// Generate Swadgesona image
void generateSwadgesonaImage(swadgesona_t* sw, bool drawBody)
{
    // Delete old images if saved
    if (sw->image.w != 0)
    {
        freeWsg(&sw->image);
    }

    // Make a new canvas
    canvasBlankInit(&sw->image, SWSN_WIDTH, SWSN_HEIGHT, cTransparent, true);

    // Body
    wsgPaletteReset(&sw->pal);
    _getPaletteFromIdx(&sw->pal, COLOR_SKIN, sw->core.skin);
    canvasDrawSimplePal(&sw->image, SWSN_HEAD_WSG, 0, 0, &sw->pal);

    if (sw->core.bodyMarks == BME_VITILIGO)
    {
        canvasDrawSimple(&sw->image, BM_VITILIGO_WSG, 0, 0);
    }

    // Ears
    // Human ears require no extra draw calls.
    if (sw->core.earShape != EAE_HUMAN)
    {
        canvasDrawSimplePal(&sw->image, earWsgs[sw->core.earShape - 1], 0, 0, &sw->pal);
    }

    // Mouth
    canvasDrawSimple(&sw->image, mouthWsgs[sw->core.mouthShape], 0, 0);

    // Eyes
    wsgPaletteReset(&sw->pal);
    _getPaletteFromIdx(&sw->pal, COLOR_EYES, sw->core.eyeColor);
    canvasDrawSimplePal(&sw->image, eyeWsgs[sw->core.eyeShape], 0, 0, &sw->pal);

    // Eyebrows
    wsgPaletteReset(&sw->pal);
    _getPaletteFromIdx(&sw->pal, COLOR_HAIR, sw->core.hairColor);
    canvasDrawSimplePal(&sw->image, eyebrowsWsgs[sw->core.eyebrows], 0, 0, &sw->pal);

    // Body marks
    if (sw->core.bodyMarks != BME_NONE && sw->core.bodyMarks != BME_VITILIGO)
    {
        canvasDrawSimplePal(&sw->image, bodymarksWsgs[sw->core.bodyMarks - 1], 0, 0, &sw->pal);
    }

    // Hair
    // Use the same palette as the eyebrows
    if (sw->core.hairStyle != HE_NONE)
    {
        canvasDrawSimplePal(&sw->image, hairWsgs[sw->core.hairStyle - 1], 0, 0, &sw->pal);
    }

    // Bunny, Cat, and Dog ears go over the hair
    if (sw->core.earShape == EAE_BUNNY || sw->core.earShape == EAE_DOG || sw->core.earShape == EAE_CAT
        || sw->core.earShape == EAE_DOWN_COW || sw->core.earShape == EAE_OPEN_COW)
    {
        canvasDrawSimplePal(&sw->image, earWsgs[sw->core.earShape - 1], 0, 0, &sw->pal);
    }

    // Draw shirt if required
    if (drawBody)
    {
        wsgPaletteReset(&sw->pal);
        _getPaletteFromIdx(&sw->pal, COLOR_SKIN, sw->core.skin);
        _getPaletteFromIdx(&sw->pal, COLOR_CLOTHES, sw->core.clothes);
        canvasDrawSimplePal(&sw->image, SWSN_BODY_WSG, 0, 0, &sw->pal);
        if (sw->core.bodyMarks == BME_CHOKER || sw->core.bodyMarks == BME_SPIKED_NECKLACE
            || sw->core.bodyMarks == BME_NECK_BLOOD)
        {
            wsgPaletteReset(&sw->pal);
            canvasDrawSimple(&sw->image, bodymarksWsgs[sw->core.bodyMarks -1], 0, 0);
        }
    }

    // If the following hairstyles are included, they need to be redrawn over the shirt
    if (sw->core.hairStyle == HE_DOLLY || sw->core.hairStyle == HE_WEDNESDAY || sw->core.hairStyle == HE_WEDNESDAY_R)
    {
        wsgPaletteReset(&sw->pal);
        _getPaletteFromIdx(&sw->pal, COLOR_HAIR, sw->core.hairColor);
        canvasDrawSimplePal(&sw->image, hairWsgs[sw->core.hairStyle - 1], 0, 0, &sw->pal);
    }
    else if (sw->core.hairStyle == HE_JINX)
    {
        wsgPaletteReset(&sw->pal);
        _getPaletteFromIdx(&sw->pal, COLOR_HAIR, sw->core.hairColor);
        canvasDrawSimplePal(&sw->image, H_JINX_HALF_WSG, 0, 0, &sw->pal);
    }

    // Glasses
    if (sw->core.glasses != G_NONE)
    {
        _getPaletteFromIdx(&sw->pal, COLOR_GLASSES, sw->core.glassesColor);
        canvasDrawSimplePal(&sw->image, glassesWsgs[sw->core.glasses - 1], 0, 0, &sw->pal);
    }

    // Hats
    if (sw->core.hat != HAE_NONE)
    {
        wsgPaletteReset(&sw->pal);
        if (sw->core.hat == HAE_PULSE) // If pulse use special color code
        {
            switch (sw->core.hatColor)
            {
                case HA_BLUE:
                case HA_DARK_BLUE:
                {
                    sw->pal.newColors[c033] = c005;
                    sw->pal.newColors[c055] = c035;
                    break;
                }
                case HA_DARK_GREEN:
                case HA_GREEN:
                {
                    sw->pal.newColors[c033] = c142;
                    sw->pal.newColors[c055] = c252;
                    break;
                }
                case HA_HOT_PINK:
                case HA_PINK:
                case HA_PURPLE:
                {
                    sw->pal.newColors[c033] = c504;
                    sw->pal.newColors[c055] = c515;
                    break;
                }
                case HA_ORANGE: // Red
                {
                    sw->pal.newColors[c033] = c500;
                    sw->pal.newColors[c055] = c532;
                    break;
                }
                case HA_YELLOW:
                {
                    sw->pal.newColors[c033] = c541;
                    sw->pal.newColors[c055] = c552;
                    break;
                }
                default:
                {
                    sw->pal.newColors[c033] = c033;
                    sw->pal.newColors[c055] = c055;
                    break;
                }
            }
        }
        else if (sw->core.hat != HAE_BIGMA && sw->core.hat != HAE_BATTRICE && sw->core.hat != HAE_MET_HELMET
                 && sw->core.hat != HAE_SAWTOOTH)
        {
            _getPaletteFromIdx(&sw->pal, COLOR_HAT, sw->core.hatColor);
        }
        // Draw hat
        canvasDrawSimplePal(&sw->image, hatWsgs[sw->core.hat - 1], 0, 0, &sw->pal);
    }
}

void loadSPSona(swadgesona_t* sw)
{
    size_t len = sizeof(swadgesonaCore_t);
    if (!readNvsBlob(spSonaNVSKey, sw, &len))
    {
        sw = NULL;
    }
}

// Get indexes
cnfsFileIdx_t getHairWSG(swadgesona_t* sw)
{
    return hairWsgs[sw->core.hairStyle];
}

//==============================================================================
// Static Functions
//==============================================================================

static void _getPaletteFromIdx(wsgPalette_t* palette, paletteSwap_t ps, int idx)
{
    switch (ps)
    {
        case COLOR_SKIN:
        {
            switch (idx)
            {
                case SKIN_ZERO:
                default:
                {
                    palette->newColors[c422] = c422; // mid color
                    palette->newColors[c544] = c544; // base color
                    break;
                }
                case SKIN_ONE:
                {
                    palette->newColors[c422] = c423; // mid color
                    palette->newColors[c544] = c545; // base color
                    break;
                }
                case SKIN_TWO:
                {
                    palette->newColors[c422] = c432; // mid color
                    palette->newColors[c544] = c543; // base color
                    break;
                }
                case SKIN_THREE:
                {
                    palette->newColors[c422] = c321; // mid color
                    palette->newColors[c544] = c432; // base color
                    break;
                }
                case SKIN_FOUR:
                {
                    palette->newColors[c422] = c211; // mid color
                    palette->newColors[c544] = c321; // base color
                    break;
                }
                case SKIN_FIVE:
                {
                    palette->newColors[c422] = c200; // mid color
                    palette->newColors[c544] = c210; // base color
                    break;
                }
                case SKIN_BLUE:
                {
                    palette->newColors[c422] = c135; // mid color
                    palette->newColors[c544] = c255; // base color
                    break;
                }
                case SKIN_GRAY:
                {
                    palette->newColors[c422] = c333; // mid color
                    palette->newColors[c544] = c444; // base color
                    break;
                }
                case SKIN_GREEN:
                {
                    palette->newColors[c422] = c133; // mid color
                    palette->newColors[c544] = c243; // base color
                    break;
                }
                case SKIN_PINK:
                {
                    palette->newColors[c422] = c525; // mid color
                    palette->newColors[c544] = c545; // base color
                    break;
                }
                case SKIN_PURPLE:
                {
                    palette->newColors[c422] = c223; // mid color
                    palette->newColors[c544] = c334; // base color
                    break;
                }
                case SKIN_RED:
                {
                    palette->newColors[c422] = c411; // mid color
                    palette->newColors[c544] = c422; // base color
                    break;
                }
            }
            break;
        }
        case COLOR_HAIR:
        {
            switch (idx)
            {
                case HAIR_GRAY:
                {
                    palette->newColors[c111] = c111;
                    palette->newColors[c222] = c222;
                    palette->newColors[c333] = c333;
                    break;
                }
                case HAIR_BLONDE:
                {
                    palette->newColors[c111] = c321;
                    palette->newColors[c222] = c432;
                    palette->newColors[c333] = c542;
                    break;
                }
                case HAIR_ORANGE:
                {
                    palette->newColors[c111] = c411;
                    palette->newColors[c222] = c421;
                    palette->newColors[c333] = c531;
                    break;
                }
                case HAIR_RED:
                {
                    palette->newColors[c111] = c300;
                    palette->newColors[c222] = c400;
                    palette->newColors[c333] = c520;
                    break;
                }
                case HAIR_DARK_RED:
                {
                    palette->newColors[c111] = c100;
                    palette->newColors[c222] = c200;
                    palette->newColors[c333] = c300;
                    break;
                }
                case HAIR_BROWN:
                {
                    palette->newColors[c111] = c000;
                    palette->newColors[c222] = c100;
                    palette->newColors[c333] = c210;
                    break;
                }
                case HAIR_BLACK:
                {
                    palette->newColors[c111] = c001;
                    palette->newColors[c222] = c012;
                    palette->newColors[c333] = c000;
                    break;
                }
                case HAIR_WHITE:
                {
                    palette->newColors[c111] = c222;
                    palette->newColors[c222] = c444;
                    palette->newColors[c333] = c555;
                    break;
                }
                case HAIR_PINK:
                {
                    palette->newColors[c111] = c302;
                    palette->newColors[c222] = c424;
                    palette->newColors[c333] = c535;
                    break;
                }
                case HAIR_HOT_PINK:
                {
                    palette->newColors[c111] = c201;
                    palette->newColors[c222] = c302;
                    palette->newColors[c333] = c413;
                    break;
                }
                case HAIR_PURPLE:
                {
                    palette->newColors[c111] = c314;
                    palette->newColors[c222] = c324;
                    palette->newColors[c333] = c435;
                    break;
                }
                case HAIR_DARK_PURPLE:
                {
                    palette->newColors[c111] = c101;
                    palette->newColors[c222] = c102;
                    palette->newColors[c333] = c203;
                    break;
                }
                case HAIR_TEAL:
                {
                    palette->newColors[c111] = c012;
                    palette->newColors[c222] = c133;
                    palette->newColors[c333] = c144;
                    break;
                }
                case HAIR_BLUE:
                {
                    palette->newColors[c111] = c001;
                    palette->newColors[c222] = c002;
                    palette->newColors[c333] = c013;
                    break;
                }
                case HAIR_GREEN:
                {
                    palette->newColors[c111] = c021;
                    palette->newColors[c222] = c032;
                    palette->newColors[c333] = c353;
                    break;
                }
                case HAIR_DARK_GREEN:
                {
                    palette->newColors[c111] = c010;
                    palette->newColors[c222] = c121;
                    palette->newColors[c333] = c131;
                    break;
                }
            }
            break;
        }
        case COLOR_EYES:
        {
            switch (idx)
            {
                case EYES_BLACK:
                {
                    palette->newColors[c130] = c444; // HIGHLIGHT
                    palette->newColors[c010] = c000; // BASE
                    break;
                }
                case EYES_BLUE:
                {
                    palette->newColors[c130] = c255; // HIGHLIGHT
                    palette->newColors[c010] = c005; // BASE
                    break;
                }
                case EYES_BROWN:
                {
                    palette->newColors[c130] = c432; // HIGHLIGHT
                    palette->newColors[c010] = c210; // BASE
                    break;
                }
                case EYES_GRAY:
                {
                    palette->newColors[c130] = c444; // HIGHLIGHT
                    palette->newColors[c010] = c222; // BASE
                    break;
                }
                default:
                case EYES_GREEN:
                {
                    palette->newColors[c130] = c130; // HIGHLIGHT
                    palette->newColors[c010] = c010; // BASE
                    break;
                }
                case EYES_PINK:
                {
                    palette->newColors[c130] = c525; // HIGHLIGHT
                    palette->newColors[c010] = c403; // BASE
                    break;
                }
                case EYES_PURPLE:
                {
                    palette->newColors[c130] = c345; // HIGHLIGHT
                    palette->newColors[c010] = c224; // BASE
                    break;
                }
                case EYES_RED:
                {
                    palette->newColors[c130] = c533; // HIGHLIGHT
                    palette->newColors[c010] = c300; // BASE
                    break;
                }
                case EYES_YELLOW:
                {
                    palette->newColors[c130] = c533; // HIGHLIGHT
                    palette->newColors[c010] = c541; // BASE
                    break;
                }
            }
            break;
        }
        case COLOR_CLOTHES:
        {
            switch (idx)
            {
                case C_BLACK:
                {
                    palette->newColors[c000] = c001;
                    palette->newColors[c301] = c111;
                    palette->newColors[c401] = c000;
                    break;
                }
                case C_BLUE:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c003;
                    palette->newColors[c401] = c004;
                    break;
                }
                case C_BROWN:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c100;
                    palette->newColors[c401] = c210;
                    break;
                }
                case C_CYAN:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c054;
                    palette->newColors[c401] = c455;
                    break;
                }
                case C_DARK_PINK:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c425;
                    palette->newColors[c401] = c435;
                    break;
                }
                case C_DARK_PURPLE:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c102;
                    palette->newColors[c401] = c203;
                    break;
                }
                case C_GRAY:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c111;
                    palette->newColors[c401] = c222;
                    break;
                }
                case C_GREEN:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c130;
                    palette->newColors[c401] = c240;
                    break;
                }
                case C_HOT_PINK:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c413;
                    palette->newColors[c401] = c505;
                    break;
                }
                case C_HOTTER_PINK:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c402;
                    palette->newColors[c401] = c503;
                    break;
                }
                case C_LIGHT_TEAL:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c122;
                    palette->newColors[c401] = c233;
                    break;
                }
                case C_LIME:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c253;
                    palette->newColors[c401] = c554;
                    break;
                }
                case C_OFF_BLACK:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c001;
                    palette->newColors[c401] = c102;
                    break;
                }
                case C_ORANGE:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c421;
                    palette->newColors[c401] = c521;
                    break;
                }
                case C_PEACH:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c533;
                    palette->newColors[c401] = c544;
                    break;
                }
                case C_PINK:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c535;
                    palette->newColors[c401] = c545;
                    break;
                }
                case C_PURPLE:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c214;
                    palette->newColors[c401] = c305;
                    break;
                }
                case C_RED:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c301;
                    palette->newColors[c401] = c401;
                    break;
                }
                case C_TAN:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c433;
                    palette->newColors[c401] = c543;
                    break;
                }
                case C_TEAL:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c022;
                    palette->newColors[c401] = c033;
                    break;
                }
                case C_WHITE:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c444;
                    palette->newColors[c401] = c555;
                    break;
                }
                case C_YELLOW:
                {
                    palette->newColors[c000] = c000;
                    palette->newColors[c301] = c431;
                    palette->newColors[c401] = c541;
                    break;
                }
            }
            break;
        }
        case COLOR_HAT:
        {
            switch (idx)
            {
                case HA_BLUE:
                {
                    palette->newColors[c523] = c345;
                    palette->newColors[c514] = c135;
                    palette->newColors[c513] = c124;
                    palette->newColors[c502] = c014;
                    break;
                }
                case HA_CYAN:
                {
                    palette->newColors[c523] = c455;
                    palette->newColors[c514] = c255;
                    palette->newColors[c513] = c144;
                    palette->newColors[c502] = c033;
                    break;
                }
                case HA_DARK_BLUE:
                {
                    palette->newColors[c523] = c134;
                    palette->newColors[c514] = c123;
                    palette->newColors[c513] = c112;
                    palette->newColors[c502] = c002;
                    break;
                }
                case HA_DARK_GREEN:
                {
                    palette->newColors[c523] = c340;
                    palette->newColors[c514] = c230;
                    palette->newColors[c513] = c120;
                    palette->newColors[c502] = c010;
                    break;
                }
                case HA_GRAY:
                {
                    palette->newColors[c523] = c444;
                    palette->newColors[c514] = c333;
                    palette->newColors[c513] = c222;
                    palette->newColors[c502] = c000;
                    break;
                }
                case HA_GREEN:
                {
                    palette->newColors[c523] = c454;
                    palette->newColors[c514] = c053;
                    palette->newColors[c513] = c142;
                    palette->newColors[c502] = c032;
                    break;
                }
                case HA_HOT_PINK:
                {
                    palette->newColors[c523] = c523;
                    palette->newColors[c514] = c514;
                    palette->newColors[c513] = c513;
                    palette->newColors[c502] = c502;
                    break;
                }
                case HA_MAUVE:
                {
                    palette->newColors[c523] = c534;
                    palette->newColors[c514] = c512;
                    palette->newColors[c513] = c412;
                    palette->newColors[c502] = c312;
                    break;
                }
                case HA_ORANGE:
                {
                    palette->newColors[c523] = c521;
                    palette->newColors[c514] = c500;
                    palette->newColors[c513] = c400;
                    palette->newColors[c502] = c300;
                    break;
                }
                case HA_PALE_BLUE:
                {
                    palette->newColors[c523] = c455;
                    palette->newColors[c514] = c345;
                    palette->newColors[c513] = c234;
                    palette->newColors[c502] = c224;
                    break;
                }
                case HA_PALE_YELLOW:
                {
                    palette->newColors[c523] = c554;
                    palette->newColors[c514] = c543;
                    palette->newColors[c513] = c432;
                    palette->newColors[c502] = c322;
                    break;
                }
                case HA_PINK:
                {
                    palette->newColors[c523] = c545;
                    palette->newColors[c514] = c534;
                    palette->newColors[c513] = c423;
                    palette->newColors[c502] = c412;
                    break;
                }
                case HA_PURPLE:
                {
                    palette->newColors[c523] = c314;
                    palette->newColors[c514] = c313;
                    palette->newColors[c513] = c212;
                    palette->newColors[c502] = c202;
                    break;
                }
                case HA_YELLOW:
                {
                    palette->newColors[c523] = c552;
                    palette->newColors[c514] = c540;
                    palette->newColors[c513] = c431;
                    palette->newColors[c502] = c320;
                    break;
                }
            }
            break;
        }
        case COLOR_GLASSES:
        {
            switch (idx)
            {
                case GC_BLACK:
                {
                    palette->newColors[c410] = c000;
                    break;
                }
                case GC_BROWN:
                {
                    palette->newColors[c410] = c211;
                    break;
                }
                case GC_RED:
                {
                    palette->newColors[c410] = c410;
                    break;
                }
            }
            break;
        }
    }
}