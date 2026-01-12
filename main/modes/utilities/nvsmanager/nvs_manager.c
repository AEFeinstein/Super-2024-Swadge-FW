/*
 * nvs_manager.c
 *
 *  Created on: 3 Dec 2022
 *      Author: bryce and dylwhich
 */

/*==============================================================================
 * Includes
 *============================================================================*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

//#include "bresenham.h"
#include "embeddedNf.h"
#include "embeddedOut.h"
#include "esp_timer.h"
#include "hdw-led.h"
#include "linked_list.h"
#include "menuManiaRenderer.h"
#include "mainMenu.h"
#include "factoryTest.h"
#include "settingsManager.h"

#include "nvs_manager.h"

/*==============================================================================
 * Defines
 *============================================================================*/

#define CORNER_OFFSET         14
//#define TOP_TEXT_X_MARGIN     CORNER_OFFSET / 2
#define SUMMARY_LINE_BREAK_Y  8
#define LINE_BREAK_Y          4
#define MAX_INT_STRING_LENGTH 21
#define ENTRIES_BUF_SIZE      MAX_INT_STRING_LENGTH + 8

#ifndef NVS_ENTRY_BYTES
#define NVS_ENTRY_BYTES 32
#endif

/// Helper macro to return an integer clamped within a range (MIN to MAX)
//#define CLAMP(X, MIN, MAX) ( ((X) > (MAX)) ? (MAX) : ( ((X) < (MIN)) ? (MIN) : (X)) )
/// Helper macro to return the highest of two integers
//#define MAX(X, Y) ( ((X) > (Y)) ? (X) : (Y) )
//#define lengthof(x) (sizeof(x) / sizeof(x[0]))

/*==============================================================================
 * Enums
 *============================================================================*/


/*==============================================================================
 * Structs
 *============================================================================*/

/// @brief Defines each separate screen in the NVS manager mode
typedef enum
{
    // Top menu
    NVS_MENU,
    // Summary of used, free, and total entries in NVS
    NVS_SUMMARY,
    // Warn users about the potential dangers of managing keys
    NVS_WARNING,
    // Manage key/value pairs in NVS
    NVS_MANAGE_DATA,
    // Manage a specific key/value pair
    NVS_MANAGE_KEY,
} nvsScreen_t;

/// @brief Defines each action the user can take while viewing a key/value pair in NVS manager
typedef enum
{
    // Erase the key
    NVS_ACTION_ERASE,
    // Send the key to another Swadge
    //NVS_ACTION_SEND,
    // Go back (this will always be the last option, so it can be used as a maximum value)
    NVS_ACTION_BACK,
} nvsManageKeyAction_t;

/*==============================================================================
 * Variables
 *============================================================================*/

