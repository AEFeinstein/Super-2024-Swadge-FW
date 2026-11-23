//==============================================================================
// Includes
//==============================================================================

#include "ray_turntable.h"
#include "ray_script.h"
#include "ray_tex_manager.h"

//==============================================================================
// Defines
//==============================================================================

#define TURNTABLE_X               8
#define KNOB_MARGIN_X             9
#define KNOB_MARGIN_Y             10
#define LABEL_RADIUS              11
#define WIGGLE_DEGREES            2
#define CATCHER_DEGREES           60
#define CATCHER_UNDERDRAW_DEGREES 10 ///< Underdraw a few degrees on either side so our collision checks feel more fair
#define CATCHER_THICKNESS         3

#define NOTE_SPAWN_MIN_US    300000
#define NOTE_SPAWN_MAX_US    800000
#define NOTE_SPAWN_DIST      70
#define NOTE_HIT_RADIUS      4
#define NOTE_SPEED_PX_PER_US (100.f / 1000000)
#define NOTE_HIT_DISPLAY_US  500000

#define SCORE_NEEDED         1000000
#define SCORE_PER_NOTE       100000
#define SCORE_DROP_PERIOD_US 30 ///< Time in us for score to drop by 1 point

#define SCORE_METER_TEXT_Y      194
#define SCORE_METER_TEXT_MARGIN 10
#define SCORE_METER_MARGIN      18
#define SCORE_METER_HEIGHT      23
#define SCORE_METER_OUTLINE     4

static const char* scoreMeterText = "Vibes";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the turntable minigame
 *
 * @param ray The entire game state
 */
void rayTurntableInit(ray_t* ray)
{
    loadWsg(TT_TURNTABLE_WSG, &ray->ts.wsg.turntable, true);
    loadWsg(TT_RECORD_GROOVES_WSG, &ray->ts.wsg.recordGrooves, true);
    loadWsg(TT_KNOBS_WSG, &ray->ts.wsg.knobs, true);
    loadWsg(TT_KNOBS_MIRRORED_WSG, &ray->ts.wsg.knobsMirrored, true);
    loadWsg(TT_NOTE_1_WSG, &ray->ts.wsg.notes[0], true);
    loadWsg(TT_NOTE_2_WSG, &ray->ts.wsg.notes[1], true);
    loadWsg(TT_NOTE_HIT_WSG, &ray->ts.wsg.noteHit, true);
    loadWsg(TT_METER_CORNER_WSG, &ray->ts.wsg.meterCorner, true);

    ray->ts.tables[0].catcherAngle = 180 - CATCHER_DEGREES / 2;
    ray->ts.tables[1].catcherAngle = 360 - CATCHER_DEGREES / 2;
    ray->ts.tables[0].wiggleAngle  = WIGGLE_DEGREES;
    ray->ts.tables[1].wiggleAngle  = WIGGLE_DEGREES;
}

/**
 * @brief Uninitialize the turntable minigame and unload assets
 *
 * @param ray The entire game state
 */
static void rayTurntableExit(ray_t* ray, bool success)
{
    freeWsg(&ray->ts.wsg.turntable);
    freeWsg(&ray->ts.wsg.recordGrooves);
    freeWsg(&ray->ts.wsg.knobs);
    freeWsg(&ray->ts.wsg.knobsMirrored);
    freeWsg(&ray->ts.wsg.notes[0]);
    freeWsg(&ray->ts.wsg.notes[1]);
    freeWsg(&ray->ts.wsg.noteHit);
    freeWsg(&ray->ts.wsg.meterCorner);

    if (success)
    {
        executeScriptEvent(ray, ray->ts.turntableScript, getTexByType(ray, OBJ_SCENERY_TURNTABLE));
    }
    raySwitchToScreen(RAY_GAME);
}

/**
 * @brief Check for button input while showing turntable and either show the next part of the turntable or return to
 * the game loop
 *
 * @param ray The entire game state
 */
void rayTurntableCheckButtons(ray_t* ray)
{
    // Check the button queue
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && (evt.button == PB_A || evt.button == PB_B))
        {
            rayTurntableExit(ray, false);
        }
    }

    // Read touchpads
    linearTouch_t touches[2] = {0};
    getTouchLinear(touches, ARRAY_SIZE(touches));
    memcpy(&ray->ts.tables[0].touch, &touches[0], sizeof(linearTouch_t));
    memcpy(&ray->ts.tables[1].touch, &touches[1], sizeof(linearTouch_t));
}

