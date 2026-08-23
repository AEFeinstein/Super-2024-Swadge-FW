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
    LOAD_TEXTURE(ray, BG_DOOR_BUSH);
    LOAD_TEXTURE(ray, BG_DOOR_CRACK_H);
    LOAD_TEXTURE(ray, BG_DOOR_CRACK_V);
    LOAD_TEXTURE(ray, BG_DOOR_3);
    LOAD_TEXTURE(ray, BG_DOOR_4);
    LOAD_TEXTURE(ray, BG_DOOR_5);
    LOAD_TEXTURE(ray, BG_DOOR_6);
    LOAD_TEXTURE(ray, BG_DOOR_7);
    LOAD_TEXTURE(ray, BG_DOOR_8);
    LOAD_TEXTURE(ray, BG_DOOR_9);
    LOAD_TEXTURE(ray, BG_DOOR_10);
    LOAD_TEXTURE(ray, BG_DOOR_11);
    LOAD_TEXTURE(ray, BG_DOOR_12);
    LOAD_TEXTURE(ray, BG_DOOR_13);
    LOAD_TEXTURE(ray, BG_DOOR_14);
    LOAD_TEXTURE(ray, BG_DOOR_15);
    LOAD_TEXTURE(ray, BG_DOOR_16);
    LOAD_TEXTURE(ray, BG_DOOR_17);
    LOAD_TEXTURE(ray, BG_DOOR_18);
    LOAD_TEXTURE(ray, BG_DOOR_19);
    LOAD_TEXTURE(ray, BG_DOOR_20);
    LOAD_TEXTURE(ray, BG_DOOR_21);
    LOAD_TEXTURE(ray, BG_DOOR_22);
    LOAD_TEXTURE(ray, BG_DOOR_23);
    LOAD_TEXTURE(ray, BG_DOOR_24);
    LOAD_TEXTURE(ray, BG_DOOR_25);
    LOAD_TEXTURE(ray, BG_DOOR_26);
    LOAD_TEXTURE(ray, BG_DOOR_27);
    LOAD_TEXTURE(ray, BG_DOOR_28);
    LOAD_TEXTURE(ray, BG_DOOR_29);
    LOAD_TEXTURE(ray, BG_DOOR_30);
    LOAD_TEXTURE(ray, BG_DOOR_31);
    LOAD_TEXTURE(ray, BG_FLOOR_HOLE);
    LOAD_TEXTURE(ray, BG_FLOOR_1);
    LOAD_TEXTURE(ray, BG_FLOOR_2);
    LOAD_TEXTURE(ray, BG_FLOOR_3);
    LOAD_TEXTURE(ray, BG_FLOOR_4);
    LOAD_TEXTURE(ray, BG_FLOOR_5);
    LOAD_TEXTURE(ray, BG_FLOOR_6);
    LOAD_TEXTURE(ray, BG_FLOOR_7);
    LOAD_TEXTURE(ray, BG_FLOOR_8);
    LOAD_TEXTURE(ray, BG_FLOOR_9);
    LOAD_TEXTURE(ray, BG_FLOOR_10);
    LOAD_TEXTURE(ray, BG_FLOOR_11);
    LOAD_TEXTURE(ray, BG_FLOOR_12);
    LOAD_TEXTURE(ray, BG_FLOOR_13);
    LOAD_TEXTURE(ray, BG_FLOOR_14);
    LOAD_TEXTURE(ray, BG_FLOOR_15);
    LOAD_TEXTURE(ray, BG_FLOOR_16);
    LOAD_TEXTURE(ray, BG_FLOOR_17);
    LOAD_TEXTURE(ray, BG_FLOOR_18);
    LOAD_TEXTURE(ray, BG_FLOOR_19);
    LOAD_TEXTURE(ray, BG_FLOOR_20);
    LOAD_TEXTURE(ray, BG_FLOOR_21);
    LOAD_TEXTURE(ray, BG_FLOOR_22);
    LOAD_TEXTURE(ray, BG_FLOOR_23);
    LOAD_TEXTURE(ray, BG_FLOOR_24);
    LOAD_TEXTURE(ray, BG_FLOOR_25);
    LOAD_TEXTURE(ray, BG_FLOOR_26);
    LOAD_TEXTURE(ray, BG_FLOOR_27);
    LOAD_TEXTURE(ray, BG_FLOOR_28);
    LOAD_TEXTURE(ray, BG_FLOOR_29);
    LOAD_TEXTURE(ray, BG_FLOOR_30);
    LOAD_TEXTURE(ray, BG_FLOOR_31);
    LOAD_TEXTURE(ray, BG_WALL_0);
    LOAD_TEXTURE(ray, BG_WALL_1);
    LOAD_TEXTURE(ray, BG_WALL_2);
    LOAD_TEXTURE(ray, BG_WALL_3);
    LOAD_TEXTURE(ray, BG_WALL_4);
    LOAD_TEXTURE(ray, BG_WALL_5);
    LOAD_TEXTURE(ray, BG_WALL_6);
    LOAD_TEXTURE(ray, BG_WALL_7);
    LOAD_TEXTURE(ray, BG_WALL_8);
    LOAD_TEXTURE(ray, BG_WALL_9);
    LOAD_TEXTURE(ray, BG_WALL_10);
    LOAD_TEXTURE(ray, BG_WALL_11);
    LOAD_TEXTURE(ray, BG_WALL_12);
    LOAD_TEXTURE(ray, BG_WALL_13);
    LOAD_TEXTURE(ray, BG_WALL_14);
    LOAD_TEXTURE(ray, BG_WALL_15);
    LOAD_TEXTURE(ray, BG_WALL_16);
    LOAD_TEXTURE(ray, BG_WALL_17);
    LOAD_TEXTURE(ray, BG_WALL_18);
    LOAD_TEXTURE(ray, BG_WALL_19);
    LOAD_TEXTURE(ray, BG_WALL_20);
    LOAD_TEXTURE(ray, BG_WALL_21);
    LOAD_TEXTURE(ray, BG_WALL_22);
    LOAD_TEXTURE(ray, BG_WALL_23);
    LOAD_TEXTURE(ray, BG_WALL_24);
    LOAD_TEXTURE(ray, BG_WALL_25);
    LOAD_TEXTURE(ray, BG_WALL_26);
    LOAD_TEXTURE(ray, BG_WALL_27);
    LOAD_TEXTURE(ray, BG_WALL_28);
    LOAD_TEXTURE(ray, BG_WALL_29);
    LOAD_TEXTURE(ray, BG_WALL_30);
    LOAD_TEXTURE(ray, BG_WALL_31);
    LOAD_TEXTURE(ray, OBJ_ITEM_EWI);
    LOAD_TEXTURE(ray, OBJ_ITEM_BOMB);
    LOAD_TEXTURE(ray, OBJ_ITEM_BOOTS);
    LOAD_TEXTURE(ray, OBJ_ITEM_SHIELD);
    LOAD_TEXTURE(ray, OBJ_ITEM_BOW);
    LOAD_TEXTURE(ray, OBJ_ITEM_BOOMERANG);
    LOAD_TEXTURE(ray, OBJ_ITEM_TURNTABLES);
    LOAD_TEXTURE(ray, OBJ_ITEM_LULLABY);
    LOAD_TEXTURE(ray, OBJ_ITEM_HEART);
    LOAD_TEXTURE(ray, OBJ_ITEM_MPOINT_1);
    LOAD_TEXTURE(ray, OBJ_ITEM_MPOINT_5);
    LOAD_TEXTURE(ray, OBJ_ITEM_MPOINT_10);
    LOAD_TEXTURE(ray, OBJ_ITEM_MPOINT_20);
    LOAD_TEXTURE(ray, OBJ_ITEM_13);
    LOAD_TEXTURE(ray, OBJ_ITEM_14);
    LOAD_TEXTURE(ray, OBJ_ITEM_15);
    LOAD_TEXTURE(ray, OBJ_ITEM_16);
    LOAD_TEXTURE(ray, OBJ_ITEM_17);
    LOAD_TEXTURE(ray, OBJ_ITEM_18);
    LOAD_TEXTURE(ray, OBJ_ITEM_19);
    LOAD_TEXTURE(ray, OBJ_ITEM_20);
    LOAD_TEXTURE(ray, OBJ_ITEM_21);
    LOAD_TEXTURE(ray, OBJ_ITEM_22);
    LOAD_TEXTURE(ray, OBJ_ITEM_23);
    LOAD_TEXTURE(ray, OBJ_ITEM_24);
    LOAD_TEXTURE(ray, OBJ_ITEM_25);
    LOAD_TEXTURE(ray, OBJ_ITEM_26);
    LOAD_TEXTURE(ray, OBJ_ITEM_27);
    LOAD_TEXTURE(ray, OBJ_ITEM_28);
    LOAD_TEXTURE(ray, OBJ_ITEM_29);
    LOAD_TEXTURE(ray, OBJ_ITEM_30);
    LOAD_TEXTURE(ray, OBJ_ITEM_31);
    LOAD_TEXTURE(ray, OBJ_SCENERY_SHOP_BOMB);
    LOAD_TEXTURE(ray, OBJ_SCENERY_1);
    LOAD_TEXTURE(ray, OBJ_SCENERY_2);
    LOAD_TEXTURE(ray, OBJ_SCENERY_3);
    LOAD_TEXTURE(ray, OBJ_SCENERY_4);
    LOAD_TEXTURE(ray, OBJ_SCENERY_5);
    LOAD_TEXTURE(ray, OBJ_SCENERY_6);
    LOAD_TEXTURE(ray, OBJ_SCENERY_7);
    LOAD_TEXTURE(ray, OBJ_SCENERY_8);
    LOAD_TEXTURE(ray, OBJ_SCENERY_9);
    LOAD_TEXTURE(ray, OBJ_SCENERY_10);
    LOAD_TEXTURE(ray, OBJ_SCENERY_11);
    LOAD_TEXTURE(ray, OBJ_SCENERY_12);
    LOAD_TEXTURE(ray, OBJ_SCENERY_13);
    LOAD_TEXTURE(ray, OBJ_SCENERY_14);
    LOAD_TEXTURE(ray, OBJ_SCENERY_15);
    LOAD_TEXTURE(ray, OBJ_SCENERY_16);
    LOAD_TEXTURE(ray, OBJ_SCENERY_17);
    LOAD_TEXTURE(ray, OBJ_SCENERY_18);
    LOAD_TEXTURE(ray, OBJ_SCENERY_19);
    LOAD_TEXTURE(ray, OBJ_SCENERY_20);
    LOAD_TEXTURE(ray, OBJ_SCENERY_21);
    LOAD_TEXTURE(ray, OBJ_SCENERY_22);
    LOAD_TEXTURE(ray, OBJ_SCENERY_23);
    LOAD_TEXTURE(ray, OBJ_SCENERY_24);
    LOAD_TEXTURE(ray, OBJ_SCENERY_25);
    LOAD_TEXTURE(ray, OBJ_SCENERY_26);
    LOAD_TEXTURE(ray, OBJ_SCENERY_27);
    LOAD_TEXTURE(ray, OBJ_SCENERY_28);
    LOAD_TEXTURE(ray, OBJ_SCENERY_29);
    LOAD_TEXTURE(ray, OBJ_SCENERY_30);
    LOAD_TEXTURE(ray, OBJ_SCENERY_31);
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
