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
        cGroveShowMainProfile(gr, ply, gr->menuFont);
    }
}

void cGroveShowMainProfile(cGrove_t* gr, playerProfile_t ply, font_t fnt)
{
    // Profile background
    fillDisplayArea(0, 0, H_SCREEN_SIZE, V_SCREEN_SIZE, c000); // TEMP

    // Show username
    drawText(&fnt, c555, ply.username, 20, 20);
    // Show pronouns
    static char buffer[16] = "";
    switch(ply.pronouns){
        case HE_HIM:
            strcpy(buffer, cGrovePronounHe);
            break;
        case SHE_HER:
            strcpy(buffer, cGrovePronounShe);
            break;
        case OTHER:
            strcpy(buffer, cGrovePronounOther);
            break;     
        default:
             strcpy(buffer, cGrovePronounThey);
    }
    drawText(&fnt, c555, buffer, 20, 50);
    // Show mood
    wsg_t* moodDisplay;
    switch(ply.mood){
        case HAPPY:
            moodDisplay = &gr->mood_icons[2];
            break;
        case ANGRY:
            moodDisplay = &gr->mood_icons[1];
            break;
        case SAD:
            moodDisplay = &gr->mood_icons[3];
            break;
        case CONFUSED:
            moodDisplay = &gr->mood_icons[5];
            break; 
        case SURPRISED:
            moodDisplay = &gr->mood_icons[6];
            break;     
        case SICK:
            moodDisplay = &gr->mood_icons[3];
            break;     
        case NEUTRAL:
            moodDisplay = &gr->mood_icons[0];
            break;
        default:
             moodDisplay = &gr->mood_icons[0];
    }
    drawText(&fnt, c555, cGroveMoodColon, 20, 80);
    drawWsg(moodDisplay, 96, 72, false, false, 0);
    // Show Chowa
    // - Name, color, mood, etc
    drawText(&fnt, c555, "Chowa name", 20, 110);
    drawText(&gr->menuFont, c555, cGroveShowAwards, 20, 200);
    drawWsg(&gr->arrow, 180, 199, false, false, 90);

    // Display arrows to cycle through profiles
    if(gr->hasOnlineProfiles){
        drawWsg(&gr->arrow, 240, 12, false, false, 0);
        drawWsg(&gr->arrow, 240, 208, false, false, 180);
    }
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
        drawText(&gr->menuFont, c555, ply.username, 20, 190);
        //FIXME: Draw all awards etc
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