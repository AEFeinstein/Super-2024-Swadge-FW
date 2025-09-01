#pragma once
//==============================================================================
// Includes
//==============================================================================

#include "danceNetwork.h"
#include "dn_entityManager.h"
#include "dn_typedef.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    DN_NULL_DATA,
    DN_BOARD_DATA,
    DN_INSTRUCTION_DATA,
    DN_CURTAIN_DATA,
    DN_ALBUMS_DATA,
    DN_ALBUM_DATA,
    DN_CHARACTER_SELECT_DATA,
    DN_TILE_SELECTOR_DATA,
    DN_PROMPT_DATA,
    DN_UPGRADE_MENU_DATA,
    DN_SWAP_DATA,
    DN_BULLET_DATA,
    DN_UNIT_DATA,
    DN_TUTORIAL_DATA,
    DN_SWAPBUTTON_DATA,
    DN_SKIPBUTTON_DATA,
} dn_dataType_t;

//==============================================================================
// Typedefs
//==============================================================================
typedef void (*dn_updateFunction_t)(dn_entity_t* self);
typedef void (*dn_updateFarFunction_t)(dn_entity_t* self);
typedef void (*dn_drawFunction_t)(dn_entity_t* self);

//==============================================================================
// Structs
//==============================================================================
struct dn_entity_t
{
    void* data;
    dn_dataType_t dataType;
    dn_updateFunction_t updateFunction; // Only set for entities that need update logic
    bool flipped;                       // draw flipped
    dn_drawFunction_t drawFunction;     // Only set for entities such as Garbotnik that need custom drawing logic
    bool destroyFlag; // Entity will be cleanly destroyed after engine updating and before engine drawing.
    vec_t pos;
    dn_animationType_t type;
    bool paused;
    dn_assetIdx_t assetIndex;

    uint16_t animationTimer;
    uint8_t gameFramesPerAnimationFrame;
    uint8_t currentAnimationFrame;
    dn_gameData_t* gameData;
};

typedef struct
{
    uint16_t yOffset;
    int16_t yVel;
    dn_entity_t* unit;     // Pointer to the unit on this tile. NULL if no unit is present.
    dn_entity_t* selector; // Pointer to the tile selector. NULL if no selector is present.
    dn_selection_t selectionType;
    bool timeout;          // Becomes true if shot out. Becomes false when another tile is shot out.
    uint8_t timeoutOffset; // further offsets the yOffset when in timeout.
    uint8_t rewards;       // some number of rerolls given to the first visitor here.
} dn_tileData_t;

typedef struct
{
    dn_boardPos_t moveTo; // Coordinates the unit is moving to.
} dn_unitData_t;

typedef struct
{
    dn_tileData_t tiles[DN_BOARD_SIZE][DN_BOARD_SIZE];
    dn_entity_t* p1Units[5]; // Pointers to player 1's units. The first unit is king, the other 4 are pawns. NULL
                             // pointers for captured units.
    dn_entity_t* p2Units[5]; // Pointers to player 2's units. The first unit is king, the other 4 are pawns. NULL
                             // pointers for captured units.
    dn_boardPos_t impactPos; // x and y indices of an impact effect.
} dn_boardData_t;

typedef struct
{
    int16_t separation; // The distance between the curtain and the center of the screen
} dn_curtainData_t;

typedef struct
{
    dn_entity_t* p1Album;
    dn_entity_t* creativeCommonsAlbum;
    dn_entity_t* p2Album;
} dn_albumsData_t;

typedef struct
{
    wsgPalette_t screenOffPalette; // replaces album tile colors with the final track colors. The first index color is
                                   // C255. No action is C344.
    wsgPalette_t screenOnPalette;  // Screen is on. Just a more vibrant variation of screenOff.
    wsgPalette_t screenAttackPalette; // Makes some explosions appear at certain tracks.
    dn_track_t tracks[16];            // Array of action tracks in this album in raster order.
    uint16_t rot;                     // Rotation degrees from 0-359 for drawing.
    bool cornerLightBlinking;
    bool cornerLightOn;
    bool screenIsOn;
    int32_t timer;
} dn_albumData_t;

