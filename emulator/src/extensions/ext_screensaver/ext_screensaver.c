#include "ext_screensaver.h"

#include <stddef.h>

#include <esp_timer.h>
#include <hdw-nvs.h>
#include <stdio.h>

#include "macros.h"
#include "emu_args.h"
#include "ext_replay.h"
#include "ext_modes.h"
#include "hdw-btn_emu.h"

const char* screensaverScripts[] = {
    "attract_bigbug1.csv",
    "attract_bigbug2.csv",
};

static bool screensaverInit(emuArgs_t* args);
static void screensaverPreFrame(uint64_t frame);

static bool enable            = false;
static bool active            = false;
static int64_t lastWake       = 0;
static int64_t activationTime = 0;
static int screensaverIndex   = 0;
static int screensaverCount   = 0;
// 1 minute
static int64_t timeout    = 6000000; // 0;
static int64_t switchTime = 30000000;

const emuExtension_t screensaverEmuExtension = {
    .name            = "screensaver",
    .fnInitCb        = screensaverInit,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = screensaverPreFrame,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

static bool screensaverInit(emuArgs_t* args)
{
    if (/*args->screensaver ==*/true)
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