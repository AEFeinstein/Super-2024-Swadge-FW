//==============================================================================
// Includes
//==============================================================================

#include "ray_turntable.h"
#include "ray_script.h"
#include "ray_tex_manager.h"

//==============================================================================
// Defines
//==============================================================================

#define TURNTABLE_X     8
#define KNOB_MARGIN_X   9
#define KNOB_MARGIN_Y   10
#define RECORD_RADIUS   50
#define LABEL_RADIUS    11
#define WIGGLE_DEGREES  2
#define CATCHER_DEGREES 50

#define NOTE_SPAWN_MIN_US    300000
#define NOTE_SPAWN_MAX_US    800000
#define NOTE_SPAWN_DIST      70
#define NOTE_SPEED_PX_PER_US (100.f / 1000000)

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

    ray->ts.tables[0].angle       = 180 - CATCHER_DEGREES / 2;
    ray->ts.tables[1].angle       = 360 - CATCHER_DEGREES / 2;
    ray->ts.tables[0].wiggleAngle = WIGGLE_DEGREES;
    ray->ts.tables[1].wiggleAngle = WIGGLE_DEGREES;
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
                ts->tables[i].angle = (ts->tables[i].angle + 1) % 360;
            }
        }
        /*if (2 == ts->angle)
        {
            ts->angle = 0;

            // Increment count for full turns
            ts->turns++;
            // After two full turns, execute the script and return to the main game
            if (2 == ts->turns)
            {
                //rayTurntableExit(ray, true);
            }
        }*/
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
    for (int i = 0; i < 3; i++)
    {
        drawArc(turntableCenterX, turntableCenterY, LABEL_RADIUS - 2 + i, ts->tables[0].angle,
                (ts->tables[0].angle + CATCHER_DEGREES) % 360, c025);
        drawArc(TFT_WIDTH - turntableCenterX - 1, turntableCenterY, LABEL_RADIUS - 2 + i, ts->tables[1].angle,
                (ts->tables[1].angle + CATCHER_DEGREES) % 360, c025);
    }

    // Spawn new note if needed
    ts->nextNoteSpawnUs = MAX(ts->nextNoteSpawnUs - elapsedUs, 0);
    if (ts->nextNoteSpawnUs <= 0)
    {
        for (int note = 0; note < MAX_NOTES_PER_TURNTABLE; note++)
        {
            if (ts->tables[ts->nextNoteSpawnSide].notes[note].distance <= 0)
            {
                ts->tables[ts->nextNoteSpawnSide].notes[note].angle    = esp_random() % 360;
                ts->tables[ts->nextNoteSpawnSide].notes[note].distance = NOTE_SPAWN_DIST;
                ts->tables[ts->nextNoteSpawnSide].notes[note].wsgIdx   = esp_random() % 2;

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
            ts->tables[table].notes[note].distance
                = MAX(ts->tables[table].notes[note].distance - NOTE_SPEED_PX_PER_US * elapsedUs, 0);
            if (ts->tables[table].notes[note].distance > 0)
            {
                wsg_t* wsg = &ts->wsg.notes[ts->tables[table].notes[note].wsgIdx];
                int32_t x
                    = (ts->tables[table].notes[note].distance * getSin1024(ts->tables[table].notes[note].angle)) / 1024;
                int32_t y
                    = -(ts->tables[table].notes[note].distance * getCos1024(ts->tables[table].notes[note].angle)) / 1024
                      + turntableCenterY;
                if (table == 0)
                {
                    x += turntableCenterX;
                }
                else
                {
                    x += TFT_WIDTH - turntableCenterX - 1;
                }

                // TODO: Check for hit, add score, despawn note

                int16_t rotation = 0;
                if (ts->tables[table].notes[note].angle > 90 && ts->tables[table].notes[note].angle < 270)
                {
                    rotation += 180;
                }

                drawWsg(wsg, x - wsg->w / 2, y - wsg->h / 2, false, false, rotation);
            }
        }
    }
}
