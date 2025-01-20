#include "ext_screensaver.h"

#include <stddef.h>

#include <esp_timer.h>
#include <hdw-nvs.h>
#include <stdio.h>
#include <hdw-nvs_emu.h>

#include "macros.h"
#include "emu_args.h"
#include "ext_replay.h"
#include "ext_modes.h"
#include "hdw-btn_emu.h"
#include "rawdraw_sf.h"

const char* screensaverScripts[] = {
    "attract_magfest.csv", "attract_song1.csv", "attract_sh2.csv",    "attract_bigbug1.csv", "attract_bigbug2.csv",
    "attract_uttt1.csv",   "attract_pango.csv", "attract_2048_3.csv", "attract_chowa.csv",
};

static bool screensaverInit(emuArgs_t* args);
static void screensaverPreFrame(uint64_t frame);
static void screensaverRender(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes);

static bool enable            = false;
static bool active            = false;
static int64_t lastWake       = 0;
static int64_t activationTime = 0;
static int screensaverIndex   = 0;
static int screensaverCount   = 0;
// 1 minute
static int64_t timeout = 60000000;
// 30 seconds
static int64_t switchTime = 30000000;
// 1.5 seconds
static int64_t blinkTime         = 1500000;
static const int64_t blinkOnTime = 750000;

const emuExtension_t screensaverEmuExtension = {
    .name            = "screensaver",
    .fnInitCb        = screensaverInit,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = screensaverPreFrame,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = screensaverRender,
};

static bool screensaverInit(emuArgs_t* args)
{
    if (args->screensaver)
    {
        // Don't immediately start the screensaver next frame
        lastWake = esp_timer_get_time();

        screensaverCount = ARRAY_SIZE(screensaverScripts);

        enable = true;
        active = false;
        return true;
    }

    enable = false;
    active = false;
    return false;
}

static void screensaverPreFrame(uint64_t frame)
{
    int64_t now = esp_timer_get_time();
    if (!active && now - timeout >= lastWake)
    {
        if (!emulatorGetKeyState())
        {
            printf("Screensaver starting!\n");
            // Activate!
            emuScreensaverActivate();
        }
    }
    else if (active && now - switchTime >= activationTime)
    {
        // Move to the next screensaver in the list
        emuScreensaverNext();
    }
}

static void screensaverRender(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes)
{
    if (active && ((esp_timer_get_time() % blinkTime) <= blinkOnTime))
    {
        const char* str = "FREE DEMO PLAY";
        int w, h;
        CNFGGetTextExtents(str, &w, &h, 10);
        CNFGPenX = (winW - w) / 2;
        CNFGPenY = (winH - h) / 4;

        CNFGColor(0xFFFFFFFF);
        CNFGSetLineWidth(8);
        CNFGDrawText(str, 10);

        CNFGColor(0xFF0000FF);
        CNFGSetLineWidth(6);
        CNFGDrawText(str, 10);
    }
}

void emuScreensaverActivate(void)
{
    if (!active)
    {
        if (screensaverCount > 0)
        {
            active         = true;
            activationTime = esp_timer_get_time();
            startPlayback(screensaverScripts[screensaverIndex]);
            screensaverIndex = (screensaverIndex + 1) % screensaverCount;
        }
    }
}

void emuScreensaverDeactivate(void)
{
    if (active)
    {
        // Is this right??
        startPlayback(NULL);
        active = false;

        emuInjectNvsClearAll();
        emulatorSetSwadgeModeByName("Main Menu");
    }
}

void emuScreensaverWake(void)
{
    lastWake = esp_timer_get_time();

    if (active)
    {
        emuScreensaverDeactivate();
    }
}

void emuScreensaverNext(void)
{
    active = false;
    emuScreensaverActivate();
}