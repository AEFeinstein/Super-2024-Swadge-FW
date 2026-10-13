//==============================================================================
// Includes
//==============================================================================

#include "fill.h"
#include "ray_cutscene.h"
#include "esp_wifi.h"
#include "ray_script.h"
#include "ray_pause.h"
#include "cutscene.h"

#include "ray_cutscene_npc_barry_the_fairy.h"
#include "ray_cutscene_npc_cho.h"
#include "ray_cutscene_npc_doria.h"
#include "ray_cutscene_npc_ember_demon.h"
#include "ray_cutscene_npc_fairy_1.h"
#include "ray_cutscene_npc_fairy_2.h"
#include "ray_cutscene_npc_fairy_3.h"
#include "ray_cutscene_npc_fairy_4.h"
#include "ray_cutscene_npc_fairy_5.h"
#include "ray_cutscene_npc_fairy_6.h"
#include "ray_cutscene_npc_fredward.h"
#include "ray_cutscene_npc_great_deejay_tree.h"
#include "ray_cutscene_npc_hyut.h"
#include "ray_cutscene_npc_king_donut.h"
#include "ray_cutscene_npc_loot.h"
#include "ray_cutscene_npc_mega_pulse.h"
#include "ray_cutscene_npc_the_mayor.h"
#include "ray_cutscene_npc_trash_man.h"

static void rayCutsceneCb(void);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Show a cutscene. The three non-ray arguments are mutually exclusive
 *
 * @param ray The entire game state
 * @param type The NPC type for this cutscene, or EMPTY
 * @param sceneID The scene ID for this cutscene, or -1
 * @param text The text to show for this cutscene, or NULL
 */
void rayShowCutscene(ray_t* ray, rayMapCellType_t type, int32_t sceneID, char* text)
{
    // Pause music and switch to the cutscene
    globalMidiPlayerPauseAll();
    raySwitchToScreen(RAY_CUTSCENE);

    // Initialize the cutscene
    ray->cutscene = initCutscene(rayCutsceneCb, CUTSCENE_NEXT_0_WSG, 0);

    // Always load these two by default
    addCutsceneStyle(ray->cutscene, c000, TQ_TOMI_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Tomi", 1, false, true, true);
    addCutsceneStyle(ray->cutscene, c000, TQ_HYUT_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Hyut", 1, false, true, true);

    // Initialize the cutscene based on NPC or scene
    if (EMPTY != type)
    {
        switch (type)
        {
            case OBJ_ENEMY_NPC_FAIRY_1:
            {
                initNpcCutsceneFairy1(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_2:
            {
                initNpcCutsceneFairy2(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_3:
            {
                initNpcCutsceneFairy3(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_4:
            {
                initNpcCutsceneFairy4(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_5:
            {
                initNpcCutsceneFairy5(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FAIRY_6:
            {
                initNpcCutsceneFairy6(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_BARRY_THE_FAIRY:
            {
                initNpcCutsceneBarryTheFairy(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_HYUT:
            {
                initNpcCutsceneHyut(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_GREAT_DEEJAY_TREE:
            {
                initNpcCutsceneGreatDeejayTree(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_LOOT:
            {
                initNpcCutsceneLoot(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_DORIA:
            {
                initNpcCutsceneDoria(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_KING_DONUT:
            {
                initNpcCutsceneKingDonut(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_MEGA_PULSE:
            {
                initNpcCutsceneMegaPulse(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_CHO:
            {
                initNpcCutsceneCho(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_TRASH_MAN:
            {
                initNpcCutsceneTrashMan(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_EMBER_DEMON:
            {
                initNpcCutsceneEmberDemon(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_THE_MAYOR:
            {
                initNpcCutsceneTheMayor(ray->cutscene);
                break;
            }
            case OBJ_ENEMY_NPC_FREDWARD:
            {
                initNpcCutsceneFredward(ray->cutscene);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if (-1 != sceneID)
    {
        // TODO map out scenes
    }
    else if (NULL != text)
    {
        // Blank style for signposts and such
        addCutsceneStyle(ray->cutscene, c000, 0, CUTSCENE_TEXTBOX_WSG, "", 0, false, false, true);
        // Add the single line
        addCutsceneLine(ray->cutscene, 2, text, false, 0, NULL);
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
    if (ray->cutsceneDone)
    {
        // Switch back to the game. This will free memory
        ray->cutsceneDone = false;
        raySwitchToScreen(RAY_GAME);
        globalMidiPlayerResumeAll();
    }
    else
    {
        // Always update the cutscene
        updateCutscene(ray->cutscene, ray->cutsceneBtnState);

        // Safety check to not get stuck in a blank cutscene
        if (ray->cutscene->lines->first == NULL)
        {
            rayCutsceneCb();
        }
        else
        {
            drawCutscene(ray->cutscene, ray->renderer->menuFont);
        }
    }
}
