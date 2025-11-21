//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_game.h"
#include "artillery_phys_camera.h"
#include "artillery_p2p.h"

//==============================================================================
// Defines
//==============================================================================

#define BARREL_INTERVAL      1
#define POWER_INTERVAL       1.0f
#define TOUCH_DEG_PER_BARREL 8

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Switch to a new game state (menu, adjusting shot, etc.)
 *
 * @param ad All the artillery mode data
 * @param newState The state to switch to
 */
void artillerySwitchToGameState(artilleryData_t* ad, artilleryGameState_t newState)
{
    // Set the new state
    ad->gState = newState;

    // Clear all camera targets
    clear(&ad->phys->cameraTargets);

    // Always stop moving when switching states
    if (ad->players[ad->plIdx])
    {
        ad->players[ad->plIdx]->moving = 0;
    }

    // Additional state-specific setup
    switch (ad->gState)
    {
        case AGS_TOUR:
        {
            bool initialCameraSet = false;
            // Build a list of points for the camera to tour
            clear(&ad->phys->cameraTour);
            node_t* lNode    = ad->phys->lines.first;
            physLine_t* line = NULL;
            while (lNode)
            {
                line = lNode->val;
                if (line->isTerrain)
                {
                    push(&ad->phys->cameraTour, &line->l.p1);
                    if (!initialCameraSet)
                    {
                        initialCameraSet   = true;
                        ad->phys->camera.x = line->l.p1.x - (TFT_WIDTH / 2);
                        ad->phys->camera.y = line->l.p1.y - (TFT_HEIGHT / 2);
                    }
                }
                lNode = lNode->next;
            }
            push(&ad->phys->cameraTour, &line->l.p2);
            break;
        }
        case AGS_MENU:
        {
            setDriveInMenu(ad->moveTimerUs);
            setAmmoInMenu();
            break;
        }
        case AGS_CPU_ADJUST:
        {
            ad->players[ad->plIdx]->targetBarrelAngle = -1;
            ad->cpuWaitTimer                          = 2000000;
            break;
        }
        case AGS_FIRE:
        {
            // If this is the player
            if (artilleryIsMyTurn(ad))
            {
                // Check the trophy for all ammo types
                trophySetChecklistTask(&artilleryTrophies[AT_ROYAL_SAMPLER], ad->players[ad->plIdx]->ammoIdx, true,
                                       true);
            }
            break;
        }
        default:
        case AGS_WAIT:
        case AGS_MOVE:
        case AGS_ADJUST:
        case AGS_CPU_MOVE:
        case AGS_LOOK:
        {
            break;
        }
    }

    // Focus on player if not looking around
    if (AGS_LOOK != ad->gState)
    {
        if (ad->players[ad->plIdx])
        {
            push(&ad->phys->cameraTargets, ad->players[ad->plIdx]);
        }
    }
}

/**
 * @brief Process button events for the artillery game
 *
 * @param ad All the artillery mode data
 * @param evt The button event
 * @return true if the p2p state changed (barrel angle, camera, etc.) which will TX a packet, false otherwise
 */
