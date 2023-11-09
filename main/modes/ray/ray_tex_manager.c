//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include <esp_log.h>

#include "macros.h"
#include "ray_tex_manager.h"
#include "ray_object.h"

//==============================================================================
// Defines
//==============================================================================

/**
 * The maximum number of loaded named textures
 */
#define MAX_LOADED_TEXTURES 64

/// Helper macro to load textures
#define LOAD_TEXTURE(r, t) loadTexture(r, #t ".wsg", t)

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Allocate memory and preload all environment textures
 *
 * @param ray The ray_t to load textures for
 */
void loadEnvTextures(ray_t* ray)
{
    // Load a portrait for dialogs
    loadWsg("CHO_PORTRAIT.wsg", &ray->portrait, true);

    // Load HUD textures
    loadWsg("GUN_NORMAL.wsg", &ray->guns[LO_NORMAL], true);
    loadWsg("GUN_MISSILE.wsg", &ray->guns[LO_MISSILE], true);
    loadWsg("GUN_ICE.wsg", &ray->guns[LO_ICE], true);
    loadWsg("GUN_XRAY.wsg", &ray->guns[LO_XRAY], true);
    loadWsg("HUD_MISSILE.wsg", &ray->missileHUDicon, true);

    // Allocate space for the textures
    ray->loadedTextures = heap_caps_calloc(MAX_LOADED_TEXTURES, sizeof(namedTexture_t), MALLOC_CAP_SPIRAM);
    // Types are 8 bit, non sequential, so allocate a 256 element array. Probably too many
    ray->typeToIdxMap = heap_caps_calloc(256, sizeof(uint8_t), MALLOC_CAP_SPIRAM);

    // String buffer to load filenames
    char fName[32];

    // MUST be in the same order as rayEnv_t
    const char* const env_types[] = {"BASE", "JUNGLE", "CAVE"};
    // MUST be in the same order as rayEnvTex_t
    const char* const env_texes[] = {"WALL_1", "WALL_2", "WALL_3", "WALL_4", "WALL_5", "FLOOR", "CEILING"};
    // Load all environment textures
    for (rayEnv_t e = 0; e < NUM_ENVS; e++)
    {
        for (rayEnvTex_t t = 0; t < NUM_ENV_TEXES; t++)
        {
            snprintf(fName, sizeof(fName) - 1, "BG_%s_%s.wsg", env_types[e], env_texes[t]);
            loadWsg(fName, &ray->envTex[e][t], true);
        }
    }

    // MUST be in teh same order as rayMapCellType_t
    const char* const enemyTypes[] = {"NORMAL", "STRONG", "ARMORED", "FLAMING", "HIDDEN", "BOSS"};
    // MUST be int the same order as rayEnemyState_t
    const char* const enemyAnimations[] = {"WALK_0", "WALK_1", "SHOOT", "HURT", "BLOCK", "DEAD"};
    // Load all enemy textures
    for (int32_t eIdx = 0; eIdx < NUM_ENEMIES; eIdx++)
    {
        for (rayEnemyState_t aIdx = 0; aIdx < E_NUM_STATES; aIdx++)
        {
            for (int32_t fIdx = 0; fIdx < NUM_ANIM_FRAMES; fIdx++)
            {
                snprintf(fName, sizeof(fName) - 1, "E_%s_%s_%" PRId32 ".wsg", enemyTypes[eIdx], enemyAnimations[aIdx],
                         fIdx);
                loadWsg(fName, &ray->enemyTex[eIdx][aIdx][fIdx], true);

                // Also load alt-textures for the hidden enemy
                if (eIdx == (OBJ_ENEMY_HIDDEN - OBJ_ENEMY_NORMAL))
                {
                    snprintf(fName, sizeof(fName) - 1, "E_%s_%s_%" PRId32 "_X.wsg", enemyTypes[eIdx],
                             enemyAnimations[aIdx], fIdx);
                    loadWsg(fName, &ray->hiddenXRTex[aIdx][fIdx], true);
                }
                // Also load alt-textures for the boss
                else if (eIdx == (OBJ_ENEMY_BOSS - OBJ_ENEMY_NORMAL))
                {
                    snprintf(fName, sizeof(fName) - 1, "E_%s_%s_%" PRId32 "_M.wsg", enemyTypes[eIdx],
                             enemyAnimations[aIdx], fIdx);
                    loadWsg(fName, &ray->bossTex[B_MISSILE][aIdx][fIdx], true);

                    snprintf(fName, sizeof(fName) - 1, "E_%s_%s_%" PRId32 "_I.wsg", enemyTypes[eIdx],
                             enemyAnimations[aIdx], fIdx);
                    loadWsg(fName, &ray->bossTex[B_ICE][aIdx][fIdx], true);

                    snprintf(fName, sizeof(fName) - 1, "E_%s_%s_%" PRId32 "_X.wsg", enemyTypes[eIdx],
                             enemyAnimations[aIdx], fIdx);
                    loadWsg(fName, &ray->bossTex[B_XRAY][aIdx][fIdx], true);
                }
            }
        }
    }

    // Special floor tiles
    LOAD_TEXTURE(ray, BG_FLOOR_WATER);
    LOAD_TEXTURE(ray, BG_FLOOR_LAVA);

    // Doors
    LOAD_TEXTURE(ray, BG_DOOR);
    LOAD_TEXTURE(ray, BG_DOOR_CHARGE);
    LOAD_TEXTURE(ray, BG_DOOR_MISSILE);
    LOAD_TEXTURE(ray, BG_DOOR_ICE);
    LOAD_TEXTURE(ray, BG_DOOR_XRAY);
    LOAD_TEXTURE(ray, BG_DOOR_SCRIPT);
    LOAD_TEXTURE(ray, BG_DOOR_KEY_A);
    LOAD_TEXTURE(ray, BG_DOOR_KEY_B);
    LOAD_TEXTURE(ray, BG_DOOR_KEY_C);
    LOAD_TEXTURE(ray, BG_DOOR_ARTIFACT);

    // Items
    LOAD_TEXTURE(ray, OBJ_ITEM_BEAM);
    LOAD_TEXTURE(ray, OBJ_ITEM_CHARGE_BEAM);
    LOAD_TEXTURE(ray, OBJ_ITEM_MISSILE);
    LOAD_TEXTURE(ray, OBJ_ITEM_ICE);
    LOAD_TEXTURE(ray, OBJ_ITEM_XRAY);
    LOAD_TEXTURE(ray, OBJ_ITEM_SUIT_WATER);
    LOAD_TEXTURE(ray, OBJ_ITEM_SUIT_LAVA);
    LOAD_TEXTURE(ray, OBJ_ITEM_ENERGY_TANK);
    LOAD_TEXTURE(ray, OBJ_ITEM_KEY_A);
    LOAD_TEXTURE(ray, OBJ_ITEM_KEY_B);
    LOAD_TEXTURE(ray, OBJ_ITEM_KEY_C);
    LOAD_TEXTURE(ray, OBJ_ITEM_ARTIFACT);
    LOAD_TEXTURE(ray, OBJ_ITEM_PICKUP_ENERGY);
    LOAD_TEXTURE(ray, OBJ_ITEM_PICKUP_MISSILE);

    // Bullets
    LOAD_TEXTURE(ray, OBJ_BULLET_NORMAL);
    LOAD_TEXTURE(ray, OBJ_BULLET_CHARGE);
    LOAD_TEXTURE(ray, OBJ_BULLET_ICE);
    LOAD_TEXTURE(ray, OBJ_BULLET_MISSILE);
    LOAD_TEXTURE(ray, OBJ_BULLET_XRAY);
    LOAD_TEXTURE(ray, OBJ_BULLET_E_NORMAL);
    LOAD_TEXTURE(ray, OBJ_BULLET_E_STRONG);
    LOAD_TEXTURE(ray, OBJ_BULLET_E_ARMOR);
    LOAD_TEXTURE(ray, OBJ_BULLET_E_FLAMING);
    LOAD_TEXTURE(ray, OBJ_BULLET_E_HIDDEN);

    // Scenery
    LOAD_TEXTURE(ray, OBJ_SCENERY_TERMINAL);
    LOAD_TEXTURE(ray, OBJ_SCENERY_PORTAL);

    // Friends
    LOAD_TEXTURE(ray, OBJ_SCENERY_F1);
    LOAD_TEXTURE(ray, OBJ_SCENERY_F2);
    LOAD_TEXTURE(ray, OBJ_SCENERY_F3);
    LOAD_TEXTURE(ray, OBJ_SCENERY_F4);
    LOAD_TEXTURE(ray, OBJ_SCENERY_F5);
    LOAD_TEXTURE(ray, OBJ_SCENERY_F6);
    LOAD_TEXTURE(ray, OBJ_SCENERY_F7);
}

