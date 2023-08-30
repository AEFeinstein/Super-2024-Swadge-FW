//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include <esp_log.h>

#include "macros.h"
#include "ray_tex_manager.h"

//==============================================================================
// Defines
//==============================================================================

/// The maximum number of loaded sprites. TODO pick a better number
#define MAX_LOADED_TEXTURES 64

/// Helper macro to load textures
#define LOAD_TEXTURE(r, t) loadTexture(r, #t ".wsg", t)

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Allocate memory and preload all textures
 *
 * @param ray The ::ray_t to load textures for
 */
void initLoadedTextures(ray_t* ray)
{
    // Allocate space for the textures
    ray->loadedTextures = heap_caps_calloc(MAX_LOADED_TEXTURES, sizeof(namedTexture_t), MALLOC_CAP_SPIRAM);
    // Types are 8 bit, non sequential, so allocate a 256 element array. Probably too many
    ray->typeToIdxMap = heap_caps_calloc(256, sizeof(uint8_t), MALLOC_CAP_SPIRAM);

    // Load everything by type!
    LOAD_TEXTURE(ray, BG_FLOOR);
    LOAD_TEXTURE(ray, BG_FLOOR_WATER);
    LOAD_TEXTURE(ray, BG_FLOOR_LAVA);
    LOAD_TEXTURE(ray, BG_WALL_1);
    LOAD_TEXTURE(ray, BG_WALL_2);
    LOAD_TEXTURE(ray, BG_WALL_3);
    LOAD_TEXTURE(ray, BG_DOOR);
    LOAD_TEXTURE(ray, BG_DOOR_CHARGE);
    LOAD_TEXTURE(ray, BG_DOOR_MISSILE);
    LOAD_TEXTURE(ray, BG_DOOR_ICE);
    LOAD_TEXTURE(ray, BG_DOOR_XRAY);
    LOAD_TEXTURE(ray, OBJ_ENEMY_START_POINT);
    LOAD_TEXTURE(ray, OBJ_ENEMY_BEAM);
    LOAD_TEXTURE(ray, OBJ_ENEMY_CHARGE);
    LOAD_TEXTURE(ray, OBJ_ENEMY_MISSILE);
    LOAD_TEXTURE(ray, OBJ_ENEMY_ICE);
    LOAD_TEXTURE(ray, OBJ_ENEMY_XRAY);
    LOAD_TEXTURE(ray, OBJ_ITEM_BEAM);
    LOAD_TEXTURE(ray, OBJ_ITEM_CHARGE_BEAM);
    LOAD_TEXTURE(ray, OBJ_ITEM_MISSILE);
    LOAD_TEXTURE(ray, OBJ_ITEM_ICE);
    LOAD_TEXTURE(ray, OBJ_ITEM_XRAY);
    LOAD_TEXTURE(ray, OBJ_ITEM_SUIT_WATER);
    LOAD_TEXTURE(ray, OBJ_ITEM_SUIT_LAVA);
    LOAD_TEXTURE(ray, OBJ_ITEM_ENERGY_TANK);
    LOAD_TEXTURE(ray, OBJ_ITEM_KEY);
    LOAD_TEXTURE(ray, OBJ_ITEM_ARTIFACT);
    LOAD_TEXTURE(ray, OBJ_ITEM_PICKUP_ENERGY);
    LOAD_TEXTURE(ray, OBJ_ITEM_PICKUP_MISSILE);
    LOAD_TEXTURE(ray, OBJ_BULLET_NORMAL);
    LOAD_TEXTURE(ray, OBJ_BULLET_CHARGE);
    LOAD_TEXTURE(ray, OBJ_BULLET_ICE);
    LOAD_TEXTURE(ray, OBJ_BULLET_MISSILE);
    LOAD_TEXTURE(ray, OBJ_BULLET_XRAY);
    LOAD_TEXTURE(ray, OBJ_SCENERY_TERMINAL);
}

/**
 * @brief Load a texture by name and set up a type mapping
 * This will not load a texture if it's already loaded
 *
 * @param ray The ray_t to load a texture into
 * @param wsgName The name of the texture to load
 * @param type The type for this texture
 * @return The index where the texture is stored, can be used with getTexById()
 */
uint8_t loadTexture(ray_t* ray, const char* name, rayMapCellType_t type)
{
    // Iterate over the loaded textures
    for (int16_t idx = 0; idx < MAX_LOADED_TEXTURES; idx++)
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

            // Return the index
            return idx;
        }
        else if (0 == strcmp(ray->loadedTextures[idx].name, name))
        {
            // Name matches, so return this loaded texture
            return idx;
        }
    }
    // Should be impossible to get here
    ESP_LOGE("JSON", "Couldn't load texture");
    return 0;
}

/**
 * @brief Get a texture by type
 *
 * @param ray The ray_t to get a texture from
 * @param type The type to get a texture for
 * @return The texture
 */
wsg_t* getTexByType(ray_t* ray, rayMapCellType_t type)
{
    return &ray->loadedTextures[ray->typeToIdxMap[type]].texture;
}

/**
 * @brief Get a texture by ID
 *
 * @param ray The ray_t to get a texture from
 * @param id The ID to get a texture for
 * @return The texture
 */
wsg_t* getTexById(ray_t* ray, uint8_t id)
{
    return &ray->loadedTextures[id].texture;
}

/**
 * @brief Free all textures and associated memory
 *
 * @param ray The ray_t to free textures from
 */
void freeAllTex(ray_t* ray)
{
    for (int16_t idx = 0; idx < MAX_LOADED_TEXTURES; idx++)
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
