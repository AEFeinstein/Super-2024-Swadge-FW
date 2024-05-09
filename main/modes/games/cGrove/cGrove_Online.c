/**
 * @file cGrove_Online.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief Online functionailty
 * @version 0.1
 * @date 2024-05-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

// Includes
//==============================================================================
#include "cGrove_Online.h"

// Online
// =============================================================================
void cGroveProfileMain(cGrove_t* gr){
    buttonEvt_t evt = {0};
    while(checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if ((evt.state & PB_UP) || (evt.state & PB_DOWN)){    // Switch profiles
                cGroveCustomSelectionWrap(evt, MAX_PREV_GUESTS + 1, &gr->customMenuSelect);
            } else if (evt.state & PB_A) {                         // Select profile to view in more detail
                gr->currState = SUBPROFILE;
            } else {
                gr->currState = MENU;
            }
        }
        // Get Profile 
        playerProfile_t ply;
        if (gr->customMenuSelect == 0) {
            ply = gr->player;
        } else {
            ply = gr->guests[(gr->customMenuSelect - 1)];
        }
        cGroveShowMainProfile(ply, gr->menuFont);
    }
}

void cGroveShowMainProfile(playerProfile_t ply, font_t fnt)
{
    // Profile background
    fillDisplayArea(0, 0, H_SCREEN_SIZE, V_SCREEN_SIZE, c111); // TEMP

    // Show username
    drawText(&fnt, c555, ply.username, 20, 20);
    // Show pronouns
    drawText(&fnt, c555, ply.username, 20, 50);
    // Show mood
    drawText(&fnt, c555, ply.username, 20, 80);
    // Show Chowa
    // - Name, color, mood, etc
    drawText(&fnt, c555, ply.username, 20, 110);

    // Display arrows to cycle through profiles
    // Grey out arrows at the top and bottom of list
}

void cGroveShowSubProfile(cGrove_t* gr)
{
    buttonEvt_t evt = {0};
    playerProfile_t ply;
    if (gr->customMenuSelect == 0) {
        ply = gr->player;
    } else {
        ply = gr->guests[(gr->customMenuSelect - 1)];
    }
    while(checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            gr->currState = PROFILE;
        }
        // Draw
        drawText(&gr->menuFont, c555, "GET PUNKED", 20, 140);
        drawText(&gr->menuFont, c555, ply.username, 20, 170);
        //FIXME: Draw all Chowa related info
    }
}

// Wireless functions
//==============================================================================
void cGroveToggleOnlineFeatures(cGrove_t* gr){
    gr->online = !gr->online;
    // TODO: Turn on and off online features
}

void cGroveEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // TODO:
}

void cGroveEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // TODO:
}