// The swadge mode
swadgeMode_t modeNvsManager =
{
    .modeName                 = "Save Data Mgr",
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = nvsManagerEnterMode,
    .fnExitMode               = nvsManagerExitMode,
    .fnMainLoop               = nvsManagerMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = nvsManagerBackgroundDrawCb,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

// The state data
typedef struct
{
    // Display, fonts, wsgs
    font_t ibm_vga8;
    font_t radiostars;
    font_t mm;
    wsg_t ibm_vga8_arrow;

    // Menu
    menu_t* menu;
    uint16_t topLevelPos;
    uint16_t manageDataPos;
    nvsManageKeyAction_t manageKeyAction;
    bool lockManageKeyAction;
    // The screen within NVS manager that the user is in
    nvsScreen_t screen;
    bool eraseDataSelected;
    bool eraseDataConfirm;
    bool warningAccepted;

    // Paginated text cache
    list_t pages;
    node_t* curPage;
    uint32_t curPageNum;
    uint16_t loadedRow;
    // Just points to blobStr or numStr
    const char* pageStr;

    ///////////////////
    // Cached NVS info
    ///////////////////

    // General NVS info
    nvs_stats_t nvsStats;
    nvs_entry_info_t* nvsEntryInfos;
    size_t nvsNumEntryInfos;

    // Per-key NVS info
    char* blobStr;
    char numStr[MAX_INT_STRING_LENGTH];
    size_t keyUsedEntries;
} nvsManager_t;

nvsManager_t* nvsManager;

/*==============================================================================
 * Const Variables
 *============================================================================*/

// Top menu
const char str_summary[] = "Summary";
const char str_manage_data[] = "Manage Data";
const char str_factory_reset[] = "Factory Reset";
const char str_confirm_no[] = "Confirm: No!";
const char str_confirm_yes[] = "Confirm: Yes";
const char str_back[] = "Back";
const char str_exit[] = "Exit";

// Summary
const paletteColor_t color_summary_text = c555;
const paletteColor_t color_summary_h_rule = c222;
const paletteColor_t color_summary_used = c134;
const paletteColor_t color_summary_free = c333;
const char str_non_volatile_storage[] = "Non-Volatile Storage (nvs:)";
const char str_nvs[] = "NVS";
const char str_type[] = "Type:";
const char str_local_flash_part[] = "Local Flash Partition";
const char str_file_system[] = "File system:";
const char str_used_space[] = "Used space:";
const char str_namespaces[] = "Namespaces:";
const char str_free_space[] = "Free space:";
const char str_capacity[] = "Capacity:";
const char str_1_entry[] = "1 entry";
const char str_entries_format[] = "%zu entries";

// Warning
const paletteColor_t color_warning_bg = c200;
const paletteColor_t color_warning_text = c555;
const char str_warning[] = "WARNING:";
const char str_warning_text[] = "Modifying, erasing, or receiving individual key/value pairs may cause certain modes or features of your Swadge to become unusable until a Factory Reset is performed, or may result in loss of save data.\n\n\nIf you acknowledge these risks and wish to proceed anyway, press Select.\n\n Press any other button to return.";

// Manage key
// Menu border colors: c112, c211, c021, c221, c102, c210,
const paletteColor_t color_key = c335;
const paletteColor_t color_namespace = c533;
const paletteColor_t color_type = c354;
const paletteColor_t color_used_entries = c553;
const paletteColor_t color_value = c435;
//c543
const paletteColor_t color_error = c511;
const char str_key[] = "Key: ";
const char str_namespace[] = "Namespace: ";
const char str_unknown[] = "Unknown";
const char str_read_failed[] = "Error: Failed to read data";
const char str_unknown_type[] = "Error: Unknown type";
const char str_erase[] = "Erase";
//const char str_send[] = "Send";
const char str_hex_format[] = "0x%x";
//const char str_u_dec_format[] = "%u";
const char str_i_dec_format[] = "%d";
const char str_page_format[] = "Page %u";

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Initializer for nvsManager
 */
void  nvsManagerEnterMode(void)
{
    // Allocate zero'd memory for the mode
    nvsManager = calloc(1, sizeof(nvsManager_t));

    loadFont(IBM_VGA_8_FONT, &nvsManager->ibm_vga8, true);
    loadFont(RADIOSTARS_FONT, &nvsManager->radiostars, true);
    loadFont(MM_FONT, &nvsManager->mm, true);

    loadWsg(ARROW_10_WSG, &nvsManager->ibm_vga8_arrow, true);

    nvsManager->loadedRow = UINT16_MAX;

    // TODO: handle errors
    nvsManagerReadAllNvsEntryInfos();

    // Initialize the menu
    nvsManager->menu = initMenu(modeNvsManager.modeName, nvsManagerTopLevelCb);
    nvsManagerSetUpTopMenu(true);
}

/**
 * Called when nvsManager is exited
 */
void  nvsManagerExitMode(void)
{
    deinitMenu(nvsManager->menu);

    freeFont(&nvsManager->ibm_vga8);
    freeFont(&nvsManager->radiostars);
    freeFont(&nvsManager->mm);

    freeWsg(&nvsManager->ibm_vga8_arrow);

    if(nvsManager->nvsEntryInfos != NULL)
    {
        free(nvsManager->nvsEntryInfos);
    }

    if (nvsManager->blobStr != NULL)
    {
        free(nvsManager->blobStr);
        nvsManager->blobStr = NULL;
    }

    clear(&nvsManager->pages);

    free(nvsManager);
}

/**
 * @brief Button callback function, plays notes and switches parameters
 *
 * @param evt The button event that occurred
 */
void  nvsManagerButtonCallback(buttonEvt_t* evt)
{
    switch (nvsManager->screen)
    {
        case NVS_MENU:
        {
            if (evt->down)
            {
                const char* selectedOption = nvsManager->menu->items[nvsManager->menu->currentItem];

                switch (evt->button)
                {
                    case PB_LEFT:
                    case PB_RIGHT:
                    {
                        if (str_confirm_no == selectedOption)
                        {
                            nvsManager->eraseDataConfirm = true;
                            nvsManagerSetUpTopMenu(false);
                        }
                        else if (str_confirm_yes == selectedOption)
                        {
                            nvsManager->eraseDataConfirm = false;
                            nvsManagerSetUpTopMenu(false);
                        }

                        break;
                    }
                    case PB_B:
                    {
                        if (nvsManager->eraseDataSelected)
                        {
                            nvsManager->eraseDataSelected = false;
                            // This is done later in the function
                            // nvsManager->eraseDataConfirm = false;
                            nvsManagerSetUpTopMenu(false);
                        }

                        break;
                    }
                    default:
                    {
                        menuButton(nvsManager->menu, *evt);
                        selectedOption = nvsManager->menu->items[nvsManager->menu->currentItem];
                        break;
                    }
                }

                if (nvsManager->eraseDataSelected && str_confirm_no != selectedOption &&
                    str_confirm_yes != selectedOption)
                {
                    // If the confirm-erase option is not selected, reset eraseDataConfirm and redraw the menu
                    nvsManager->topLevelPos = nvsManager->menu->currentItem;
                    nvsManager->eraseDataSelected = false;
                    nvsManager->eraseDataConfirm = false;
                    nvsManagerSetUpTopMenu(false);
                }
            }

            break;
        }
        case NVS_SUMMARY:
        {
            if(evt->down && evt->button == PB_B)
            {
                nvsManagerSetUpTopMenu(false);
            }

            break;
        }
        case NVS_WARNING:
        {
            if(evt->down)
            {
                switch(evt->button)
                {
                    case PB_SELECT:
                    {
                        nvsManager->warningAccepted = true;
                        nvsManagerSetUpManageDataMenu(true);
                        break;
                    }
                    case PB_B:
                    default:
                    {
                        nvsManager->screen = NVS_MENU;
                        break;
                    }
                }
            }

            break;
        }
        case NVS_MANAGE_DATA:
        {
            if(evt->down)
            {
                if(evt->button == PB_B)
                {
                    nvsManagerSetUpTopMenu(false);
                    break;
                }

                menuButton(nvsManager->menu, *evt);
            }

            break;
        }
        case NVS_MANAGE_KEY:
        {
            if(evt->down)
            {
                switch (evt->button)
                {
                    case PB_B:
                    {
                        switch(nvsManager->manageKeyAction)
                        {
                            case NVS_ACTION_ERASE:
                            {
                                if (nvsManager->eraseDataSelected)
                                {
                                    nvsManager->eraseDataSelected = false;
                                    nvsManager->eraseDataConfirm = false;
                                    break;
                                }

                                // Intentional fallthrough
                            }
                            case NVS_ACTION_BACK:
                            default:
                            {
                                nvsManagerSetUpManageDataMenu(false);
                            }
                        }
                        break;
                    }
                    case PB_UP:
                    {
                        if (nvsManager->curPage != NULL && nvsManager->curPage->prev != NULL)
                        {
                            nvsManager->curPage = nvsManager->curPage->prev;
                            nvsManager->curPageNum--;
                        }
                        break;
                    }
                    case PB_DOWN:
                    {
                        if (nvsManager->curPage != NULL && nvsManager->curPage->next != NULL)
                        {
                            nvsManager->curPage = nvsManager->curPage->next;
                            nvsManager->curPageNum++;
                        }
                        break;
                    }
                    case PB_LEFT:
                    case PB_RIGHT:
                    {
                        switch(nvsManager->manageKeyAction)
                        {
                            case NVS_ACTION_ERASE:
                            {
                                if(nvsManager->eraseDataConfirm)
                                {
                                    nvsManager->eraseDataConfirm = false;
                                    break;
                                }
                                else if(nvsManager->eraseDataSelected)
                                {
                                    nvsManager->eraseDataConfirm = true;
                                    break;
                                }

                                // Intentional fallthrough
                            }
                            case NVS_ACTION_BACK:
                            default:
                            {
                                if(nvsManager->lockManageKeyAction)
                                {
                                    break;
                                }

                                if(evt->button == PB_LEFT)
                                {
                                    nvsManager->manageKeyAction = (nvsManager->manageKeyAction + 1) % NVS_ACTION_BACK;
                                }
                                else // button == RIGHT
                                {
                                    if(nvsManager->manageKeyAction == 0)
                                    {
                                        nvsManager->manageKeyAction = NVS_ACTION_BACK;
                                    }
                                    else
                                    {
                                        nvsManager->manageKeyAction--;
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                    case PB_A:
                    {
                        switch(nvsManager->manageKeyAction)
                        {
                            case NVS_ACTION_ERASE:
                            {
                                if(!nvsManager->eraseDataSelected)
                                {
                                    nvsManager->eraseDataSelected = true;
                                }
                                else
                                {
                                    if(nvsManager->eraseDataConfirm)
                                    {
                                        eraseNvsKey(nvsManager->nvsEntryInfos[nvsManager->loadedRow].key);
                                        nvsManager->eraseDataSelected = false;
                                        nvsManager->eraseDataConfirm = false;
                                        // TODO: handle errors
                                        nvsManagerReadAllNvsEntryInfos();
                                        nvsManagerSetUpManageDataMenu(false);
                                    }
                                    else
                                    {
                                        nvsManager->eraseDataSelected = false;
                                    }
                                }
                                break;
                            }
                            // case NVS_ACTION_SEND:
                            // {
                            //     break;
                            // }
                            case NVS_ACTION_BACK:
                            {
                                nvsManagerSetUpManageDataMenu(false);
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

/**
 * Update the display by drawing the current state of affairs
 */
void  nvsManagerMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        nvsManagerButtonCallback(&evt);
    }
    
    switch(nvsManager->screen)
    {
        case NVS_MENU:
        case NVS_MANAGE_DATA:
        {
            // Draw the menu
            drawMegaMenu(nvsManager->menu);
            break;
        }
        case NVS_SUMMARY:
        {
            // Done in background draw callback
            //clearPxTft();

            led_t leds[CONFIG_NUM_LEDS];
            memset(leds, 0, CONFIG_NUM_LEDS * sizeof(led_t));
            setLeds(leds, CONFIG_NUM_LEDS);

            char buf[ENTRIES_BUF_SIZE];

            // Partition name
            int16_t yOff = CORNER_OFFSET;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_non_volatile_storage, CORNER_OFFSET, yOff);

            yOff += nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y + 1;
            drawLine(CORNER_OFFSET, yOff, TFT_WIDTH - CORNER_OFFSET, yOff, color_summary_h_rule, 0);

            // Partition type
            yOff += SUMMARY_LINE_BREAK_Y + 1;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_type, CORNER_OFFSET, yOff);
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_local_flash_part, TFT_WIDTH - textWidth(&nvsManager->ibm_vga8, str_local_flash_part) - CORNER_OFFSET, yOff);

            // Partition file system
            yOff += nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_file_system, CORNER_OFFSET, yOff);
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_nvs, TFT_WIDTH - textWidth(&nvsManager->ibm_vga8, str_nvs) - CORNER_OFFSET, yOff);

            yOff += nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y + 1;
            drawLine(CORNER_OFFSET, yOff, TFT_WIDTH - CORNER_OFFSET, yOff, color_summary_h_rule, 0);

            // Used space
            yOff += SUMMARY_LINE_BREAK_Y + 1;
            fillDisplayArea(CORNER_OFFSET, yOff, CORNER_OFFSET + nvsManager->ibm_vga8.height, yOff + nvsManager->ibm_vga8.height, color_summary_used);
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_used_space, CORNER_OFFSET + nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y, yOff);
            snprintf(buf, ENTRIES_BUF_SIZE, str_entries_format, nvsManager->nvsStats.used_entries);
            drawText(&nvsManager->ibm_vga8, color_summary_text, nvsManager->nvsStats.used_entries == 1 ? str_1_entry : buf, TFT_WIDTH - textWidth(&nvsManager->ibm_vga8, buf) - CORNER_OFFSET, yOff);

            // Namespaces
            yOff += nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_namespaces, CORNER_OFFSET + nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y, yOff);
            snprintf(buf, ENTRIES_BUF_SIZE, str_entries_format, nvsManager->nvsStats.namespace_count);
            int16_t tWidth = textWidth(&nvsManager->ibm_vga8, buf);
            snprintf(buf, ENTRIES_BUF_SIZE, "%zu", nvsManager->nvsStats.namespace_count);
            drawText(&nvsManager->ibm_vga8, color_summary_text, buf, TFT_WIDTH - tWidth - CORNER_OFFSET, yOff);

            // Free space
            yOff += nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y;
            fillDisplayArea(CORNER_OFFSET, yOff, CORNER_OFFSET + nvsManager->ibm_vga8.height, yOff + nvsManager->ibm_vga8.height, color_summary_free);
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_free_space, CORNER_OFFSET + nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y, yOff);
            snprintf(buf, ENTRIES_BUF_SIZE, str_entries_format, nvsManager->nvsStats.free_entries);
            drawText(&nvsManager->ibm_vga8, color_summary_text, nvsManager->nvsStats.free_entries == 1 ? str_1_entry : buf, TFT_WIDTH - textWidth(&nvsManager->ibm_vga8, buf) - CORNER_OFFSET, yOff);

            yOff += nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y + 1;
            drawLine(CORNER_OFFSET, yOff, TFT_WIDTH - CORNER_OFFSET, yOff, color_summary_h_rule, 0);

            // Capacity
            yOff += SUMMARY_LINE_BREAK_Y + 1;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_capacity, CORNER_OFFSET + nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y, yOff);
            snprintf(buf, ENTRIES_BUF_SIZE, str_entries_format, nvsManager->nvsStats.total_entries);
            drawText(&nvsManager->ibm_vga8, color_summary_text, buf, TFT_WIDTH - textWidth(&nvsManager->ibm_vga8, buf) - CORNER_OFFSET, yOff);

            yOff += nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y;
            int16_t xStart = CORNER_OFFSET + nvsManager->ibm_vga8.height + SUMMARY_LINE_BREAK_Y;
            int16_t xEnd = TFT_WIDTH - CORNER_OFFSET - nvsManager->ibm_vga8.height - SUMMARY_LINE_BREAK_Y;
            fillDisplayArea(xStart, yOff, xStart + roundf((float_t)nvsManager->nvsStats.used_entries / nvsManager->nvsStats.total_entries * (xEnd - xStart)), yOff + nvsManager->ibm_vga8.height, color_summary_used);
            fillDisplayArea(xEnd - roundf((float_t)nvsManager->nvsStats.free_entries / nvsManager->nvsStats.total_entries * (xEnd - xStart)), yOff, xEnd, yOff + nvsManager->ibm_vga8.height, color_summary_free);

            break;
        }
        case NVS_WARNING:
        {
            led_t leds[CONFIG_NUM_LEDS];
            for(uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                leds[i].r = 0xFF;
                leds[i].g = 0x00;
                leds[i].b = 0x00;
            }
            setLeds(leds, CONFIG_NUM_LEDS);

            // Done in background draw callback
            //fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, color_warning_bg);

            int16_t yOff = CORNER_OFFSET;
            drawText(&nvsManager->mm, color_warning_text, str_warning, (TFT_WIDTH - textWidth(&nvsManager->mm, str_warning)) / 2, yOff);

            yOff += nvsManager->mm.height + nvsManager->ibm_vga8.height * 2 + 3;
            int16_t xOff = CORNER_OFFSET;
            drawTextWordWrap(&nvsManager->ibm_vga8, color_warning_text, str_warning_text, &xOff, &yOff, TFT_WIDTH - CORNER_OFFSET, TFT_HEIGHT - CORNER_OFFSET);
            break;
        }
        case NVS_MANAGE_KEY:
        {
            // Done in background draw callback
            //clearPxTft();

            led_t leds[CONFIG_NUM_LEDS];
            memset(leds, 0, CONFIG_NUM_LEDS * sizeof(led_t));
            setLeds(leds, CONFIG_NUM_LEDS);

            nvs_entry_info_t entryInfo = nvsManager->nvsEntryInfos[nvsManager->menu->currentItem];

            // Key
            int16_t yOff = CORNER_OFFSET;
            const int16_t afterLongestLabel = CORNER_OFFSET + textWidth(&nvsManager->ibm_vga8, str_used_space) + textWidth(&nvsManager->ibm_vga8, " ") + 1;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_key, CORNER_OFFSET, yOff);
            drawText(&nvsManager->ibm_vga8, color_key, entryInfo.key, afterLongestLabel, yOff);

            // Namespace
            yOff +=  nvsManager->ibm_vga8.height + LINE_BREAK_Y;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_namespace, CORNER_OFFSET, yOff);
            drawText(&nvsManager->ibm_vga8, color_namespace, entryInfo.namespace_name, afterLongestLabel, yOff);

            // Type
            yOff +=  nvsManager->ibm_vga8.height + LINE_BREAK_Y;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_type, CORNER_OFFSET, yOff);
            const char* typeName = getNvsTypeName(entryInfo.type);
            char typeAsHex[5];
            if(typeName[0] == '\0')
            {
                snprintf(typeAsHex, 5, str_hex_format, entryInfo.type);
            }
            drawText(&nvsManager->ibm_vga8, color_type, typeName[0] == '\0' ? typeAsHex : typeName, afterLongestLabel, yOff);

            // Prepare for getting the value, and pagination
            bool readSuccess = false;
            if (nvsManager->loadedRow != nvsManager->menu->currentItem)
            {

                nvsManager->curPage = NULL;
                nvsManager->curPageNum = 0;
                clear(&nvsManager->pages);

                if (nvsManager->blobStr != NULL)
                {
                    free(nvsManager->blobStr);
                    nvsManager->blobStr = NULL;
                }

                nvsManager->loadedRow = nvsManager->menu->currentItem;
                nvsManager->keyUsedEntries = 0;

                // Get the value
                bool foundType = true;

                switch(entryInfo.type)
                {
                    case NVS_TYPE_I32:
                    {
                        int32_t val;
                        readSuccess = readNvs32(entryInfo.key, &val);
                        if(readSuccess)
                        {
                            snprintf(nvsManager->numStr, MAX_INT_STRING_LENGTH, str_i_dec_format, val);
                            nvsManager->keyUsedEntries = 1;
                        }
                        break;
                    }
                    case NVS_TYPE_BLOB:
                    {
                        size_t length;
                        readSuccess = readNvsBlob(entryInfo.key, NULL, &length);
                        if(readSuccess)
                        {
                            char* blob = calloc(1, length);
                            readSuccess = readNvsBlob(entryInfo.key, blob, &length);
                            if(readSuccess)
                            {
                                nvsManager->blobStr = blobToStrWithPrefix(blob, length);
                                /**
                                 * When the ESP32 is storing blobs, it uses 1 entry to index chunks,
                                 * 1 entry per chunk, then 1 entry for every 32 bytes of data, rounding up.
                                 *
                                 * I don't know how to find out how many chunks the ESP32 would split
                                 * certain length blobs into, so for now I'm assuming 1 chunk per blob.
                                 *
                                 * TODO: find a way to get number of entries or chunks in the blob
                                 */
                                nvsManager->keyUsedEntries = 2 + ceil((float) length / NVS_ENTRY_BYTES);
                            }
                            free(blob);
                        }
                        break;
                    }
                    case NVS_TYPE_U8:
                    case NVS_TYPE_I8:
                    case NVS_TYPE_U16:
                    case NVS_TYPE_I16:
                    case NVS_TYPE_U32:
                    case NVS_TYPE_U64:
                    case NVS_TYPE_I64:
                    case NVS_TYPE_STR:
                    case NVS_TYPE_ANY:
                    default:
                    {
                        foundType = false;
                        break;
                    }
                }

                if(foundType)
                {
                    if(readSuccess)
                    {
                        switch(entryInfo.type)
                        {
                            case NVS_TYPE_U8:
                            case NVS_TYPE_I8:
                            case NVS_TYPE_U16:
                            case NVS_TYPE_I16:
                            case NVS_TYPE_U32:
                            case NVS_TYPE_I32:
                            case NVS_TYPE_U64:
                            case NVS_TYPE_I64:
                            {
                                nvsManager->pageStr = nvsManager->numStr;
                                break;
                            }

                            case NVS_TYPE_BLOB:
                            {
                                nvsManager->pageStr = nvsManager->blobStr;
                                break;
                            }

                            case NVS_TYPE_STR:
                            case NVS_TYPE_ANY:
                            default:
                            {
                                nvsManager->pageStr = str_unknown_type;
                                break;
                            }
                        }
                    }
                    else
                    {
                        nvsManager->pageStr = str_read_failed;
                    }
                }
                else
                {
                    nvsManager->pageStr = str_unknown_type;
                }
            }

            // Used space
            yOff += nvsManager->ibm_vga8.height + LINE_BREAK_Y;
            drawText(&nvsManager->ibm_vga8, color_summary_text, str_used_space, CORNER_OFFSET, yOff);
            if(nvsManager->keyUsedEntries == 1)
            {
                drawText(&nvsManager->ibm_vga8, color_used_entries, str_1_entry, afterLongestLabel, yOff);
            }
            else if(nvsManager->keyUsedEntries > 1)
            {
                char buf[ENTRIES_BUF_SIZE];
                snprintf(buf, ENTRIES_BUF_SIZE, str_entries_format, nvsManager->keyUsedEntries);
                drawText(&nvsManager->ibm_vga8, color_used_entries, buf, afterLongestLabel, yOff);
            }
            else
            {
                drawText(&nvsManager->ibm_vga8, color_error, str_unknown, afterLongestLabel, yOff);
            }

            yOff += nvsManager->ibm_vga8.height + LINE_BREAK_Y + 1;
            drawLine(0, yOff, TFT_WIDTH, yOff, color_summary_h_rule, 0);

            //////////////////////////////////////////////
            // Value
            //////////////////////////////////////////////
            yOff += LINE_BREAK_Y + 1;
            int16_t xOff = CORNER_OFFSET;
            int16_t newYOff = TFT_HEIGHT - CORNER_OFFSET - nvsManager->ibm_vga8.height * 2 - LINE_BREAK_Y * 3 - 2;
            const char* nextText;
            if (nvsManager->curPage == NULL)
            {
                // Add the beginning of the text as the first page
                push(&nvsManager->pages, nvsManager->pageStr);
                nvsManager->curPage = nvsManager->pages.first;
                nvsManager->curPageNum = 1;
            }

            // Now, draw the text, which will always be in the current page
            nextText = drawTextWordWrap(&nvsManager->ibm_vga8, color_value, (const char*)(nvsManager->curPage->val),
                                                        &xOff, &yOff, TFT_WIDTH - CORNER_OFFSET, newYOff);

            if (nextText != NULL && nvsManager->curPage->next == NULL)
            {
                // Save the next page if it hasn't already been saved
                push(&nvsManager->pages, nextText);
            }

            // if(nextText != NULL)
            // {
            //     drawText(&nvsManager->ibm_vga8, color_value, "...", TFT_WIDTH - CORNER_OFFSET, yOff);
            // }

            // Pagination controls, page number
            yOff = newYOff + LINE_BREAK_Y;
            if(nvsManager->curPage->next != NULL || nvsManager->curPage->prev != NULL)
            {
                drawWsg(&nvsManager->ibm_vga8_arrow, CORNER_OFFSET, yOff, false, false, 0);
                char buf[ENTRIES_BUF_SIZE];
                snprintf(buf, ENTRIES_BUF_SIZE, str_page_format, nvsManager->curPageNum);
                drawText(&nvsManager->ibm_vga8, color_summary_text, buf, (TFT_WIDTH - textWidth(&nvsManager->ibm_vga8, buf)) / 2, yOff);
                drawWsg(&nvsManager->ibm_vga8_arrow, TFT_WIDTH - nvsManager->ibm_vga8_arrow.w - CORNER_OFFSET, yOff, false, true, 0);
            }

            yOff += nvsManager->ibm_vga8.height + LINE_BREAK_Y + 1;
            drawLine(0, yOff, TFT_WIDTH, yOff, color_summary_h_rule, 0);

            // Controls
            yOff += LINE_BREAK_Y + 1;
            // If the read failed or the key's namespace isn't the one we have access to, hide all actions except "Back"
            if(nvsManager->pageStr == str_read_failed || strcmp(entryInfo.namespace_name, NVS_NAMESPACE_NAME))
            {
                nvsManager->manageKeyAction = NVS_ACTION_BACK;
                nvsManager->lockManageKeyAction = true;
            }
            if(!nvsManager->lockManageKeyAction)
            {
                drawWsg(&nvsManager->ibm_vga8_arrow, CORNER_OFFSET, yOff, false, false, 270);
            }
            const char* actionStr;
            switch(nvsManager->manageKeyAction)
            {
                case NVS_ACTION_ERASE:
                {
                    if(nvsManager->eraseDataConfirm)
                    {
                        actionStr = str_confirm_yes;
                    }
                    else if(nvsManager->eraseDataSelected)
                    {
                        actionStr = str_confirm_no;
                    }
                    else
                    {
                        actionStr = str_erase;
                    }
                    break;
                }
                // case NVS_ACTION_SEND:
                // {
                //     actionStr = str_send;
                // }
                case NVS_ACTION_BACK:
                {
                    actionStr = str_back;
                    break;
                }
                default:
                {
                    actionStr = "";
                    break;
                }
            }
            drawText(&nvsManager->ibm_vga8, color_summary_text, actionStr, (TFT_WIDTH - textWidth(&nvsManager->ibm_vga8, actionStr)) / 2, yOff);
            if(!nvsManager->lockManageKeyAction)
            {
                drawWsg(&nvsManager->ibm_vga8_arrow, TFT_WIDTH - nvsManager->ibm_vga8_arrow.h - CORNER_OFFSET, yOff, false, false, 90);
            }

            break;
        }
    }
}

