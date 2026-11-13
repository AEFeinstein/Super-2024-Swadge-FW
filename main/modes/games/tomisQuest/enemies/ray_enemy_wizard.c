//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_wizard.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TQ_EN_WIZARD_ST_WARPING,
    TQ_EN_WIZARD_ST_APPEARING,
    TQ_EN_WIZARD_ST_IDLE,
    TQ_EN_WIZARD_ST_BUMPED
} wizardStateIndex_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wizardStateIndex_t state;
    int32_t stateTimer; // In microseconds
    int8_t stateCounter;
    vec_q24_8 vel;     ///< The direction the wizard got bumped
    vec_q24_8 tempPos; ///< Used to put wizard off screen to make it "invisible"
    bool visible;
} wizardState_t;

//==============================================================================
// Function Declarations
//==============================================================================

static bool rayEnemyWizardMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
static void rayEnemyWizardCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX,
                                               q24_8* deltaY);
static void rayEnemyWizardGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a wizard
 *
 * @param ray The whole game state
 * @param e The wizard to initialize
 */
void rayInitEnemyWizard(ray_t* ray, rayEnemy_t* e)
{
    // Wizard has one texture, for now
    e->c.sprite = loadTexture(ray, OBJ_ENEMY_GRUNT_WSG, OBJ_ENEMY_WIZARD);
    e->portrait = e->c.sprite;

    // Load bullet textures
    loadTexture(ray, OBJ_BULLET_SHIELD_0_WSG, OBJ_BULLET_SHIELD_0);
    loadTexture(ray, OBJ_BULLET_SHIELD_1_WSG, OBJ_BULLET_SHIELD_1);
    loadTexture(ray, OBJ_BULLET_SHIELD_2_WSG, OBJ_BULLET_SHIELD_2);
    loadTexture(ray, OBJ_BULLET_SHIELD_3_WSG, OBJ_BULLET_SHIELD_3);

    // Wizard has 2hp to not die
    e->health = 2;

    // Allocate state for this enemy
    e->state = heap_caps_calloc(1, sizeof(wizardState_t), MALLOC_CAP_8BIT);

    ((wizardState_t*)e->state)->state      = TQ_EN_WIZARD_ST_WARPING;
    ((wizardState_t*)e->state)->stateTimer = -1;
    ((wizardState_t*)e->state)->visible    = true;

    // Set function pointers
    e->mainFn           = rayEnemyWizardMain;
    e->collisionFn      = rayEnemyWizardCheckPlayerCollision;
    e->getShotFn        = rayEnemyWizardGetShot;
    e->dropAfterDeathFn = rayEnemyStandardItemDrop;
}

/**
 * @brief Main loop for the Wizard enemy. Handles movement logic.
 *
 * @param ray The whole game state
 * @param enemy The Wizard enemy
 * @param elapsedUs The time since this function was last called
 * @return true if this enemy is dead (after any death animations), false if it is alive
 */
