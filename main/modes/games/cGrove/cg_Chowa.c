/**
 * @file cg_chowa.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Pet behavior and appearance
 * @version 0.2.0
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
    "_angry-1.wsg",     "_angry-2.wsg",                   // Angry             (0-1)
    "_climb-1.wsg",     "_climb-2.wsg",                   // Climbing          (2-3)
    "_confused-1.wsg",  "_confused-2.wsg",                // Confused          (4-5)
    "_cry-1.wsg",       "_cry-2.wsg",       "_cry-3.wsg", // Crying            (6-8)
    "_dance-1.wsg",     "_dance-2.wsg",     "_dance-3.wsg",
    "_dance-4.wsg",                       // Dancing           (9-12)
    "_disgust-1.wsg",   "_disgust-2.wsg", // Disgust           (13-14)
    "_draw-1.wsg",      "_draw-2.wsg",    // Drawing           (15-16)
    "_eat-1.wsg",       "_eat-2.wsg",       "_eat-3.wsg",
    "_eat-4.wsg",                                             // Eating            (17-20)
    "_fear-1.wsg",      "_fear-2.wsg",      "_fear-3.wsg",    // Afraid            (21-23)
    "_flail-1.wsg",     "_flail-2.wsg",                       // Flailing          (24-25)
    "_gift-1.wsg",                                            // Gift/Cheer        (26)
    "_giveup-1.wsg",    "_giveup-2.wsg",    "_giveup-3.wsg",  // Depressed/Give up (27-29)
    "_happy-1.wsg",     "_happy-2.wsg",                       // Happy             (30-31)
    "_hdbtt-lft-1.wsg", "_hdbtt-lft-2.wsg",                   // Headbutt Left     (32-33)
    "_hdbtt-rgt-1.wsg", "_hdbtt-rgt-2.wsg",                   // Headbutt Right    (34-35)
    "_hit-lft-1.wsg",   "_hit-lft-2.wsg",   "_hit-lft-3.wsg", // Was hit Left      (36-38)
    "_hit-rgt-1.wsg",   "_hit-rgt-2.wsg",   "_hit-rgt-3.wsg", // Was hit Right     (39-41)
    "_jump-1.wsg",      "_jump-2.wsg",                        // Jumping           (42-43)
    "_kick-lft-1.wsg",  "_kick-lft-2.wsg",                    // Kick left         (44-45)
    "_kick-rgt-1.wsg",  "_kick-rgt-2.wsg",                    // Kick right        (46-47)
    "_pet-1.wsg",                                             // Being pet         (48)
    "_punch-lft-1.wsg", "_punch-lft-2.wsg",                   // Punching left     (49-50)
    "_punch-rgt-1.wsg", "_punch-rgt-2.wsg",                   // Punching right    (51-52)
    "_read-1.wsg",      "_read-2.wsg",      "_read-3.wsg",    // Reading           (53-55)
    "_sad-1.wsg",       "_sad-2.wsg",                         // Sad               (56-57)
    "_shocked-1.wsg",   "_shocked-2.wsg",                     // Shocked           (58-59)
    "_sing-1.wsg",      "_sing-2.wsg",      "_sing-3.wsg",
    "_sing-4.wsg", // Sing              (60-63)
    "_sit-1.wsg",  // Sit               (64)
    "_swim-lft-1.wsg",  "_swim-lft-2.wsg",  "_swim-lft-3.wsg",
    "_swim-lft-4.wsg", // Swim              (65-68)
    "_swim-rgt-1.wsg",  "_swim-rgt-2.wsg",  "_swim-rgt-3.wsg",
    "_swim-rgt-4.wsg",                                          // Swim              (69-72)
    "_sword-1.wsg",     "_sword-2.wsg",     "_sword-3.wsg",     // Swords            (73-75)
    "_throw-lft-1.wsg", "_throw-lft-2.wsg", "_throw-lft-3.wsg", // Throw item        (76-78)
    "_throw-rgt-1.wsg", "_throw-rgt-2.wsg", "_throw-rgt-3.wsg", // Throw item        (79-81)
    "_trip-bk-1.wsg",   "_trip-bk-2.wsg",   "_trip-bk-3.wsg",
    "_trip-bk-3.wsg", // Tripped backwards (82-85)
    "_trip-lft-1.wsg",  "_trip-lft-2.wsg",  "_trip-lft-3.wsg",
    "_trip-lft-3.wsg", // Tripped Left      (86-89)
    "_trip-rgt-1.wsg",  "_trip-rgt-2.wsg",  "_trip-rgt-3.wsg",
    "_trip-rgt-3.wsg", // Tripped Right     (90-93)
    "_walkfwd-1.wsg",   "_walkfwd-2.wsg",   "_walkfwd-3.wsg",
    "_walkfwd-4.wsg", // Walking toward    (94-97)
    "_walkbk-1.wsg",    "_walkbk-2.wsg",    "_walkbk-3.wsg",
    "_walkbk-4.wsg", // Walking away      (98-101)
    "_walklft-1.wsg",   "_walklft-2.wsg",   "_walklft-3.wsg",
    "_walklft-4.wsg", // Walking left      (102-105)
    "_walkrgt-1.wsg",   "_walkrgt-2.wsg",   "_walkrgt-3.wsg",
    "_walkrgt-4.wsg", // Walking right     (106-109)
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
    cg_initByPrefix(cg, CG_KING_DONUT, CG_ADULT, "akd");
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
        case CG_ANIM_CONFUSED:
        {
            return &spr[4 + idx];
        }
        case CG_ANIM_CRY:
        {
            return &spr[6 + idx];
        }
        case CG_ANIM_DISGUST:
        {
            return &spr[13 + idx];
        }
        case CG_ANIM_FEAR:
        {
            return &spr[21 + idx];
        }
        case CG_ANIM_FLAIL:
        {
            return &spr[24 + idx];
        }
        case CG_ANIM_GIVE_UP:
        {
            return &spr[27 + idx];
        }
        case CG_ANIM_HAPPY:
        {
            return &spr[30 + idx];
        }
        case CG_ANIM_SAD:
        {
            return &spr[56 + idx];
        }
        case CG_ANIM_WALK_DOWN:
        {
            return &spr[94 + idx];
        }
        case CG_ANIM_WALK_UP:
        {
            return &spr[98 + idx];
        }
        case CG_ANIM_WALK_RIGHT:
        {
            return &spr[106 + idx];
        }
        case CG_ANIM_WALK_LEFT:
        {
            return &spr[102 + idx];
        }
        case CG_ANIM_SWIM_RIGHT:
        {
            return &spr[69 + idx];
        }
        case CG_ANIM_SWIM_LEFT:
        {
            return &spr[65 + idx];
        }
        case CG_ANIM_CLIMB:
        {
            return &spr[2 + idx];
        }
        case CG_ANIM_JUMP:
        {
            return &spr[42 + idx];
        }
        case CG_ANIM_TRIP_UP:
        {
            return &spr[82 + idx];
        }
        case CG_ANIM_TRIP_RIGHT:
        {
            return &spr[90 + idx];
        }
        case CG_ANIM_TRIP_LEFT:
        {
            return &spr[86 + idx];
        }
        case CG_ANIM_HEADBUTT_RIGHT:
        {
            return &spr[34 + idx];
        }
        case CG_ANIM_HEADBUTT_LEFT:
        {
            return &spr[32 + idx];
        }
        case CG_ANIM_KICK_RIGHT:
        {
            return &spr[46 + idx];
        }
        case CG_ANIM_KICK_LEFT:
        {
            return &spr[44 + idx];
        }
        case CG_ANIM_PUNCH_RIGHT:
        {
            return &spr[51 + idx];
        }
        case CG_ANIM_PUNCH_LEFT:
        {
            return &spr[49 + idx];
        }
        case CG_ANIM_HIT_RIGHT:
        {
            return &spr[39 + idx];
        }
        case CG_ANIM_HIT_LEFT:
        {
            return &spr[36 + idx];
        }
        case CG_ANIM_DRAW:
        {
            return &spr[15 + idx];
        }
        case CG_ANIM_EAT:
        {
            return &spr[17 + idx];
        }
        case CG_ANIM_GIFT:
        {
            return &spr[26];
        }
        case CG_ANIM_READ:
        {
            return &spr[53 + idx];
        }
        case CG_ANIM_SWORD:
        {
            return &spr[73 + idx];
        }
        case CG_ANIM_THROW_RIGHT:
        {
            return &spr[79 + idx];
        }
        case CG_ANIM_THROW_LEFT:
        {
            return &spr[76 + idx];
        }
        case CG_ANIM_DANCE:
        {
            return &spr[9 + idx];
        }
        case CG_ANIM_PET:
        {
            return &spr[48];
        }
        case CG_ANIM_SING:
        {
            return &spr[60 + idx];
        }
        case CG_ANIM_SIT:
        {
            return &spr[64];
        }
        default:
        {
            return &spr[94]; // Walk down frame 1
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