/**
 * @brief Draw a portion of the background when requested
 *
 * @param x The X offset to draw
 * @param y The Y offset to draw
 * @param w The width to draw
 * @param h The height to draw
 * @param up The current number of the update call
 * @param upNum The total number of update calls for this frame
 */
void nvsManagerBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    /*
    paletteColor_t* const src = nvsManager->backgroundWsg.px;
    paletteColor_t* dst       = getPxTftFramebuffer();
    for (int16_t row = y; row < y + h; row++)
    {
        memcpy(&dst[row * TFT_WIDTH + x], &src[row * TFT_WIDTH + x], w);
    }
    */
    
    switch(nvsManager->screen)
    {
        case NVS_MENU:
        case NVS_MANAGE_DATA:
        {
            return;
        }
        case NVS_SUMMARY:
        case NVS_MANAGE_KEY:
        {
            fillDisplayArea(x, y, x + w, y + h, c000);
            break;
        }
        case NVS_WARNING:
        {
            fillDisplayArea(x, y, x + w, y + h, color_warning_bg);
            break;
        }
}

/**
 * Set up the top level menu
 *
 * @param resetPos true to reset the position to 0, false to leave it where it is
 */
void nvsManagerSetUpTopMenu(bool resetPos)
{
    // Set up the menu
    resetMeleeMenu(nvsManager->menu, modeNvsManager.modeName, nvsManagerTopLevelCb);
    addSingleItemToMenu(nvsManager->menu, str_summary);
    addSingleItemToMenu(nvsManager->menu, str_manage_data);

    // Add the row for factory resetting the Swadge
    if (nvsManager->eraseDataSelected)
    {
        if (nvsManager->eraseDataConfirm)
        {
            addSingleItemToMenu(nvsManager->menu, str_confirm_yes);
        }
        else
        {
            addSingleItemToMenu(nvsManager->menu, str_confirm_no);
        }
    }
    else
    {
        addSingleItemToMenu(nvsManager->menu, str_factory_reset);
    }

    addSingleItemToMenu(nvsManager->menu, str_exit);

    // Set the position
    if(resetPos)
    {
        nvsManager->topLevelPos = 0;
    }
    nvsManager->menu->currentItem = nvsManager->topLevelPos;

    nvsManager->screen = NVS_MENU;
}