typedef struct
{
    bool selectDiamondShape[45];
    int8_t selectCharacterIdx;
    int32_t xSelectScrollTimer;
    int16_t xSelectScrollOffset;
} dn_characterSelectData_t;

typedef struct
{
    // A bunch of y positions for lines going up.
    uint8_t lineYs[NUM_SELECTOR_LINES];
    // colors that get used for drawing the lines.
    paletteColor_t colors[3];
    // which chess tile the selector is on
    dn_boardPos_t pos;
    // callback when A is pressed.
    dn_callbackFunction_t a_callback;
    // callback when B is pressed.
    dn_callbackFunction_t b_callback;
    dn_entity_t* selectedUnit;
    // Stores bounce location for certain attacks
    dn_boardPos_t bounce;
} dn_tileSelectorData_t;

typedef struct
{
    char text[8];
    dn_callbackFunction_t callback;
    bool downPressDetected;
    int16_t selectionAmount;
} dn_promptOption_t;

typedef struct
{
    bool animatingIntroSlide;
    int16_t yOffset;
    uint8_t selectionIdx;
    bool playerHasSlidThis;
    bool usesTwoLinesOfText;
    char text[40];
    char text2[40];
    uint8_t numOptions;
    list_t* options;
    bool isPurple;
} dn_promptData_t;

typedef struct
{
    int32_t timer;

    int8_t selectionIdx; // The selected menu item.

    dn_track_t trackColor;
    dn_boardPos_t track[17]; // relative vector from a unit. 16 plus a null separator
    uint8_t album[4];        // 0 for p1's album, 2 for creative commons album, 1 for p2's album. 3 plus a 3 separator

    dn_promptOption_t options[4]; // reroll 1, reroll 2, reroll 3, and confirm.

    int8_t numTracksToAdd;
    int8_t flashyBoxSize;
} dn_upgradeMenuData_t;

typedef struct
{
    dn_entity_t* firstAlbum;
    dn_entity_t* secondAlbum;
    uint8_t firstAlbumIdx;
    uint8_t secondAlbumIdx;
    int16_t lerpAmount;
    vec_t center;
    vec_t offset;
    bool progressPhase;
} dn_swapAlbumsData_t;

typedef struct
{
    vec_t start;
    vec_t end;
    int8_t yOffset;
    int16_t lerpAmount;
    dn_boardPos_t firstTarget;
    dn_boardPos_t secondTarget;
    dn_boardPos_t ownerToMove; // Position of the attacker if they are remixing and need to move after the attack.
} dn_bulletData_t;

typedef struct
{
    uint8_t page;
    bool advancedTips;
} dn_tutorialData_t;


//==============================================================================
// Prototypes
//==============================================================================
void dn_setData(dn_entity_t* self, void* data, dn_dataType_t dataType);

// main game entities
void dn_drawAsset(dn_entity_t* self);
void dn_drawNothing(dn_entity_t* self);

void dn_updateBoard(dn_entity_t* self);
bool dn_belongsToP1(dn_entity_t* unit);
void dn_drawBoard(dn_entity_t* self);
bool dn_availableMoves(dn_entity_t* unit, list_t* movesList);
dn_track_t dn_trackTypeAtColor(dn_entity_t* album, paletteColor_t trackCoords);
dn_track_t dn_trackTypeAtCoords(dn_entity_t* album, dn_boardPos_t trackCoords);
bool dn_calculateMoveableUnits(dn_entity_t* board);
bool dn_isKing(dn_entity_t* unit);

void dn_updateCurtain(dn_entity_t* self);
void dn_drawCurtain(dn_entity_t* self);

void dn_drawAlbums(dn_entity_t* self);

dn_boardPos_t dn_colorToTrackCoords(paletteColor_t color);
dn_twoColors_t dn_trackCoordsToColor(dn_boardPos_t trackCoords);
void dn_addTrackToAlbum(dn_entity_t* album, dn_boardPos_t trackCoords, dn_track_t track);
void dn_updateAlbum(dn_entity_t* self);
void dn_drawAlbum(dn_entity_t* self);

