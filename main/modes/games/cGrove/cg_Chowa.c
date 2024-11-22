/**
 * @file cg_chowa.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Pet behavior and appearance
 * @version 0.1
 * @date 2024-09-08
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_Chowa.h"

//==============================================================================
// Consts
//==============================================================================

static const char* imageStrings[] = {
    "_angry-1.wsg",     "_angry-2.wsg",                                         // Angry                (0-1)
    "_bkfall1.wsg",     "_bkfall2.wsg",     "_bkfall3.wsg",     "_bkfall4.wsg", // Falling up           (2-5)
    "_bkwalk1.wsg",     "_bkwalk2.wsg",     "_bkwalk3.wsg",     "_bkwalk4.wsg", // Walking up           (6-9)
    "_climb1.wsg",      "_climb2.wsg",                                          // Climbing             (10-11)
    "_dance-1.wsg",     "_dance-2.wsg",     "_dance-3.wsg",     "_dance-4.wsg", // Dancing              (12-15)
    "_disgust1.wsg",    "_disgust2.wsg",                                        // Disgust              (16-17)
    "_draw-1.wsg",      "_draw-2.wsg",                                          // Drawing              (18-19)
    "_eat-1.wsg",       "_eat-2.wsg",       "_eat-3.wsg",       "_eat-4.wsg",   // Eating               (20-23)
    "_fallingbk-1.wsg", "_fallingbk-2.wsg",                                     // Falling facing down  (24-25)
    "_fear-1.wsg",      "_fear-2.wsg",                                          // Afraid               (26-27)
    "_flail-1.wsg",     "_flail-2.wsg",                                         // Flailing             (28-29)
    "_gift-1.wsg",                                                              // Gift/Cheer           (30)
    "_givup-1.wsg",     "_givup-2.wsg",                                         // Depressed/Give up    (31-32)
    "_happy1.wsg",      "_happy2.wsg",                                          // Happy                (33-34)
    "_hdbtt-1.wsg",     "_hdbtt-2.wsg",                                         // Headbutt             (35-36)
    "_kick-1.wsg",      "_kick-2.wsg",                                          // Kick                 (37-38)
    "_pet-1.wsg",                                                               // Being pet            (39)
    "_punch-1.wsg",     "_punch-2.wsg",                                         // Punching             (40-41)
    "_read-1.wsg",      "_read-2.wsg",      "_read-3.wsg",                      // Reading              (42-44)
    "_sad-1.wsg",       "_sad-2.wsg",                                           // Sad                  (45-46)
    "_sdfall1.wsg",     "_sdfall2.wsg",     "_sdfall3.wsg",                     // Side Fall            (47-49)
    "_sdwalk1.wsg",     "_sdwalk2.wsg",     "_sdwalk3.wsg",     "_sdwalk4.wsg", // Side walk            (50-53)
    "_sing-1.wsg",      "_sing-2.wsg",      "_sing-3.wsg",      "_sing-4.wsg",  // Sing                 (54-57)
    "_sit1.wsg",                                                                // Sit                  (58)
    "_swim-1.wsg",      "_swim-2.wsg",      "_swim-3.wsg",      "_swim-4.wsg",  // Swim                 (59-62)
    "_swordplay-1.wsg", "_swordplay-2.wsg", "_swordplay-3.wsg",                 // Swords               (63-65)
    "_throw-1.wsg",     "_throw-2.wsg",     "_throw-3.wsg",                     // Throw item           (66-68)
    "_walk1.wsg",       "_walk2.wsg",       "_walk3.wsg",       "_walk4.wsg",   // Walk down            (69-72)
};

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Uses a prefix to load assets
 *
 * @param cg Game data
 * @param type The type of Chowa
 * @param adult If it's an adult or not
 * @param prefix The string to append to load the file
 */
static void cg_initByPrefix(cGrove_t* cg, cgColorType_t type, int8_t adult, char* prefix);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize all the Chowa sprites
 *
 * @param cg Game Data
 */
void cg_initChowaWSGs(cGrove_t* cg)
{
    for (int32_t idx = 0; idx < CG_NUM_TYPES; idx++)
    {
        for (int32_t idx2 = 0; idx2 < 2; idx2++)
        {
            cg->chowaWSGs[idx][idx2] = calloc(ARRAY_SIZE(imageStrings), sizeof(wsg_t));
        }
    }
    cg_initByPrefix(cg, CG_KING_DONUT, CG_CHILD, "ckd");
}