/**
 * Callback for the top level menu
 *
 * @param opt The menu option which was selected
 */
void nvsManagerTopLevelCb(const char* opt)
{
    // Save the position
    nvsManager->topLevelPos = nvsManager->menu->currentItem;

    // Handle the option
    if(str_summary == opt)
    {
        nvsManager->screen = NVS_SUMMARY;
    }
    else if(str_manage_data == opt)
    {
        if(!nvsManager->warningAccepted)
        {
            nvsManager->screen = NVS_WARNING;
        }
        else
        {
            nvsManagerSetUpManageDataMenu(true);
        }
    }
    else if(str_factory_reset == opt)
    {
        nvsManager->eraseDataSelected = true;
        nvsManagerSetUpTopMenu(false);
    }
    else if (str_confirm_yes == opt)
    {
        // If this succeeds, we shouldn't let someone back into the main menu, so switch to test mode immediately
        // If this fails, we have no idea what state NVS is in, so send them back to the main menu and pray
        if(eraseNvs())
        {
            switchToSwadgeMode(&factoryTestMode);
        }
        else
        {
            switchToSwadgeMode(&mainMenuMode);
        }
        nvsManager->eraseDataConfirm = false;
        nvsManager->eraseDataSelected = false;
    }
    else if (str_confirm_no == opt)
    {
        nvsManager->eraseDataSelected = false;
        nvsManager->eraseDataConfirm = false;
        nvsManagerSetUpTopMenu(false);
    }
    else if(str_exit == opt)
    {
        switchToSwadgeMode(&mainMenuMode);
    }
}

