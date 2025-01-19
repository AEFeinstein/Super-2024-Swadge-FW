#ifndef _ENTITY_BIGBUG_H_
#define _ENTITY_BIGBUG_H_

#define sign(a) (((a) < 0) ? -1 : ((a) > 0))

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>
#include <stdbool.h>

#include "gameData_bigbug.h"
#include "entityManager_bigbug.h"
#include "sprite_bigbug.h"

#include "linked_list.h"
#include "vector2d.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    NULL_DATA,
    ATTACHMENT_ARM_DATA,
    WALKING_BUG_DATA,
    FLYING_BUG_DATA,
    CAR_DATA,
    DEATH_DUMPSTER_DATA,
    DIALOGUE_DATA,
    EGG_DATA,
    EGG_LEAVES_DATA,
    GAME_OVER_DATA,
    GARBOTNIK_DATA,
    GO_TO_DATA,
    GRABBY_HAND_DATA,
    HEAVY_FALLING_DATA,
    JANKY_BUG_DIG_DATA,
    RADAR_PING_DATA,
    MENU_BUG_DATA,
    MENU_DATA,
    PHYSICS_DATA,
    PROJECTILE_DATA,
    ROCKET_DATA,
    STUCK_HARPOON_DATA,
    SPIT_DATA,
    FOOD_CART_DATA,
    WILE_DATA,
    BB_501KG_DATA,
    EXPLOSION_DATA,
    ATMOSPHERIC_ATOMIZER_DATA,
    DRILL_BOT_DATA,
    SPACE_LASER_DATA,
    FINAL_BOSS_DATA,
} bb_data_type_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    vec_t vel;            // velocity
    vec_t accel;          // acceleration
    vec_t previousPos;    // position from the previous frame
    vec_t yaw;            //.x is the yaw, .y is the change in yaw over time. Gravitates toward left or right.
    uint8_t numHarpoons;  // number of harpoons
    int32_t fuel;         // garbotnik's remaining fuel. Like a level timer that can be influenced.
    list_t towedEntities; // A list of entities attached via tow cable.
    int16_t towTimer;     // Overflows to negative to detach a towed entity. Then resets to zero.

    // touchpad stuff
    bool touching;
    bool fire; // becomes true for a frame upon touchpad release "event"
    int32_t phi;
    int32_t r;
    int32_t intensity;

    int16_t activationRadius; // the radius at which touchpad input fires. Decrements while touchpad is held.

    // dialogue stuff
    int16_t landingPhrases[29];

    int8_t damageEffect;     // decrements over time. Render damagePalette color swap if > 0.
    int16_t harpoonCooldown; // decrements over time. Fires if < 0 and resets to GameData's GarbotnikStat_fireTime.

    uint8_t activeWile;

    uint8_t dragShift; // typically 17 or 19 with the atmospheric atomizer.
} bb_garbotnikData_t;

typedef struct
{
    bb_entity_t* parent;
    vec_t offset;
    uint16_t lifetime;
    vecFl_t floatVel;
} bb_stuckHarpoonData_t;

typedef struct // parent class
{
    uint8_t flags;       // bitpacked flags for various things.
                         // 0b00000001  // face left. Flip the sprite if true
                         // 0b00000010  // isTethered.
    uint8_t speed;       // randomized on creation. Used for walking or flying.
    int8_t health;       // bug dies at negative numbers
    int8_t damageEffect; // decrements over time. Render damagePalette color swap if > 0.
} bb_bugData_t;

typedef struct // child class
{
    uint8_t flags;       // bitpacked flags for various things.
                         // 0b00000001  // face left. Flip the sprite if true
                         // 0b00000010  // isTethered.
    uint8_t speed;       // randomized on creation. Used for walking or flying.
    int8_t health;       // bug dies at negative numbers
    int8_t damageEffect; // decrements over time. Render damagePalette color swap if > 0.
    //-----------------------------------------------
    enum bb_direction_t gravity; // to walk on the walls & ceiling: local gravity switches
    uint8_t fallSpeed;           // increments in free fall
} bb_walkingBugData_t;

