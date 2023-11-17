#ifndef _ENTITYMANAGER_H_
#define _ENTITYMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "breakout_typedef.h"
#include "entity.h"
#include "tilemap.h"
#include "gameData.h"
#include "hdw-tft.h"
#include "sprite.h"
#include "soundManager.h"

//==============================================================================
// Constants
//==============================================================================
#define MAX_ENTITIES   32
#define SPRITESET_SIZE 33

//==============================================================================
// Structs
//==============================================================================

struct entityManager_t
{
    sprite_t sprites[SPRITESET_SIZE];
    entity_t* entities;
    uint8_t activeEntities;

    entity_t* viewEntity;
    entity_t* playerEntity;

    tilemap_t* tilemap;
};

//==============================================================================
// Prototypes
//==============================================================================
void initializeEntityManager(entityManager_t* entityManager, tilemap_t* tilemap, gameData_t* gameData,
                             soundManager_t* soundManager);
void loadSprites(entityManager_t* entityManager);
void updateEntities(entityManager_t* entityManager);
void deactivateAllEntities(entityManager_t* entityManager, bool excludePlayer, bool excludePersistent, bool respawn);
void drawEntities(entityManager_t* entityManager);
entity_t* findInactiveEntity(entityManager_t* entityManager);

void viewFollowEntity(tilemap_t* tilemap, entity_t* entity);
entity_t* createEntity(entityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y);
entity_t* createPlayer(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createPlayerPaddleTop(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createPlayerPaddleLeft(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createPlayerPaddleRight(entityManager_t* entityManager, uint16_t x, uint16_t y);

entity_t* createBall(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createCaptiveBall(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createTimeBomb(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createExplosion(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createRemoteBomb(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createBallTrail(entityManager_t* entityManager, uint16_t x, uint16_t y);

entity_t* createChoIntro(entityManager_t* entityManager, uint16_t x, uint16_t y);
entity_t* createCrawler(entityManager_t* entityManager, uint16_t x, uint16_t y);

void freeEntityManager(entityManager_t* entityManager);

#endif