/**
 * @brief Load a texture by name and set up a type mapping
 * This will not load a texture if it's already loaded
 *
 * @param ray The ray_t to load a texture into
 * @param wsgName The name of the texture to load
 * @param type The type for this texture
 * @return The A pointer to the loaded texture
 */
wsg_t* loadTexture(ray_t* ray, const char* name, rayMapCellType_t type)
{
    // Iterate over the loaded textures
    for (int32_t idx = 0; idx < MAX_LOADED_TEXTURES; idx++)
    {
        // Check if the name is NULL
        if (NULL == ray->loadedTextures[idx].name)
        {
            // If so, we've reached the end and should load this texture
            if (loadWsg(name, &ray->loadedTextures[idx].texture, true))
            {
                // If the texture loads, save the name
                ray->loadedTextures[idx].name = calloc(1, strlen(name) + 1);
                memcpy(ray->loadedTextures[idx].name, name, strlen(name) + 1);
            }

            // If this has a type
            if (EMPTY != type)
            {
                // Set up mapping for later
                ray->typeToIdxMap[type] = idx;
            }

            // Return the pointer
            return &ray->loadedTextures[idx].texture;
        }
        else if (0 == strcmp(ray->loadedTextures[idx].name, name))
        {
            // Name matches, so return this loaded texture
            return &ray->loadedTextures[idx].texture;
        }
    }
    // Should be impossible to get here
    ESP_LOGE("JSON", "Couldn't load texture");
    return NULL;
}

