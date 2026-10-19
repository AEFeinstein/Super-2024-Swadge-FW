#include "ray_cutscene.h"
#include "ray_cutscene_npc_fredward.h"

/**
 * @brief Add a cutscene style for Fredward
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleFredward(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_FREDWARD_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Fredward", 1, true, true, true);
}

static void shopVisited(void)
{
    getRayState()->p.cutsceneViewed |= TQS_FREDWARD;
}

/**
 * @brief Initialize the cutscene for Fredward
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneFredward(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_FREDWARD (CS_STYLE_HYUT + 1)
    addCutsceneStyleFredward(cutscene);

    // TODO add more items to the shop
    bool allItemsPurchased = ray->p.i.haveBombs;

    // clang-format off

    if(ray->ps.firstShopVisit)
    {
        if(!ray->ps.itemPurchased)
        {
            addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Hello! I hope you take stock of our fine... um.. Stock!", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh wow, you got a lot of great stuff here!", false, TOMI_FIREDUP, NULL);
            addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Yep! Support local creators by spending your hard-earned m-Points! ", false, 0, shopVisited);
        }
        else
        {
            addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Thanks!", false, 0, NULL);
        }
    }
    else if(!allItemsPurchased)
    {
        if(!ray->ps.itemPurchased)
        {
            // On Subsequent visits to the Item Shop, regardless of chapter:
            addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Welcome back! Were my items helpful on your journey?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah! Super helpful in fact. What else ya got?", false, TOMI_CALM, NULL);
            addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Oh, all sorts of stuff. I'm trying to get a good nest egg going, there's a great lillypad that's supposed to be coming on the market soon.", false, 0, NULL);
        }
        else
        {
            // After purchase: 
            addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Awesome! Down payment, here I come!", false, 0, NULL);
        }
    }
    else
    {
        // If the player purchases everything in the shop:
        addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Ohmygosh! Ohmygosh! You bought it all!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sure did, Fredward. Now you can finally afford that down payment!", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Haha, are you kidding? With interest rates this high, getting a mortgage is for suckers right now. I'm doing the FIRE thing.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Financial Independence, Retire Early?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Oh, is THAT what the acronym means? I just thought it meant setting fire to all your possessions and living in a swamp somewhere.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Is that what you're going to do then?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_FREDWARD, "Maybe. Or maybe I'll just buy some new synths.", false, 0, NULL);
        // TODO ACHIEVEMENT UNLOCKED: "Bad Influence" - he's just gonna buy synths, isn't he.
    }
    // clang-format on
}
