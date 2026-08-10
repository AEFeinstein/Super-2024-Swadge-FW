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

/// Helper macro to load textures
#define LOAD_TEXTURE(r, t) loadTexture(r, t##_WSG, t)

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
    // Background tiles, floor (base only)
    loadTexture(ray, BG_BASE_FLOOR_WSG, BG_FLOOR);
    loadTexture(ray, BG_FLOOR_WATER, BG_FLOOR_WATER);
    loadTexture(ray, BG_FLOOR_LAVA, BG_FLOOR_LAVA);
    loadTexture(ray, BG_BASE_CEILING_WSG, BG_CEILING);
    loadTexture(ray, BG_FLOOR_HEAL, BG_FLOOR_HEAL);

    // Walls (base only)
    loadTexture(ray, BG_BASE_WALL_1_WSG, BG_WALL_1);
    loadTexture(ray, BG_BASE_WALL_2_WSG, BG_WALL_2);
    loadTexture(ray, BG_BASE_WALL_3_WSG, BG_WALL_3);
    loadTexture(ray, BG_BASE_WALL_4_WSG, BG_WALL_4);
    loadTexture(ray, BG_BASE_WALL_5_WSG, BG_WALL_5);

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

    LOAD_TEXTURE(ray, OBJ_ITEM_KEY_A);
    LOAD_TEXTURE(ray, OBJ_ITEM_KEY_B);
    LOAD_TEXTURE(ray, OBJ_ITEM_KEY_C);
    LOAD_TEXTURE(ray, OBJ_ITEM_ARTIFACT);
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
wsg_t* loadTexture(ray_t* ray, cnfsFileIdx_t fIdx, rayMapCellType_t type)
{
    // Iterate over the loaded textures
    node_t* node = ray->loadedTextures.first;
    while (node)
    {
        loadedTexture_t* lTex = node->val;
        // Check if the requested texture is already loaded
        if (lTex->fIdx == fIdx)
        {
            // Return if already loaded
            return &lTex->texture;
        }
        node = node->next;
    }

    // If we haven't returned a texture, load and save it
    loadedTexture_t* lTex = heap_caps_calloc(1, sizeof(loadedTexture_t), MALLOC_CAP_SPIRAM);
    lTex->fIdx            = fIdx;
    loadWsg(fIdx, &lTex->texture, true);
    push(&ray->loadedTextures, lTex);

    // If this has a type
    if (EMPTY != type)
    {
        // Set up mapping for later
        ray->typeToTexMap[type] = &lTex->texture;
    }

    // Return the texture
    return &lTex->texture;
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
    if (NULL == ray->typeToTexMap[type])
    {
        printf("TEX NOT FOUND %d\n", type);
        exit(0);
    }
    return ray->typeToTexMap[type];
}

/**
 * @brief Free all textures and associated memory
 *
 * @param ray The ray_t to free textures from
 */
void freeAllTex(ray_t* ray)
{
    while (ray->loadedTextures.length)
    {
        loadedTexture_t* lTex = pop(&ray->loadedTextures);
        freeWsg(&lTex->texture);
        heap_caps_free(lTex);
    }
}
