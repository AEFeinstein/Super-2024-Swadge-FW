//==============================================================================
// Includes
//==============================================================================

#include "fill.h"
#include "ray_cutscene.h"
#include "esp_wifi.h"
#include "ray_script.h"
#include "ray_pause.h"
#include "cutscene.h"

static void rayCutsceneCb(void);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Show a cutscene. The two non-ray arguments are mutually exclusive
 *
 * @param ray The entire game state
 * @param type The NPC type for this cutscene, or EMPTY
 * @param text The The title of a scene, the text to show for this cutscene, or NULL
 */
void rayShowCutscene(ray_t* ray, rayMapCellType_t type, char* text)
{
    // Pause music and switch to the cutscene
    globalMidiPlayerPauseAll();
    raySwitchToScreen(RAY_CUTSCENE);

    // Initialize the cutscene
    ray->cutscene         = initCutscene(rayCutsceneCb, CNFS_NUM_FILES, 0);
    ray->cutsceneBtnState = 0;
    ray->cutsceneDone     = false;

    // Always load these two by default
    addCutsceneStyle(ray->cutscene, c000, TQ_TOMI_PORTRAIT_00_COCKY_WSG, CUTSCENE_TEXTBOX_WSG, "Tomi",
                     NUM_TOMI_EMOTIONS, false, true, true);
    addCutsceneStyle(ray->cutscene, c000, TQ_HYUT_PORTRAIT_00_NEUTRAL_WSG, CUTSCENE_TEXTBOX_WSG, "Hyut",
                     NUM_HYUT_EMOTIONS, false, true, true);

    // Initialize the cutscene based on NPC or scene
    if (EMPTY != type)
    {
        switch (type)
        {
            case OBJ_ENEMY_NPC_FAIRY_KAITIE:
            {
                initNpcCutsceneKaitie(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_SABLE:
            {
                initNpcCutsceneSable(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_BEAN:
            {
                initNpcCutsceneBean(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_DACC:
            {
                initNpcCutsceneDacc(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_KIKI:
            case OBJ_ENEMY_NPC_FAIRY_BOUBA:
            {
                initNpcCutsceneKikiBouba(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_BARRY:
            {
                initNpcCutsceneBarryTheFairy(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_HYUT:
            {
                // No dialog for Hyut, only scenes
                break;
            }
            case OBJ_ENEMY_NPC_GREAT_DEEJAY_TREE:
            {
                initNpcCutsceneGreatDejayTree(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_LOOT:
            {
                initNpcCutsceneLoot(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_DORIA:
            {
                initNpcCutsceneDoria(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_KING_DONUT:
            {
                initNpcCutsceneKingDonut(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_MEGA_PULSE:
            {
                initNpcCutsceneMegaPulse(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_CHO:
            {
                initNpcCutsceneCho(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_TRASH_MAN:
            {
                initNpcCutsceneTrashMan(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_EMBER_DEMON:
            {
                initNpcCutsceneEmberDemon(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_THE_MAYOR:
            {
                initNpcCutsceneTheMayor(ray, ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FREDWARD:
            {
                initNpcCutsceneFredward(ray, ray->cutscene);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if (NULL != text)
    {
        // Check if this text is a scene title
        rayCutsceneSceneInitFn_t scenes[] = {
            initCutsceneIntro,
            initCutscenePairing,
            initCutsceneHyutIntro,
            initCutsceneHyutPairing,
            initCutsceneLootIntro,
            initCutsceneDoriaIntro,
            initCutsceneDungeon1Complete,
            initCutsceneDungeon2Complete,
            initCutsceneDungeon3Complete,
            initCutsceneDungeon4Complete,
            initCutsceneSinging,
            initCutsceneFinalFight,
            initCutsceneFightComplete,
            initCutsceneGreatFairy,
            initCutsceneDream1,
            initCutsceneDream2,
            initCutsceneDream3,
            initCutsceneFinalWarp,
            initCutsceneStatue,
            initCutsceneBombGet,
            initCutsceneBowGet,
            initCutsceneTurntablesGet,
            initCutsceneDungeon4Tomb,
        };

        bool sceneStarted = false;
        for (int sIdx = 0; sIdx < ARRAY_SIZE(scenes); sIdx++)
        {
            if (scenes[sIdx](ray, ray->cutscene, text))
            {
                if (0 == ray->cutscene->lines.length)
                {
                    // Zero lines indicates the cutscene name matched, but has already been viewed
                    ray->cutsceneDone = true;
                    return;
                }
                else
                {
                    sceneStarted = true;
                }
                break;
            }
        }

        if (false == sceneStarted)
        {
            // Blank style for signposts and such
            addCutsceneStyleSign(ray->cutscene);
            // Add the single line
            addCutsceneLine(ray->cutscene, 2, text, false, 0, NULL);
        }
    }
}

/**
 * @brief Callback when the cutscene is done to resume the game
 *
 */
static void rayCutsceneCb(void)
{
    // Raise a flag to resume the game in the main function
    // Freeing the cutscene here causes a segfault
    getRayState()->cutsceneDone = true;
}

/**
 * @brief Check for button input while showing cutscene and either show the next part of the cutscene or return to the
 * game loop
 *
 * @param ray The entire game state
 */
void rayCutsceneCheckButtons(ray_t* ray)
{
    // Check the button queue
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // Save button state
        ray->cutsceneBtnState = evt.state;
        // Update cutscene with button presses
        updateCutscene(ray->cutscene, ray->cutsceneBtnState);
    }
}

/**
 * @brief Render the current cutscene box
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayCutsceneRender(ray_t* ray, uint32_t elapsedUs)
{
    // If the cutscene is done
    if (ray->cutsceneDone || NULL == ray->cutscene->lines.first)
    {
        // Switch back to the game. This will free memory
        ray->cutsceneDone = false;
        raySwitchToScreen(RAY_GAME);
        globalMidiPlayerResumeAll();
    }
    else
    {
        // Update and draw the cutscene
        updateCutscene(ray->cutscene, ray->cutsceneBtnState);
        drawCutscene(ray->cutscene, ray->renderer->menuFont);
    }
}

/**
 * @brief Add a narrator style to the cutscene
 *
 * @param cutscene The cutscene to add a style to
 */
void addCutsceneStyleNarrator(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, -1, CUTSCENE_TEXTBOX_WSG, "Narrator", 1, true, false, true);
}

/**
 * @brief Add an unknown style to the cutscene
 *
 * @param cutscene The cutscene to add a style to
 */
void addCutsceneStyleUnknown(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, -1, CUTSCENE_TEXTBOX_WSG, "???", 1, true, false, true);
}

/**
 * @brief Add Hank Waddledorf style to the cutscene
 *
 * @param cutscene The cutscene to add a style to
 */
void addCutsceneStyleHankW(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_WADDLEDORF_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Hank Waddledorf", 1, true, true,
                     true);
}

/**
 * @brief Add Great Fairy style to the cutscene
 *
 * @param cutscene The cutscene to add a style to
 */
void addCutsceneStyleGreatFairy(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_GREAT_FAIRY_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Great Fairy", 1, true, true,
                     true);
}

/**
 * @brief Add Great Fairies style to the cutscene
 *
 * @param cutscene The cutscene to add a style to
 */
void addCutsceneStyleGreatFairies(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_GREAT_FAIRIES_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Great Fairies", 1, true,
                     true, true);
}

/**
 * @brief Delete Doria from the scene
 *
 */
void deleteDoria(void)
{
    ray_t* ray = getRayState();

    // Iterate through all enemies
    node_t* eNode = ray->enemies.first;
    while (eNode)
    {
        rayEnemy_t* e = eNode->val;
        node_t* next  = eNode->next;

        // If this is Doria, delete her
        if (OBJ_ENEMY_NPC_DORIA == e->c.type)
        {
            heap_caps_free(removeEntry(&ray->enemies, eNode));
        }

        eNode = next;
    }
}