/**
 * @brief Get a texture by type
 *
 * @param ray The ray_t to get a texture from
 * @param type The type to get a texture for
 * @return A pointer to the texture
 */
wsg_t* getTexByType(ray_t* ray, rayMapCellType_t type)
{
    return &ray->loadedTextures[ray->typeToIdxMap[type]].texture;
}

/**
 * @brief Free all textures and associated memory
 *
 * @param ray The ray_t to free textures from
 */
void freeAllTex(ray_t* ray)
{
    freeWsg(&ray->portrait);
    freeWsg(&ray->guns[LO_NORMAL]);
    freeWsg(&ray->guns[LO_MISSILE]);
    freeWsg(&ray->guns[LO_ICE]);
    freeWsg(&ray->guns[LO_XRAY]);
    freeWsg(&ray->missileHUDicon);

    // Free all environment textures
    for (rayEnv_t e = 0; e < NUM_ENVS; e++)
    {
        for (rayEnvTex_t t = 0; t < NUM_ENV_TEXES; t++)
        {
            freeWsg(&ray->envTex[e][t]);
        }
    }

    // Free all enemy textures
    for (int32_t eIdx = 0; eIdx < NUM_ENEMIES; eIdx++)
    {
        for (int32_t aIdx = 0; aIdx < E_NUM_STATES; aIdx++)
        {
            for (int32_t fIdx = 0; fIdx < NUM_ANIM_FRAMES; fIdx++)
            {
                freeWsg(&ray->enemyTex[eIdx][aIdx][fIdx]);

                // Also free alt-textures for the hidden enemy
                if (eIdx == (OBJ_ENEMY_HIDDEN - OBJ_ENEMY_NORMAL))
                {
                    freeWsg(&ray->hiddenXRTex[aIdx][fIdx]);
                }
                else if (eIdx == (OBJ_ENEMY_BOSS - OBJ_ENEMY_NORMAL))
                {
                    freeWsg(&ray->bossTex[B_MISSILE][aIdx][fIdx]);
                    freeWsg(&ray->bossTex[B_ICE][aIdx][fIdx]);
                    freeWsg(&ray->bossTex[B_XRAY][aIdx][fIdx]);
                }
            }
        }
    }

    if (ray->loadedTextures)
    {
        for (int32_t idx = 0; idx < MAX_LOADED_TEXTURES; idx++)
        {
            // Check if the name is NULL
            if (NULL != ray->loadedTextures[idx].name)
            {
                free(ray->loadedTextures[idx].name);
                freeWsg(&ray->loadedTextures[idx].texture);
            }
        }
        free(ray->loadedTextures);
        free(ray->typeToIdxMap);
    }
}