/**
 * Set up the data management menu
 *
 * @param resetPos true to reset the position to 0, false to leave it where it is
 */
void nvsManagerSetUpManageDataMenu(bool resetPos)
{
    // Set up the menu
    resetMeleeMenu(nvsManager->menu, str_manage_data, nvsManagerManageDataCb);

    for(size_t i = 0; i < nvsManager->nvsNumEntryInfos; i++)
    {
        if(nvsManager->nvsEntryInfos[i].key[0] != '\0')
        {
            addSingleItemToMenu(nvsManager->menu, nvsManager->nvsEntryInfos[i].key);
        }
    }

    addSingleItemToMenu(nvsManager->menu, str_back);

    // Set the position
    if(resetPos)
    {
        nvsManager->manageDataPos = 0;
    }
    nvsManager->menu->currentItem = nvsManager->manageDataPos;
    nvsManager->eraseDataSelected = false;
    nvsManager->eraseDataConfirm = false;

    nvsManager->screen = NVS_MANAGE_DATA;
}

/**
 * Callback for the data management menu
 *
 * @param opt The menu option which was selected
 */
void nvsManagerManageDataCb(const char* opt)
{
    // Save the position
    nvsManager->manageDataPos = nvsManager->menu->currentItem;

    // Handle the option
    if(str_back == opt)
    {
        nvsManagerSetUpTopMenu(false);
    }
    else
    {
        nvsManager->loadedRow = UINT16_MAX;
        nvsManager->manageKeyAction = 0;
        nvsManager->lockManageKeyAction = false;
        nvsManager->screen = NVS_MANAGE_KEY;
    }
}