typedef struct // child class
{
    uint8_t flags;       // bitpacked flags for various things.
                         // 0b00000001  // face left. Flip the sprite if true
                         // 0b00000010  // isTethered.
    uint8_t speed;       // randomized on creation. Used for walking or flying.
    int8_t health;       // bug dies at negative numbers
    int8_t damageEffect; // decrements over time. Render damagePalette color swap if > 0.
    //-----------------------------------------------
    vec_t direction; // buggo moves in the direction vector.
} bb_flyingBugData_t;

typedef struct
{
    int8_t selectionIdx;
    bb_entity_t* cursor;
} bb_menuData_t;

typedef struct
{
    vec_t vel;
    uint16_t lifetime;
    bool prevFrameInAir; // Was this previous frame located in air or garbage.
} bb_projectileData_t;

typedef struct
{
    int32_t yVel;
    bb_entity_t* flame; // tracks the flame to update position like a child object
    uint16_t numBugs;   // number of bugs in the booster
    uint8_t numDonuts;  // number of donuts in the booster
    int32_t armAngle;   // Typically rotated at 180. Increments to 359 while garbotnik is on the booster.
} bb_rocketData_t;

typedef struct
{
    int32_t yVel; // FINISH ME!!! maybe get away with int16_t
} bb_heavyFallingData_t;

typedef struct
{
    vec_t vel;
    uint8_t bounceNumerator; // numerator and denominator are used to control bounciness. 1/1 reflects velocity with the
                             // same magnitude. 1/4 absorbs 75% velocity on a bounce. 2/1 would be looney toons physics.
    uint8_t bounceDenominator;
    uint8_t tileTime; // Only relevant for Garbotnik's dying scenario (and wiles now). Goes up with every tile collision
                      // and decrements steadily over time. So it serves to detect when he is steadily sitting
                      // on the ground and trigger a game over.
} bb_physicsData_t;

typedef struct
{
    vec_t vel;
    uint8_t bounceNumerator; // numerator and denominator are used to control bounciness. 1/1 reflects velocity with the
                             // same magnitude. 1/4 absorbs 75% velocity on a bounce. 2/1 would be looney toons physics.
    uint8_t bounceDenominator;
    uint8_t tileTime; // Only relevant for Garbotnik's dying scenario (and wiles now). Goes up with every tile collision
                      // and decrements steadily over time. So it serves to detect when he is steadily sitting
                      // on the ground and trigger a game over.
    // child attributes
    uint16_t lifetime;
} bb_timedPhysicsData_t;

typedef struct
{
    vec_t vel;
    uint8_t bounceNumerator; // numerator and denominator are used to control bounciness. 1/1 reflects velocity with the
                             // same magnitude. 1/4 absorbs 75% velocity on a bounce. 2/1 would be looney toons physics.
    uint8_t bounceDenominator;
    uint8_t tileTime; // Only relevant for Garbotnik's dying scenario (and wiles now). Goes up with every tile collision
                      // and decrements steadily over time. So it serves to detect when he is steadily sitting
                      // on the ground and trigger a game over.
    // child attributes
    int16_t targetY; // The y position to reach before stopping.
    uint16_t lifetime;
    bool facingRight;
    int8_t attacking; // janky way to overide the animation to drilling when colliding with a bug. Decrements to zero.
} bb_drillBotData_t;

typedef struct
{
    vec_t vel;
    uint8_t bounceNumerator; // numerator and denominator are used to control bounciness. 1/1 reflects velocity with the
                             // same magnitude. 1/4 absorbs 75% velocity on a bounce. 2/1 would be looney toons physics.
    uint8_t bounceDenominator;
    uint8_t tileTime; // Only relevant for Garbotnik's dying scenario (and wiles now). Goes up with every tile collision
                      // and decrements steadily over time. So it serves to detect when he is steadily sitting
                      // on the ground and trigger a game over.
    // new attributes
    uint16_t lifetime;
    uint8_t wileIdx; // the index of the wile to activate.
} bb_wileData_t;

typedef struct
{
    int8_t xVel;
    bool firstTrip;
} bb_menuBugData_t;

typedef struct
{
    uint8_t brightness; // stores brightness so that it may be stored so it may be calculated once then used to
                        // stimulate the egg and also to draw.
    bb_entity_t* egg;   // tracks the egg to stimulate it.
} bb_eggLeavesData_t;

typedef struct
{
    uint16_t stimulation; // once it reaches 600, it turns into a bug.
} bb_eggData_t;