bool artilleryGameInput(artilleryData_t* ad, buttonEvt_t evt)
{
    switch (ad->gState)
    {
        default:
        {
            // Do nothing
            break;
        }
        case AGS_TOUR:
        {
            if (evt.down && ((PB_START | PB_SELECT) & evt.button))
            {
                artilleryTxFinishTour(ad);
                artilleryFinishTour(ad);
            }
            break;
        }
        case AGS_WAIT:
        {
            // Do nothing!
            break;
        }
        case AGS_MENU:
        {
            // Menu navigation
            menu_t* oldMenu = ad->gameMenu;
            ad->gameMenu    = menuButton(ad->gameMenu, evt);

            // If the ammo menu was entered, scroll to current ammo
            if (oldMenu != ad->gameMenu && str_load_ammo == ad->gameMenu->title)
            {
                menuNavigateToItem(ad->gameMenu, getAmmoAttribute(ad->players[ad->plIdx]->ammoIdx)->name);
            }
            break;
        }
        case AGS_LOOK:
        {
            // Check if the menu should be exited
            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_A:
                    case PB_B:
                    {
                        // Return to the menu
                        artillerySwitchToGameState(ad, AGS_MENU);
                        // Return true to send state
                        return true;
                    }
                    default:
                    {
                        break;
                    }
                }
            }

            // Set button to pan camera
            physSetCameraButton(ad->phys, evt.state);
            break;
        }
        case AGS_MOVE:
        {
            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_LEFT:
                    case PB_RIGHT:
                    {
                        // Left and right buttons set acceleration
                        ad->players[ad->plIdx]->moving = evt.button;
                        break;
                    }
                    case PB_A:
                    case PB_B:
                    {
                        artillerySwitchToGameState(ad, AGS_MENU);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            else
            {
                // Releasing a button clears acceleration
                ad->players[ad->plIdx]->moving = 0;
            }
            break;
        }
        case AGS_ADJUST:
        {
            switch (evt.button)
            {
                case PB_LEFT:
                case PB_RIGHT:
                {
                    if (evt.down)
                    {
                        ad->adjButtonHeld       = evt.button;
                        ad->adjButtonStartTimer = 0;
                        int16_t bDiff           = (PB_LEFT == evt.button) ? -(BARREL_INTERVAL) : (BARREL_INTERVAL);
                        setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle + bDiff);
                        return true;
                    }
                    else
                    {
                        ad->adjButtonHeld       = 0;
                        ad->adjButtonStartTimer = 0;
                    }
                    break;
                }
                case PB_UP:
                case PB_DOWN:
                {
                    if (evt.down)
                    {
                        ad->adjButtonHeld       = evt.button;
                        ad->adjButtonStartTimer = 0;
                        float pDiff             = (PB_UP == evt.button) ? POWER_INTERVAL : -POWER_INTERVAL;
                        setShotPower(ad->players[ad->plIdx], ad->players[ad->plIdx]->shotPower + pDiff);
                    }
                    else
                    {
                        ad->adjButtonHeld       = 0;
                        ad->adjButtonStartTimer = 0;
                    }
                    break;
                }
                case PB_A:
                case PB_B:
                {
                    if (evt.down)
                    {
                        artillerySwitchToGameState(ad, AGS_MENU);
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case AGS_CPU_MOVE:
        case AGS_CPU_ADJUST:
        {
            // No input while it's the CPU turn
            break;
        }
    }
    return false;
}

/**
 * @brief Run the main loop for the artillery game.
 * This includes running physics and drawing to the TFT.
 *
 * @param ad All the artillery mode data
 * @param elapsedUs The time elapsed since this was last called.
 * @param stateChanged
 */
void artilleryGameLoop(artilleryData_t* ad, uint32_t elapsedUs, bool stateChanged)
{
    // Draw the scene
    drawPhysOutline(ad->phys, ad->players, &ad->font_pulseAux, &ad->font_pulseAuxOutline, ad->turn);

    // Step the physics
    bool playerMoved = false;
    bool cameraMoved = false;
    physStep(ad->phys, elapsedUs, AGS_MENU == ad->gState, &playerMoved, &cameraMoved);

    // Draw depending on the game state
    switch (ad->gState)
    {
        case AGS_TOUR:
        {
            // If there are no more points to tour
            if (0 == ad->phys->cameraTour.length)
            {
                // Move to the next game state
                if (artilleryIsMyTurn(ad))
                {
                    artillerySwitchToGameState(ad, AGS_MENU);
                    // And start on the ammo menu
                    openAmmoMenu();
                }
                else
                {
                    artillerySwitchToGameState(ad, AGS_WAIT);
                }
            }
            break;
        }
        case AGS_MENU:
        {
            // Menu renderer
            drawMenuSimple(ad->gameMenu, ad->smRenderer, elapsedUs);
            break;
        }
        case AGS_ADJUST:
        {
            // Check for touchpad input
            int32_t phi       = 0;
            int32_t r         = 0;
            int32_t intensity = 0;
            if (getTouchJoystick(&phi, &r, &intensity))
            {
                // If the touchpad was previously untouched, set the last angle as current
                if (INT32_MIN == ad->tpLastPhi)
                {
                    ad->tpLastPhi = phi;
                }

                // Find the difference between the last angle and this one
                int32_t diff = ad->tpLastPhi - phi;
                if (diff > 180)
                {
                    diff -= 360;
                }
                else if (diff < -180)
                {
                    diff += 360;
                }

                // Keep track of the cumulative difference
                ad->tpCumulativeDiff += diff;

                // Adjust the barrel when enough touchpad degrees have accumulated
                while (ad->tpCumulativeDiff >= TOUCH_DEG_PER_BARREL)
                {
                    ad->tpCumulativeDiff -= TOUCH_DEG_PER_BARREL;
                    setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle + BARREL_INTERVAL);
                    stateChanged = true;
                }
                while (ad->tpCumulativeDiff <= -TOUCH_DEG_PER_BARREL)
                {
                    ad->tpCumulativeDiff += TOUCH_DEG_PER_BARREL;
                    setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle - BARREL_INTERVAL);
                    stateChanged = true;
                }

                // Save the current touchpad angle
                ad->tpLastPhi = phi;
            }
            else
            {
                ad->tpLastPhi        = INT32_MIN;
                ad->tpCumulativeDiff = 0;
            }

            // If a button is held
            if (ad->adjButtonHeld)
            {
                // Ignore it for the first quarter second
                if (ad->adjButtonStartTimer < 250000)
                {
                    ad->adjButtonStartTimer += elapsedUs;
                }
                else
                {
                    // Left/Right moves faster than Up/Down
                    int32_t timerInterval = (ad->adjButtonHeld & (PB_LEFT | PB_RIGHT)) ? 10000 : 40000;

                    // Run a periodic timer to adjust inputs
                    RUN_TIMER_EVERY(ad->adjButtonHeldTimer, timerInterval, elapsedUs, {
                        switch (ad->adjButtonHeld)
                        {
                            case PB_LEFT:
                            case PB_RIGHT:
                            {
                                int16_t bDiff = (PB_LEFT == ad->adjButtonHeld) ? -(BARREL_INTERVAL) : (BARREL_INTERVAL);
                                setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle + bDiff);
                                stateChanged = true;
                                break;
                            }
                            case PB_UP:
                            case PB_DOWN:
                            {
                                float pDiff = (PB_UP == ad->adjButtonHeld) ? POWER_INTERVAL : -POWER_INTERVAL;
                                setShotPower(ad->players[ad->plIdx], ad->players[ad->plIdx]->shotPower + pDiff);
                                stateChanged = true;
                                break;
                            }
                            default:
                            {
                                ad->adjButtonHeld = 0;
                                break;
                            }
                        }
                    });
                }
            }

            font_t* f           = getSysFont();
            const physCirc_t* p = ad->players[ad->plIdx];

            const int32_t BAR_WIDTH   = MAX_SHOT_POWER / 2;
            const int32_t BAR_MARGIN  = ((TFT_WIDTH - BAR_WIDTH) / 2);
            const int32_t FONT_MARGIN = 3;
            const int32_t BAR_HEIGHT  = f->height + (FONT_MARGIN * 2);

            // Draw power bar
            int32_t barSplit = BAR_MARGIN + (p->shotPower / 2);
            fillDisplayArea(BAR_MARGIN, TFT_HEIGHT - BAR_HEIGHT, barSplit, TFT_HEIGHT, COLOR_POWER_BAR_FILL);
            fillDisplayArea(barSplit, TFT_HEIGHT - BAR_HEIGHT, TFT_WIDTH - BAR_MARGIN, TFT_HEIGHT,
                            COLOR_POWER_BAR_EMPTY);
            drawRect(BAR_MARGIN, TFT_HEIGHT - BAR_HEIGHT, TFT_WIDTH - BAR_MARGIN, TFT_HEIGHT, COLOR_POWER_BAR_BORDER);

            // Draw the power
            char fireParams[64];
            snprintf(fireParams, sizeof(fireParams) - 1, "Power %d", (int)p->shotPower);
            drawText(f, COLOR_TEXT, fireParams, BAR_MARGIN + FONT_MARGIN, TFT_HEIGHT - f->height - FONT_MARGIN);

            // Draw the angle
            snprintf(fireParams, sizeof(fireParams) - 1, "Angle %d", p->barrelAngle);
            drawTextShadow(f, COLOR_TEXT, COLOR_TEXT_SHADOW, fireParams, BAR_MARGIN + FONT_MARGIN,
                           TFT_HEIGHT - (2 * f->height) - (3 * FONT_MARGIN));

            // Draw the ammo
            const char* ammo = getAmmoAttribute(p->ammoIdx)->name;
            int16_t tWidth   = textWidth(f, ammo);
            drawTextShadow(f, COLOR_TEXT, COLOR_TEXT_SHADOW, ammo, TFT_WIDTH - BAR_MARGIN - FONT_MARGIN - tWidth,
                           TFT_HEIGHT - (2 * f->height) - (3 * FONT_MARGIN));

            break;
        }
        case AGS_FIRE:
        {
            physCirc_t* player   = ad->players[ad->plIdx];
            physCirc_t* opponent = ad->players[(ad->plIdx + 1) % NUM_PLAYERS];

            // If the shot hasn't fired yet, fire away!
            if (false == ad->phys->shotFired)
            {
                ad->phys->shotFired = true;
                artilleryTxShot(ad, player);
                fireShot(ad->phys, player, opponent, true);
            }
            else if (player->shotsRemaining)
            {
                RUN_TIMER_EVERY(player->shotTimer, 250000, elapsedUs, {
                    player->shotsRemaining--;
                    fireShot(ad->phys, player, opponent, false);
                });
            }
            // Run a timer to wait between switching players, otherwise it's too rushed
            else if (ad->phys->playerSwapTimerUs)
            {
                ad->phys->playerSwapTimerUs -= elapsedUs;
                if (ad->phys->playerSwapTimerUs <= 0)
                {
                    artilleryPassTurn(ad);

                    // Only switch back to idle eyes if dead eyes aren't being shown
                    if (ad->deadEyeTimer <= 0)
                    {
                        // Go back to idle
                        ad->eyeSlot = EYES_CC;
                        ch32v003SelectBitmap(ad->eyeSlot);
                    }
                }
            }
            else if (ad->phys->cameraTargets.length)
            {
                // Center Pulse's viewpoint a bit below the player
                vec_t center = {
                    .x = ad->phys->bounds.x / 2,
                    .y = ad->players[ad->plIdx]->c.pos.y + 16,
                };

                // Look at the first camera target
                physCirc_t* target = ad->phys->cameraTargets.first->val;
                vec_t look         = {
                            .x = target->c.pos.x - center.x,
                            .y = target->c.pos.y - center.y,
                };

                // Pick the eye slot based on the looking direction
                artilleryEye_t eyeSlot = EYES_CC;
                if (0 == look.x)
                {
                    if (look.y < 0)
                    {
                        eyeSlot = EYES_UC;
                    }
                    else
                    {
                        eyeSlot = EYES_DC;
                    }
                }
                else
                {
                    // Find the slope of the look vector
                    int slope = (1024 * look.y) / look.x;

                    // Pick where the eyes are pointing depending on the slope
                    if (slope > 2472 || slope < -2472)
                    {
                        // Up or down
                        if (look.y < 0)
                        {
                            eyeSlot = EYES_UC;
                        }
                        else
                        {
                            eyeSlot = EYES_DC;
                        }
                    }
                    else if (slope > 424) // between 424 and 2472
                    {
                        // Down right or up left
                        if (look.x > 0)
                        {
                            eyeSlot = EYES_DR;
                        }
                        else
                        {
                            eyeSlot = EYES_UL;
                        }
                    }
                    else if (slope > -424) // between -424 and 424
                    {
                        // Right or Left
                        if (look.x > 0)
                        {
                            eyeSlot = EYES_CR;
                        }
                        else
                        {
                            eyeSlot = EYES_CL;
                        }
                    }
                    else // between -2472 and -424
                    {
                        // Up right or down left
                        if (look.x > 0)
                        {
                            eyeSlot = EYES_UR;
                        }
                        else
                        {
                            eyeSlot = EYES_DL;
                        }
                    }
                }

                // Select new eyes if it changed, and deadeyes aren't being displayed
                if (ad->deadEyeTimer <= 0 && eyeSlot != ad->eyeSlot)
                {
                    ad->eyeSlot = eyeSlot;
                    ch32v003SelectBitmap(ad->eyeSlot);
                }
            }
            break;
        }
        case AGS_MOVE:
        case AGS_CPU_MOVE:
        {
            // Pick a random direction for the CPU to move
            if ((AGS_CPU_MOVE == ad->gState) && (0 == ad->players[ad->plIdx]->moving))
            {
                ad->players[ad->plIdx]->moving = (esp_random() & 0x01) ? PB_LEFT : PB_RIGHT;
            }

            // If there is time left to move and the player is moving
            if (ad->moveTimerUs && ad->players[ad->plIdx]->moving)
            {
                // Decrement the timer
                ad->moveTimerUs -= elapsedUs;
                // If the timer expired
                if (ad->moveTimerUs <= 0)
                {
                    // Clear timer and button inputs and return to the menu
                    ad->moveTimerUs                = 0;
                    ad->players[ad->plIdx]->moving = 0;
                    // Humans go back to the menu, CPUs adjust shot
                    artillerySwitchToGameState(ad, (ad->gState == AGS_MOVE) ? AGS_MENU : AGS_CPU_ADJUST);
                }
            }

            // Draw gas gauge
            fillDisplayArea(0, TFT_HEIGHT - 24, (TFT_WIDTH * ad->moveTimerUs) / TANK_MOVE_TIME_US, TFT_HEIGHT,
                            COLOR_GAS_GAUGE);
            shadeDisplayArea(0, TFT_HEIGHT - 24, (TFT_WIDTH * ad->moveTimerUs) / TANK_MOVE_TIME_US, TFT_HEIGHT, 2,
                             COLOR_GAS_GAUGE_2);

            break;
        }
        case AGS_LOOK:
        case AGS_WAIT:
        {
            break;
        }
        case AGS_CPU_ADJUST:
        {
            physCirc_t* cpu = ad->players[ad->plIdx];

            // Keep track of if the barrel is positioned correctly
            bool readyToFire = false;
            if (-1 != cpu->targetBarrelAngle)
            {
                // Randomize ammo
                uint16_t numAmmos;
                getAmmoAttributes(&numAmmos);
                cpu->ammoIdx = esp_random() % numAmmos;

                // Find the clockwise and counterclockwise distances to move the barrel to the target
                int16_t deltaCw = cpu->barrelAngle - cpu->targetBarrelAngle;
                if (deltaCw < 0)
                {
                    deltaCw += 360;
                }
                int16_t deltaCCw = cpu->targetBarrelAngle - cpu->barrelAngle;
                if (deltaCCw < 0)
                {
                    deltaCCw += 360;
                }

                // If the barrel isn't at the target, move barrel towards target
                if (abs(deltaCw) > BARREL_INTERVAL)
                {
                    if (deltaCw < deltaCCw)
                    {
                        setBarrelAngle(cpu, cpu->barrelAngle - BARREL_INTERVAL);
                    }
                    else
                    {
                        setBarrelAngle(cpu, cpu->barrelAngle + BARREL_INTERVAL);
                    }
                }
                else // Barrel is at the target
                {
                    readyToFire = true;
                }
            }

            // Wait some time after moving or to adjust the barrel
            if (ad->cpuWaitTimer > 0)
            {
                ad->cpuWaitTimer -= elapsedUs;

                if (ad->cpuWaitTimer <= 0)
                {
                    // If the target hasn't acquired yet
                    if (cpu->targetBarrelAngle < 0)
                    {
                        // Calculate the shot
                        adjustCpuShot(ad->phys, cpu, ad->players[(ad->plIdx + 1) % NUM_PLAYERS]);

                        // Round power and angle, to be fair
                        cpu->shotPower
                            = POWER_INTERVAL * (int)((cpu->shotPower / POWER_INTERVAL) + (POWER_INTERVAL / 2.0f));
                        cpu->targetBarrelAngle
                            = BARREL_INTERVAL
                              * (int)((cpu->targetBarrelAngle / BARREL_INTERVAL) + (BARREL_INTERVAL / 2.0f));

                        // Give time to move the barrel
                        ad->cpuWaitTimer = 2000000;
                    }
                    else if (readyToFire)
                    {
                        // Fire!
                        artillerySwitchToGameState(ad, AGS_FIRE);
                    }
                    else
                    {
                        // Timer elapsed, but barrel isn't ready yet, extend by a little
                        ad->cpuWaitTimer = 1;
                    }
                }
            }
            break;
        }
    }

    // If this is a wireless game and there is some change and it's our turn
    if (AG_WIRELESS == ad->gameType)
    {
        if ((stateChanged || playerMoved || cameraMoved) && artilleryIsMyTurn(ad))
        {
            // Transmit the player and camera state to the other Swadge
            artilleryTxState(ad);
        }
    }

    // Uncomment to draw FPS
    // DRAW_FPS_COUNTER((*getSysFont()));
}