bool nvsManagerReadAllNvsEntryInfos()
{
    // Save the existing number of allocated entry infos so we know if we can reuse the old allocation
    size_t oldNvsNumEntryInfos = nvsManager->nvsNumEntryInfos;

    // Get the number of entry infos so we can allocate the memory
    if(!readAllNvsEntryInfos(&nvsManager->nvsStats, NULL, &nvsManager->nvsNumEntryInfos))
    {
        return false;
    }

    // If nvsEntryInfos is already allocated and it's not going to be big enough, free it first
    if(nvsManager->nvsEntryInfos != NULL && oldNvsNumEntryInfos < nvsManager->nvsNumEntryInfos)
    {
        free(nvsManager->nvsEntryInfos);
        nvsManager->nvsEntryInfos = NULL;
    }

    // If we just freed nvsEntryInfos or it wasn't allocated to begin with, allocate it
    if(nvsManager->nvsEntryInfos == NULL)
    {
        nvsManager->nvsEntryInfos = calloc(nvsManager->nvsNumEntryInfos, sizeof(nvs_entry_info_t));
    }

    // Do the actual read of entry infos
    return readAllNvsEntryInfos(&nvsManager->nvsStats, &nvsManager->nvsEntryInfos, &nvsManager->nvsNumEntryInfos);
}