bool rayEnemyWizardMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Return if dead for cleanup
    if (enemy->health <= 0)
    {
        return true;
    }

    // Casted pointer for convenience
    wizardState_t* state = enemy->state;

    // Get a bounding box for where the enemy is
    rectangle_t bb = rayGetObjBB(&enemy->c);

    switch (state->state)
    {
        case TQ_EN_WIZARD_ST_WARPING:
        {
            state->vel.x = 0;
            state->vel.y = 0;

            // warp
            vec_t warpTile;
            do
            {
                warpTile.x = (ray->camera.x / CELL_SIZE) + (esp_random() % (TFT_WIDTH / CELL_SIZE));
                warpTile.y = (ray->camera.y / CELL_SIZE) + (esp_random() % (TFT_HEIGHT / CELL_SIZE));
            } while (isPassableCell(&ray->map.tiles[warpTile.x][warpTile.y]));
            enemy->c.posX = TO_FX(warpTile.x) + TO_FX_FRAC(1, 2);
            enemy->c.posY = TO_FX(warpTile.y) + TO_FX_FRAC(1, 2);
            // update bb after warping
            bb = rayGetObjBB(&enemy->c);

            // big box to make sure player is not too close
            rectangle_t proximityBox = {
                .pos = {
                    .x = enemy->c.posX - TO_FX(2),
                    .y = enemy->c.posY - TO_FX(2),
                },
                .width  = TO_FX(4),
                .height = TO_FX(4),
            };

            if (rayBoundingBoxFitsInMap(ray, bb, NULL) &&
                !rectRectIntersection(rayGetPlayerBB(ray), proximityBox, NULL))
            {
                state->state      = TQ_EN_WIZARD_ST_APPEARING;
                state->tempPos.x  = enemy->c.posX;
                state->tempPos.y  = enemy->c.posY;
                state->stateTimer = 3000000;
            }

            break;
        }
        case TQ_EN_WIZARD_ST_APPEARING:
        {
            state->visible = !state->visible;
            enemy->c.posX  = ((state->stateTimer >> 4) & 0b1) ? state->tempPos.x : -TO_FX(1);
            enemy->c.posY  = ((state->stateTimer >> 4) & 0b1) ? state->tempPos.y : -TO_FX(1);

            state->stateTimer -= elapsedUs;

            if (state->stateTimer < 0)
            {
                enemy->c.posX = state->tempPos.x;
                enemy->c.posY = state->tempPos.y;

                vec_q24_8 shotVec;

                bool xAxisMatch = FROM_FX(ray->p.posX) == FROM_FX(enemy->c.posX);
                bool yAxisMatch = FROM_FX(ray->p.posY) == FROM_FX(enemy->c.posY);
                bool randomAxis = esp_random() & 0b1;

                /*
                    Face directly towards the player if player is axis aligned
                    Otherwise face towards the player on either axis.

                    xAxisMatch | 0 | 1 | 0 | 1 |
                    yAxisMatch | 0 | 0 | 1 | 1 |
                    randomAxis | * | * | * | * |
                    ----------------------------
                    axisSelect | * | 0 | 1 | * |
                */

                bool axisSelect = (xAxisMatch == yAxisMatch) ? randomAxis : yAxisMatch;

                if (axisSelect)
                {
                    if (ray->p.posX > enemy->c.posX)
                    {
                        enemy->c.spriteRotation = 90;
                        shotVec.x               = TO_FX(1);
                        shotVec.y               = 0;
                    }
                    else
                    {
                        enemy->c.spriteRotation = 270;
                        shotVec.x               = -TO_FX(1);
                        shotVec.y               = 0;
                    }
                }
                else
                {
                    if (ray->p.posY > enemy->c.posY)
                    {
                        enemy->c.spriteRotation = 0;
                        shotVec.x               = 0;
                        shotVec.y               = TO_FX(1);
                    }
                    else
                    {
                        enemy->c.spriteRotation = 180;
                        shotVec.x               = 0;
                        shotVec.y               = -TO_FX(1);
                    }
                }

                // Shoot a bullet
                rayCreateBullet(ray,                                      //
                                OBJ_BULLET_SHIELD_0 + (esp_random() % 4), // Type
                                enemy->c.posX, enemy->c.posY,             // Position
                                shotVec.x, shotVec.y,                     // Velocity
                                0, 0,                                     // Acceleration
                                -1, false);

                state->state = TQ_EN_WIZARD_ST_IDLE;

                state->stateTimer = 3000000;
            }

            break;
        }
        case TQ_EN_WIZARD_ST_IDLE:
        {
            state->stateTimer -= elapsedUs;

            if (state->stateTimer < 0)
            {
                state->state = TQ_EN_WIZARD_ST_WARPING;
            }
            break;
        }
        case TQ_EN_WIZARD_ST_BUMPED:
        {
            state->stateTimer -= elapsedUs;

            if (state->stateTimer <= 0)
            {
                state->state      = TQ_EN_WIZARD_ST_WARPING;
                state->stateTimer = 200000;
            }
            break;
        }
    }

    // Move half as fast as than the player
    vec_q24_8 delta = {
        .x = (state->vel.x * (int32_t)elapsedUs) / (40000 * 6),
        .y = (state->vel.y * (int32_t)elapsedUs) / (40000 * 6),
    };

    // Move the bounding box without moving the enemy yet
    bb.pos.x += delta.x;
    bb.pos.y += delta.y;

    // Update the enemy position if it's in bounds
    if (rayBoundingBoxFitsInMap(ray, bb, NULL))
    {
        enemy->c.posX += delta.x;
        enemy->c.posY += delta.y;
    }
    else
    {
        // Can't move this way, pick a new direction
        state->vel.x = -state->vel.x;
        state->vel.y = -state->vel.y;
    }

    // Not dead yet!
    return false;
}

/**
 * @brief Function to check if the wizard collides with the player
 * Touching the wizard damages and bumps the player
 *
 * @param ray The whole game state
 * @param enemy The wizard to check for collisions
 * @param player The player to check for collisions
 * @param deltaX The X distance the player is trying to move
 * @param deltaY The Y distance the player is trying to move
 */
void rayEnemyWizardCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    // If the player is currently hittable
    if (((wizardState_t*)enemy->state)->state == TQ_EN_WIZARD_ST_IDLE)
    {
        checkPlayerEnemyStandardCollision(ray, enemy, player, deltaX, deltaY);
    }
}

/**
 * @brief This function is called when the wizard is hit
 *
 * @param ray The whole game state
 * @param enemy The enemy which was shot
 * @param bullet OBJ_BULLET_ARROW, OBJ_BULLET_BOMB, OBJ_BULLET_BOOMERANG, or OBJ_BULLET_SWORD
 */
void rayEnemyWizardGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Convenience pointer
    wizardState_t* state = enemy->state;

    if (state->state != TQ_EN_WIZARD_ST_IDLE)
    {
        return;
    }

    switch (bullet)
    {
        case OBJ_BULLET_SWORD:
        {
            // Give the player a bump away from the wizard
            state->vel.x = enemy->c.posX - ray->p.posX;
            state->vel.y = enemy->c.posY - ray->p.posY;
            fastNormVec(&state->vel.x, &state->vel.y);

            // Bump for 250ms
            state->state      = TQ_EN_WIZARD_ST_BUMPED;
            state->stateTimer = 250000;
        }
        // fall through
        case OBJ_BULLET_ARROW:
        case OBJ_BULLET_BOMB:
        case OBJ_BULLET_BOOMERANG:
        case OBJ_BULLET_SHIELD_0:
        case OBJ_BULLET_SHIELD_1:
        case OBJ_BULLET_SHIELD_2:
        case OBJ_BULLET_SHIELD_3:
        {
            enemy->health--;
            enemy->iFrameTimer = ENEMY_DEFAULT_IFRAMES_US;
            break;
        }
        default:
        {
            return;
        }
    }
}