/**
 * @brief TODO
 *
 * @param ad
 */
void artilleryFinishTour(artilleryData_t* ad)
{
    // Immediately clear the tour points
    clear(&ad->phys->cameraTour);

    // Move to the next game state
    if (artilleryIsMyTurn(ad))
    {
        artillerySwitchToGameState(ad, AGS_MENU);
        // And start on the ammo menu
        openAmmoMenu();
    }
    else
    {
        artillerySwitchToGameState(ad, AGS_WAIT);
    }
}

/**
 * @brief TODO doc
 *
 */
void artilleryPassTurn(artilleryData_t* ad)
{
    ad->phys->playerSwapTimerUs = 0;
    ad->phys->shotFired         = false;

    // Switch to the next player
    ad->plIdx = (ad->plIdx + 1) % NUM_PLAYERS;

    // If we're back to the first player
    if (0 == ad->plIdx)
    {
        // Increment the turn
        ad->turn++;

        if (MAX_TURNS == ad->turn + 1)
        {
            // If all turns have been taken end the game
            ad->mState = AMS_GAME_OVER;

            // Stop playing music
            globalMidiPlayerStop(MIDI_BGM);
            midiGmOn(globalMidiPlayerGet(MIDI_BGM));
            midiPause(globalMidiPlayerGet(MIDI_BGM), true);
            globalMidiPlayerStop(MIDI_SFX);
            midiGmOn(globalMidiPlayerGet(MIDI_SFX));
            midiPause(globalMidiPlayerGet(MIDI_SFX), false);

            bool isP1 = true;
            switch (ad->gameType)
            {
                case AG_CPU_PRACTICE:
                {
                    isP1 = true;
                    if (ad->players[0]->score > ad->players[1]->score)
                    {
                        trophyUpdate(&artilleryTrophies[AT_SKYNET], true, true);
                    }
                    break;
                }
                default:
                case AG_PASS_AND_PLAY:
                {
                    isP1 = true;
                    trophyUpdate(&artilleryTrophies[AT_PASS_AND_PLAY],
                                 trophyGetSavedValue(&artilleryTrophies[AT_PASS_AND_PLAY]) + 1, true);
                    break;
                }
                case AG_WIRELESS:
                {
                    isP1 = (GOING_FIRST == p2pGetPlayOrder(&ad->p2p));
                    trophyUpdate(&artilleryTrophies[AT_P2P], trophyGetSavedValue(&artilleryTrophies[AT_P2P]) + 1, true);
                    break;
                }
            }

            ad->gameOverData[0].score       = ad->players[0]->score;
            ad->gameOverData[0].baseColor   = ad->players[0]->baseColor;
            ad->gameOverData[0].accentColor = ad->players[0]->accentColor;
            ad->gameOverData[0].isPlayer    = isP1;

            ad->gameOverData[1].score       = ad->players[1]->score;
            ad->gameOverData[1].baseColor   = ad->players[1]->baseColor;
            ad->gameOverData[1].accentColor = ad->players[1]->accentColor;
            ad->gameOverData[1].isPlayer    = !isP1;

            return;
        }
    }

    // Reset move timer
    ad->moveTimerUs = TANK_MOVE_TIME_US;

    // Check if the tank is in lava
    physCirc_t* player = ad->players[ad->plIdx];
    node_t* lNode      = ad->phys->lines.first;
    while (lNode)
    {
        physLine_t* l = lNode->val;
        if (l->isTerrain && l->isLava)
        {
            if ((player->c.pos.x - player->c.radius) <= (l->l.p2.x) && //
                (player->c.pos.x + player->c.radius) >= (l->l.p1.x))
            {
                // Touching lava, decrement score
                player->score -= 50;
                // Animate lava damage
                player->lavaAnimTimer = (LAVA_ANIM_PERIOD * LAVA_ANIM_BLINKS);
                break;
            }
        }
        lNode = lNode->next;
    }

    // Switch to the right state depending on game type
    switch (ad->gameType)
    {
        default:
        case AG_PASS_AND_PLAY:
        {
            // Return to the menu
            artillerySwitchToGameState(ad, AGS_MENU);
            break;
        }
        case AG_CPU_PRACTICE:
        {
            artillerySwitchToGameState(ad, artilleryIsMyTurn(ad) ? AGS_MENU : AGS_CPU_MOVE);
            break;
        }
        case AG_WIRELESS:
        {
            artillerySwitchToGameState(ad, artilleryIsMyTurn(ad) ? AGS_MENU : AGS_WAIT);
            break;
        }
    }

    if (artilleryIsMyTurn(ad))
    {
        // Always open ammo menu after passing turn because the prior ammo was used
        openAmmoMenu();
    }
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @return true if it is this player's turn, false otherwise
 */
bool artilleryIsMyTurn(artilleryData_t* ad)
{
    switch (ad->gameType)
    {
        default:
        case AG_PASS_AND_PLAY:
        {
            return true;
        }
        case AG_CPU_PRACTICE:
        {
            return (0 == ad->plIdx);
        }
        case AG_WIRELESS:
        {
            // 'GOING_FIRST' sends game parameters first, so 'GOING_SECOND' makes the first move
            return (GOING_SECOND == p2pGetPlayOrder(&ad->p2p)) == (0 == ad->plIdx);
        }
    }
}