/**
 * @brief Convert a blob to a hex string, with "0x" prefix
 *
 * @param value The blob
 * @param length The length of the blob
 * @return char* An allocated hex string, must be free()'d when done
 */
char* blobToStrWithPrefix(const void * value, size_t length)
{
    const uint8_t * value8 = (const uint8_t *)value;
    char * blobStr = malloc((length * 2) + 3);
    size_t i = 0;
    sprintf(&blobStr[i*2], "%s", "0x");
    for(i++; i < length; i++)
    {
        sprintf(&blobStr[i*2], "%02X", value8[i]);
    }
    return blobStr;
}

const char* getNvsTypeName(nvs_type_t type)
{
    switch(type)
    {
#ifdef USING_MORE_THAN_I32_AND_BLOB
        case NVS_TYPE_U8:
        {
            return "8-bit unsigned integer";
        }
        case NVS_TYPE_I8:
        {
            return "8-bit signed integer";
        }
        case NVS_TYPE_U16:
        {
            return "16-bit unsigned integer";
        }
        case NVS_TYPE_I16:
        {
            return "16-bit signed integer";
        }
        case NVS_TYPE_U32:
        {
            return "32-bit unsigned integer";
        }
#endif
        case NVS_TYPE_I32:
        {
            return "32-bit signed integer";
        }
#ifdef USING_MORE_THAN_I32_AND_BLOB
        case NVS_TYPE_U64:
        {
            return "64-bit unsigned integer";
        }
        case NVS_TYPE_I64:
        {
            return "64-bit signed integer";
        }
        case NVS_TYPE_STR:
        {
            return "String";
        }
#endif
        case NVS_TYPE_BLOB:
        {
            return "Blob";
        }
        case NVS_TYPE_ANY:
        default:
        {
            return "";
        }
    }
}