typedef struct
{
    bb_entity_t* rocket; // a reference to the booster. To reposition self and Notify to take off at angle = 359.
} bb_attachmentArmData_t;

typedef struct
{
    bb_entity_t* grabbed; // a bug that is tragically grabbed into the booster
    bb_entity_t* rocket;  // a reference to the booster. To reposition self.
} bb_grabbyHandData_t;

typedef struct
{
    bb_entity_t* jankyBugDig[6]; // When a bug collides with this, the dirt "digs" toward the car fight arena
    bb_spriteDef_t reward;       // The sprite to spawn when the car trunk opens
} bb_PointOfInterestParentData_t;

typedef struct
{
    bb_entity_t* jankyBugDig[6]; // When a bug collides with this, the dirt "digs" toward the car fight arena
    bb_spriteDef_t reward;       // The sprite to spawn when the car trunk opens
    midiFile_t alarm;            // The midi file to play when the car is active. Can be 1 of 3 files.
    bool midiLoaded;             // True if the midi file is loaded, false otherwise.
    uint16_t textTimer;          // The timer for the text to appear on screen.
} bb_carData_t;

typedef struct
{
    bb_entity_t* jankyBugDig[6]; // When a bug collides with this, the dirt "digs" toward the car fight arena
    bb_spriteDef_t reward;       // The sprite to spawn when the food cart is destroyed.
    bb_entity_t* partner;        // the other piece of the food cart
    int8_t damageEffect; // decrements over time. Render damagePalette color swap if > 0, and if this cart is not the
                         // zero animation frame because the cart background gets not graphical effect.
} bb_foodCartData_t;

typedef struct
{
    vec_t vel;
    uint16_t lifetime;
} bb_spitData_t;

typedef struct
{
    bb_entity_t* tracking;
    uint16_t speed;
    uint32_t midPointSqDist; // The point at which to decelerate.
    bb_callbackFunction_t executeOnArrival;
} bb_goToData;

typedef struct
{
    uint8_t numStrings;
    char** strings; // the list of strings to be spoken in order.
    int8_t curString;
    char** characters;                   // the list of characters who are speaking in order.
    bb_callbackFunction_t endDialogueCB; // executes when the character is done talking.
    wsg_t sprite;                        // the current talking sprite
    wsg_t spriteNext;                    // a blinking triangle right of the dialogue
    int16_t offsetY;                     // Track the sprite sliding up or down on screen.
    int8_t blinkTimer;
} bb_dialogueData_t;

typedef struct
{
    bool loaded; // tracks wether or not the death dumpster sprite is loaded
} bb_DeathDumpsterData_t;

typedef struct
{ // There are 3 game over graphics that take turns loading into wsg_t fullscreenGraphic;
    bool wsgLoaded;
    wsg_t fullscreenGraphic;
} bb_gameOverData_t;

typedef struct
{
    enum bb_direction_t arena; // The direction to dig towards the car fight arena.
    uint8_t numberOfDigs;      // Increments with each dig. Destroy self at 2. That's three digs when you count 0.
} bb_jankyBugDigData_t;

typedef struct
{
    vec_t vel;
    int16_t targetY; // The y position to reach before stopping.
    int16_t angle;   // The angle to rotate the sprite. Calculate once at creation.
} bb_501kgData_t;

typedef struct
{
    uint16_t lifetime;
    uint16_t radius;
} bb_explosionData_t;

typedef struct
{
    uint16_t lifetime;
} bb_atmosphericAtomizerData_t;

typedef struct
{
    vec_t pos;
    uint16_t radius;
} pingCircle;

typedef struct
{
    uint16_t radius;
    uint8_t reflectionIdx;
    uint16_t timer;
    pingCircle reflections[20];
    paletteColor_t color;
    bb_callbackFunction_t executeAfterPing;
} bb_radarPingData_t;

typedef struct
{
    uint16_t lifetime;
    uint8_t highestGarbage;
} bb_spaceLaserData_t;

typedef struct
{
    vec_t vel;
    int8_t damageEffect; // decrements over time.
    int16_t health;
    bb_entity_t* bossEggs[10];
    int16_t stateTimer; // increments over time. Doing some attacks when positive. Overflows intentionally.
    bool firstDialogeDone;
    bool secondDialogeDone;
} bb_finalBossData_t;