/**
 * @brief De-Init all the CHowa Sprites
 *
 * @param cg Game Data
 */
void cg_deInitChowaWSGs(cGrove_t* cg)
{
    for (int32_t idx = 0; idx < CG_NUM_TYPES; idx++)
    {
        for (int32_t idx2 = 0; idx2 < 2; idx2++)
        {
            for (int idx3 = 0; idx3 < ARRAY_SIZE(imageStrings); idx3++)
            {
                freeWsg(&cg->chowaWSGs[idx][idx2][idx3]);
            }
            free(cg->chowaWSGs[idx][idx2]);
        }
    }
}

/**
 * @brief Gets the sprite of a chowa for the animation provided. From there, is can be used in any normal WSG function
 *
 * @param cg Game Data
 * @param c Chowa to get animation for
 * @param anim Animation to grab
 * @param idx Index of the animation
 * @return wsg_t* Pointer to the expected WSG
 */
wsg_t* cg_getChowaWSG(cGrove_t* cg, cgChowa_t* c, cgChowaAnimIdx_t anim, int8_t idx)
{
    // Get type and age
    cgChowaAnimAge_t age = CG_CHILD;
    if (c->age > CG_ADULT_AGE)
    {
        age = CG_ADULT;
    }
    wsg_t* spr = cg->chowaWSGs[c->type][age];

    // Get spr from animation
    switch (anim)
    {
        case CG_ANIM_ANGRY:
        {
            return &spr[0 + idx];
        }
        case CG_ANIM_DISGUST:
        {
            return &spr[16 + idx];
        }
        case CG_ANIM_FEAR:
        {
            return &spr[26 + idx];
        }
        case CG_ANIM_FLAIL:
        {
            return &spr[28 + idx];
        }
        case CG_ANIM_GIVE_UP:
        {
            return &spr[31 + idx];
        }
        case CG_ANIM_HAPPY:
        {
            return &spr[33 + idx];
        }
        case CG_ANIM_SAD:
        {
            return &spr[45 + idx];
        }
        case CG_ANIM_WALK_DOWN:
        {
            return &spr[69 + idx];
        }
        case CG_ANIM_WALK_SIDE:
        {
            return &spr[50 + idx];
        }
        case CG_ANIM_WALK_UP:
        {
            return &spr[6 + idx];
        }
        case CG_ANIM_SWIM:
        {
            return &spr[59 + idx];
        }
        case CG_ANIM_CLIMB:
        {
            return &spr[10 + idx];
        }
        case CG_ANIM_FALL_SIDE:
        {
            return &spr[47 + idx];
        }
        case CG_ANIM_FALL_UP:
        {
            return &spr[2 + idx];
        }
        case CG_ANIM_FALL_DOWN:
        {
            return &spr[24 + idx];
        }
        case CG_ANIM_HEADBUTT:
        {
            return &spr[35 + idx];
        }
        case CG_ANIM_KICK:
        {
            return &spr[37 + idx];
        }
        case CG_ANIM_PUNCH:
        {
            return &spr[40 + idx];
        }
        case CG_ANIM_DRAW:
        {
            return &spr[18 + idx];
        }
        case CG_ANIM_EAT:
        {
            return &spr[20 + idx];
        }
        case CG_ANIM_GIFT:
        {
            return &spr[30];
        }
        case CG_ANIM_READ:
        {
            return &spr[42 + idx];
        }
        case CG_ANIM_SWORD:
        {
            return &spr[63 + idx];
        }
        case CG_ANIM_THROW:
        {
            return &spr[66 + idx];
        }
        case CG_ANIM_DANCE:
        {
            return &spr[12 + idx];
        }
        case CG_ANIM_PET:
        {
            return &spr[39];
        }
        case CG_ANIM_SING:
        {
            return &spr[54 + idx];
        }
        case CG_ANIM_SIT:
        {
            return &spr[58];
        }
        default:
        {
            return &spr[69]; // Walk down frame 1
        }
    }
}

//==============================================================================
// Static Functions
//==============================================================================

static void cg_initByPrefix(cGrove_t* cg, cgColorType_t type, int8_t adult, char* prefix)
{
    char buffer[64];
    for (int idx = 0; idx < ARRAY_SIZE(imageStrings); idx++)
    {
        strcpy(buffer, prefix);
        strcat(buffer, imageStrings[idx]);
        loadWsg(buffer, &cg->chowaWSGs[type][adult][idx], true);
    }
}