// characterSelect entities
void dn_updateCharacterSelect(dn_entity_t* self);
void dn_drawCharacterSelect(dn_entity_t* self);

void dn_updateTileSelector(dn_entity_t* self);
void dn_drawTileSelectorBackHalf(dn_entity_t* self, int16_t x, int16_t y);
void dn_drawTileSelectorFrontHalf(dn_entity_t* self, int16_t x, int16_t y);
void dn_trySelectUnit(dn_entity_t* self);
void dn_cancelSelectUnit(dn_entity_t* self);
void dn_trySelectTrack(dn_entity_t* self);
void dn_cancelSelectTrack(dn_entity_t* self);

void dn_drawPlayerTurn(dn_entity_t* self);

void dn_updatePrompt(dn_entity_t* self);
void dn_drawPrompt(dn_entity_t* self);
void dn_startTurn(dn_entity_t* self);
void dn_gainReroll(dn_entity_t* self);
void dn_gainRerollAndSetupDancePhase(dn_entity_t* self);
void dn_setupDancePhase(dn_entity_t* self);
void dn_acceptRerollAndSkip(dn_entity_t* self);
void dn_acceptRerollAndSwapHelper(dn_entity_t* self, bool progressPhase);
void dn_acceptRerollAndSwap(dn_entity_t* self);
void dn_acceptRerollAndSwapAndProgress(dn_entity_t* self);
void dn_acceptThreeRerolls(dn_entity_t* self);
void dn_clearSelectableTiles(dn_entity_t* self);
void dn_startUpgradeMenu(dn_entity_t* self, int32_t countOff);
void dn_acceptSwapCC(dn_entity_t* self);
void dn_incrementPhase(dn_entity_t* self);

void dn_drawPit(dn_entity_t* self);
void dn_drawPitForeground(dn_entity_t* self);

dn_boardPos_t dn_getUnitBoardPos(dn_entity_t* unit);

void dn_updateUpgradeMenu(dn_entity_t* self);
void dn_updateAfterUpgradeMenu(dn_entity_t* self);
void dn_drawUpgradeMenu(dn_entity_t* self);
void dn_initializeSecondUpgradeOption(dn_entity_t* self);
void dn_initializeThirdUpgradeOption(dn_entity_t* self);
void dn_initializeFirstUpgradeOption(dn_entity_t* self);
void dn_initializeUpgradeConfirmOption(dn_entity_t* self);
void dn_rerollSecondUpgradeOption(dn_entity_t* self);
void dn_rerollThirdUpgradeOption(dn_entity_t* self);
void dn_rerollFirstUpgradeOptionFree(dn_entity_t* self);
void dn_rerollFirstUpgradeOption(dn_entity_t* self);
void dn_confirmUpgrade(dn_entity_t* self);
void dn_updateSwapAlbums(dn_entity_t* self);
void dn_updateAfterSwap(dn_entity_t* self);
void dn_setBlinkingLights(dn_entity_t* self);

void dn_updateBullet(dn_entity_t* self);
void dn_drawBullet(dn_entity_t* self);

void dn_moveUnit(dn_entity_t* self);

void dn_afterPlunge(dn_entity_t* self);

void dn_sharedButtonLogic(dn_entity_t* self);

void dn_updateSwapButton(dn_entity_t* self);
void dn_drawSwapButton(dn_entity_t* self);
void dn_unpauseSwapButton(dn_entity_t* self);

void dn_updateSkipButton(dn_entity_t* self);
void dn_drawSkipButton(dn_entity_t* self);
void dn_unpauseSkipButton(dn_entity_t* self);

void dn_updateTutorial(dn_entity_t* self);
void dn_drawTutorial(dn_entity_t* self);

dn_entity_t* dn_findLastEntityOfType(dn_entity_t* self, dn_dataType_t type);

void dn_setupBounceOptions(dn_entity_t* self);

void dn_trySelectBounceDest(dn_entity_t* self);
void dn_cancelSelectBounceDest(dn_entity_t* self);

void dn_exitSubMode(dn_entity_t* self);