typedef void (*bb_updateFunction_t)(bb_entity_t* self);
typedef void (*bb_updateFarFunction_t)(bb_entity_t* self);
typedef void (*bb_drawFunction_t)(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
typedef void (*bb_collisionHandler_t)(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
typedef struct
{
    list_t* checkOthers; // A list of bb_spriteDef_t's to check collision against. i.e. all bug spriteDef indices for
                         // the harpoon.
    bb_collisionHandler_t function; // Triggers on collision enter with any of the checkOthers.
} bb_collision_t;

struct bb_entity_t
{
    bool active;
    bool cacheable;

    void* data;
    bb_data_type_t dataType;
    bb_updateFunction_t updateFunction;       // Only set for entities that need update logic
    bb_updateFarFunction_t updateFarFunction; // Only set for execution when the entity is far from the camera center
    bb_drawFunction_t drawFunction;           // Only set for entities such as Garbotnik that need custom drawing logic
    list_t* collisions; // It's a list of bb_collision_t which each get processed by the entity manager.

    vec_t pos;

    bb_animationType_t type;
    bb_spriteDef_t spriteIndex;
    bool paused;

    uint16_t animationTimer;
    uint8_t gameFramesPerAnimationFrame;
    uint8_t currentAnimationFrame;

    bb_gameData_t* gameData;

    int16_t halfWidth;  // Distance from the origin to the side edge (for AABB physics)
    int16_t halfHeight; // Distance from the origin to the top edge (for AABB physics)
    int32_t cSquared;   // Squared distance from the sprite origin to the corner of the AABB hitbox. Used for collision
                        // optimization.
};

//==============================================================================
// Prototypes
//==============================================================================
void bb_initializeEntity(bb_entity_t* self, bb_entityManager_t* entityManager, bb_gameData_t* gameData);

void bb_setData(bb_entity_t* self, void* data, bb_data_type_t dataType);
void bb_clearCollisions(bb_entity_t* self, bool keepCached);

void bb_destroyEntity(bb_entity_t* self, bool caching, bool wasInTheMainArray);

void bb_updateRocketLanding(bb_entity_t* self);
void bb_updateRocketLiftoff(bb_entity_t* self);
void bb_updateHeavyFallingInit(bb_entity_t* self);
void bb_updateHeavyFalling(bb_entity_t* self);
void bb_updatePhysicsObject(bb_entity_t* self);
void bb_updateGarbotnikDeploy(bb_entity_t* self);
void bb_updateGarbotnikFlying(bb_entity_t* self);
void bb_updateGarbotnikDying(bb_entity_t* self);
void bb_updateHarpoon(bb_entity_t* self);
void bb_updateStuckHarpoon(bb_entity_t* self);
void bb_updateEggLeaves(bb_entity_t* self);
void bb_updateFarEggleaves(bb_entity_t* self);
void bb_updateFarDestroy(bb_entity_t* self);
void bb_updateFarMenu(bb_entity_t* self);
void bb_updateFarMenuAndUnload(bb_entity_t* self);
void bb_updateMenuBug(bb_entity_t* self);
void bb_updateMoveLeft(bb_entity_t* self);
void bb_rotateBug(bb_entity_t* self, int8_t orthogonalRotations);
void bb_updateBugShooting(bb_entity_t* self);
void bb_updateWalkingBug(bb_entity_t* self);
void bb_updateFlyingBug(bb_entity_t* self);
void bb_updateMenu(bb_entity_t* self);
void bb_updatePOI(bb_entity_t* self);
void bb_updateFlame(bb_entity_t* self);
void bb_updateCharacterTalk(bb_entity_t* self);
void bb_updateAttachmentArm(bb_entity_t* self);
void bb_updateGameOver(bb_entity_t* self);
void bb_updateRadarPing(bb_entity_t* self);
void bb_updateGrabbyHand(bb_entity_t* self);
void bb_updateFarGrabbyHand(bb_entity_t* self);
void bb_updateDoor(bb_entity_t* self);
void bb_updateCarActive(bb_entity_t* self);
void bb_updateCarOpen(bb_entity_t* self);
void bb_updateFarCar(bb_entity_t* self);
void bb_updateSpit(bb_entity_t* self);
void bb_updatePangoAndFriends(bb_entity_t* self);
void bb_updateDiveSummary(bb_entity_t* self);
void bb_updateWile(bb_entity_t* self);
void bb_update501kg(bb_entity_t* self);
void bb_updateExplosion(bb_entity_t* self);
void bb_updateAtmosphericAtomizer(bb_entity_t* self);
void bb_updateDrillBot(bb_entity_t* self);
void bb_updateTimedPhysicsObject(bb_entity_t* self);
void bb_updatePacifier(bb_entity_t* self);
void bb_updateSpaceLaser(bb_entity_t* self);
void bb_updateQuickplay(bb_entity_t* self);
void bb_updateFinalBoss(bb_entity_t* self);

void bb_drawGarbotnikFlying(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawHarpoon(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawStuckHarpoon(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawEggLeaves(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawEgg(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawBasicEmbed(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawMenu(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawMenuForeground(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawStar(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawNothing(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawMenuBug(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawCharacterTalk(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawGameOver(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawAttachmentArm(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawDeathDumpster(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawRadarPing(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawBug(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawRocket(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawCar(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawSpit(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawHitEffect(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawGrabbyHand(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawDiveSummary(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawFoodCart(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawWile(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_draw501kg(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawExplosion(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawAtmosphericAtomizer(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawDrillBot(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawPacifier(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawSpaceLaser(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawDeadBug(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawGarbotnikUI(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawQuickplay(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawFinalBoss(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);

// void bb_drawRect(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);

void bb_onCollisionHarpoon(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionSimple(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
bool bb_onCollisionHeavyFalling(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionHeavyFallingBug(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionHeavyFallingGarbotnik(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionRocketGarbotnik(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionCarIdle(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionAttachmentArm(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionFuel(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionGrabbyHand(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionJankyBugDig(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionSpit(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionSwadge(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionFoodCart(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionDrillBot(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionAmmoSupply(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionBrickTutorial(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionSpaceLaserGarbotnik(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionSpaceLaserBug(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionBossEgg(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);
void bb_onCollisionBoss(bb_entity_t* self, bb_entity_t* other, bb_hitInfo_t* hitInfo);

// callbacks
void bb_startGarbotnikIntro(bb_entity_t* self);
void bb_startGarbotnikLandingTalk(bb_entity_t* self);
void bb_startGarbotnikCloningTalk(bb_entity_t* self);
void bb_startGarbotnikEggTutorialTalk(bb_entity_t* self);
void bb_startGarbotnikFuelTutorialTalk(bb_entity_t* self);
void bb_afterGarbotnikTutorialTalk(bb_entity_t* self);
void bb_afterGarbotnikEggTutorialTalk(bb_entity_t* self);
void bb_afterGarbotnikIntro(bb_entity_t* self);
void bb_afterGarbotnikLandingTalk(bb_entity_t* self);
void bb_afterLiftoffInteraction(bb_entity_t* self);
void bb_deployBooster(bb_entity_t* self);
void bb_openMap(bb_entity_t* self);
void bb_upgradeRadar(bb_entity_t* self);
void bb_triggerGameOver(bb_entity_t* self);
void bb_upgradeGarbotnik(bb_entity_t* self);
void bb_playCarAlarm(bb_entity_t* self);
void bb_bugDeath(bb_entity_t* self, bb_hitInfo_t* hitInfo);
void bb_cartDeath(bb_entity_t* self, bb_hitInfo_t* hitInfo);
void bb_spawnHorde(bb_entity_t* self, uint8_t numBugs);
void bb_afterEnding(bb_entity_t* self);

void bb_crumbleDirt(bb_gameData_t* gameData, uint8_t gameFramesPerAnimationFrame, uint8_t tile_i, uint8_t tile_j,
                    bool zeroHealth, bool flagNeighborsForPathfinding);
bb_dialogueData_t* bb_createDialogueData(uint8_t numStrings, const char* firstCharacter);
void bb_setCharacterLine(bb_dialogueData_t* dData, uint8_t index, const char* character, const char* str);
void bb_freeDialogueData(bb_dialogueData_t* dData);
int8_t bb_compareWileCallSequences(enum bb_direction_t* playerInputSequence, enum bb_direction_t* compareTo);

#endif