/**
 * @brief Render the current turntable box
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayTurntableRender(ray_t* ray, uint32_t elapsedUs)
{
    rayTurntableState_t* ts = &ray->ts;

    // 33.3 RPM is 12000 degrees every 60000000uS, or 1 degree every 5000 uS
    RUN_TIMER_EVERY(ts->rotationTimer, 5000, elapsedUs, {
        for (int i = 0; i < 2; i++)
        {
            if (!ts->tables[i].touch.touched)
            {
                ts->tables[i].catcherAngle = (ts->tables[i].catcherAngle + 1) % 360;
            }
        }
    });

    RUN_TIMER_EVERY(ts->wiggleTimer, 250000, elapsedUs, {
        for (int i = 0; i < 2; i++)
        {
            if (!ts->tables[i].touch.touched)
            {
                ts->tables[i].wiggleAngle *= -1;
            }
        }
    });

    int16_t turntableCenterX = TURNTABLE_X + ts->wsg.turntable.w / 2;
    int16_t turntableCenterY = TFT_HEIGHT / 2;
    int16_t turntableY       = turntableCenterY - ts->wsg.turntable.h / 2;
    int16_t recordGrooveX    = TURNTABLE_X + (ts->wsg.turntable.w / 2 - ts->wsg.recordGrooves.w / 2);
    int16_t recordGrooveY    = turntableY + (ts->wsg.turntable.h / 2 - ts->wsg.recordGrooves.h / 2);

    // Left turntable
    drawWsgTile(&ts->wsg.turntable, TURNTABLE_X, turntableY);
    drawWsgSimple(&ts->wsg.knobs, TURNTABLE_X + KNOB_MARGIN_X,
                  turntableY + ts->wsg.turntable.h - KNOB_MARGIN_Y - ts->wsg.knobs.h);
    drawWsg(&ts->wsg.recordGrooves, recordGrooveX, recordGrooveY, false, false, ts->tables[0].wiggleAngle + 180);

    // Right turntable
    drawWsgTile(&ts->wsg.turntable, TFT_WIDTH - TURNTABLE_X - ts->wsg.turntable.w, turntableY);
    drawWsgSimple(&ts->wsg.knobsMirrored, TFT_WIDTH - TURNTABLE_X - KNOB_MARGIN_X - ts->wsg.knobsMirrored.w,
                  turntableY + ts->wsg.turntable.h - KNOB_MARGIN_Y - ts->wsg.knobsMirrored.h);
    drawWsg(&ts->wsg.recordGrooves, TFT_WIDTH - recordGrooveX - ts->wsg.recordGrooves.w, recordGrooveY, false, false,
            ts->tables[1].wiggleAngle + 180);

    // Catchers
    for (int i = 0; i <= CATCHER_THICKNESS; i++)
    {
        drawArc(turntableCenterX, turntableCenterY, LABEL_RADIUS - i,
                (ts->tables[0].catcherAngle + CATCHER_UNDERDRAW_DEGREES) % 360,
                (ts->tables[0].catcherAngle + CATCHER_DEGREES - CATCHER_UNDERDRAW_DEGREES) % 360, c025);
        drawArc(TFT_WIDTH - turntableCenterX - 1, turntableCenterY, LABEL_RADIUS - i,
                (ts->tables[1].catcherAngle + CATCHER_UNDERDRAW_DEGREES) % 360,
                (ts->tables[1].catcherAngle + CATCHER_DEGREES - CATCHER_UNDERDRAW_DEGREES) % 360, c025);
    }

    // Spawn new note if needed
    ts->nextNoteSpawnUs = MAX(ts->nextNoteSpawnUs - elapsedUs, 0);
    if (ts->nextNoteSpawnUs <= 0)
    {
        for (int note = 0; note < MAX_NOTES_PER_TURNTABLE; note++)
        {
            if (ts->tables[ts->nextNoteSpawnSide].notes[note].distance <= 0)
            {
                ts->tables[ts->nextNoteSpawnSide].notes[note].angle     = esp_random() % 360;
                ts->tables[ts->nextNoteSpawnSide].notes[note].distance  = NOTE_SPAWN_DIST;
                ts->tables[ts->nextNoteSpawnSide].notes[note].hitTimeUs = 0;
                ts->tables[ts->nextNoteSpawnSide].notes[note].wsgIdx    = esp_random() % 2;

                ts->nextNoteSpawnUs   = esp_random() % (NOTE_SPAWN_MAX_US - NOTE_SPAWN_MIN_US) + NOTE_SPAWN_MIN_US;
                ts->nextNoteSpawnSide = (ts->nextNoteSpawnSide + 1) % 2;

                break;
            }
        }
    }

    // Move notes and check for hits
    for (int table = 0; table < 2; table++)
    {
        for (int note = 0; note < MAX_NOTES_PER_TURNTABLE; note++)
        {
            float noteDist = ts->tables[table].notes[note].distance;
            if (ts->tables[table].notes[note].hitTimeUs == 0)
            {
                noteDist                               = MAX(noteDist - NOTE_SPEED_PX_PER_US * elapsedUs, 0);
                ts->tables[table].notes[note].distance = noteDist;
            }

            if (noteDist > 0)
            {
                int32_t x = (noteDist * getCos1024(ts->tables[table].notes[note].angle)) / 1024;
                int32_t y = -(noteDist * getSin1024(ts->tables[table].notes[note].angle)) / 1024 + turntableCenterY;
                if (table == 0)
                {
                    x += turntableCenterX;
                }
                else
                {
                    x += TFT_WIDTH - turntableCenterX - 1;
                }

                wsg_t* wsg;
                int16_t rotation = 0;
                if (ts->tables[table].notes[note].hitTimeUs == 0)
                {
                    uint16_t noteAngle = ts->tables[table].notes[note].angle;
                    wsg                = &ts->wsg.notes[ts->tables[table].notes[note].wsgIdx];
                    if (noteAngle < 180)
                    {
                        rotation = 180;
                    }

                    // Treat the notes like a circle for collision purposes
                    if (noteDist - NOTE_HIT_RADIUS < LABEL_RADIUS
                        && noteDist + NOTE_HIT_RADIUS > LABEL_RADIUS - CATCHER_THICKNESS
                        && ((ts->tables[table].catcherAngle <= noteAngle
                             && ts->tables[table].catcherAngle + CATCHER_DEGREES >= noteAngle)
                            || (ts->tables[table].catcherAngle <= noteAngle + 360
                                && ts->tables[table].catcherAngle + CATCHER_DEGREES >= noteAngle + 360)))
                    {
                        ts->tables[table].notes[note].hitTimeUs = NOTE_HIT_DISPLAY_US;
                        ts->score                               = MIN(ts->score + SCORE_PER_NOTE, SCORE_NEEDED);
                    }
                }
                else
                {
                    wsg = &ts->wsg.noteHit;
                    ts->tables[table].notes[note].hitTimeUs
                        = MAX(ts->tables[table].notes[note].hitTimeUs - elapsedUs, 0);
                    if (ts->tables[table].notes[note].hitTimeUs == 0)
                    {
                        ts->tables[table].notes[note].distance = 0;
                    }
                }

                drawWsg(wsg, x - wsg->w / 2, TFT_HEIGHT - y - wsg->h / 2, false, false, rotation);
            }
        }
    }

    if (ts->score >= SCORE_NEEDED)
    {
        // TODO: Some kind of win animation
        rayTurntableExit(ray, true);
    }

    RUN_TIMER_EVERY(ts->scoreTimer, SCORE_DROP_PERIOD_US, elapsedUs, {
        if (ts->score < SCORE_NEEDED)
        {
            ts->score = MAX(ts->score - 1, 0);
        }
    });

    // Draw score meter
    uint16_t meterTextWidth = textWidth(ray->renderer->titleFont, scoreMeterText);
    drawText(ray->renderer->titleFont, c555, scoreMeterText, SCORE_METER_MARGIN, SCORE_METER_TEXT_Y);

    uint16_t meterX      = SCORE_METER_MARGIN + meterTextWidth + SCORE_METER_TEXT_MARGIN + SCORE_METER_OUTLINE;
    uint16_t meterY      = SCORE_METER_TEXT_Y + (ray->renderer->titleFont->height - SCORE_METER_HEIGHT) / 2;
    uint16_t meterWidth  = TFT_WIDTH - SCORE_METER_MARGIN - SCORE_METER_OUTLINE - meterX;
    uint16_t filledWidth = meterWidth * ts->score / SCORE_NEEDED;

    drawRectFilled(meterX, meterY, meterX + filledWidth, meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2, c500);
    drawRectFilled(meterX + filledWidth, meterY, meterX + meterWidth,
                   meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2, c000);
    drawLineFast(meterX + filledWidth, meterY, meterX + filledWidth,
                 meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2, c533);

    for (int i = 1; i <= SCORE_METER_OUTLINE; i++)
    {
        paletteColor_t color;
        if (i == 1 || i == SCORE_METER_OUTLINE)
        {
            color = c000;
        }
        else
        {
            color = c555;
        }
        drawLineFast(meterX, meterY - i, meterX + meterWidth - 1, meterY - i, color);
        drawLineFast(meterX, meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2 + i - 1, meterX + meterWidth - 1,
                     meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2 + i - 1, color);

        drawLineFast(meterX - i, meterY, meterX - i, meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2 - 1, color);
        drawLineFast(meterX + meterWidth + i - 1, meterY, meterX + meterWidth + i - 1,
                     meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2 - 1, color);
    }

    drawWsg(&ts->wsg.meterCorner, meterX - SCORE_METER_OUTLINE, meterY - SCORE_METER_OUTLINE, false, false, 0);
    drawWsg(&ts->wsg.meterCorner, meterX + meterWidth - 1, meterY - SCORE_METER_OUTLINE, false, false, 90);
    drawWsg(&ts->wsg.meterCorner, meterX + meterWidth - 1, meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2 - 1,
            false, false, 180);
    drawWsg(&ts->wsg.meterCorner, meterX - SCORE_METER_OUTLINE,
            meterY + SCORE_METER_HEIGHT - SCORE_METER_OUTLINE * 2 - 1, false, false, 270);
}
