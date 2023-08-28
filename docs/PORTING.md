This is a non-exhaustive list of changes I've found to be needed when porting Tunernome. They should cut down on time required to port modes from 2023. If porting from pre-2023, check out [this similar list I made last year](https://github.com/AEFeinstein/Super-2023-Swadge-FW/issues/31#issuecomment-1221395802).

# Changes that involve re-design

- `mm.font` has been removed. The closest font is `logbook.font` which is 3 pixels smaller in height. Arrows and symbol `wsg`s need to be updated for the smaller font too.
- Accel, touch, and button callbacks no longer exist. Instead, checks must be done in the main loop function. This can be used to more easily adapt button callbacks to the new standard:

```
buttonEvt_t evt = {0};
while (checkButtonQueueWrapper(&evt))
{
    // If processing menu buttons:
    tunernome->menu = menuButton(tunernome->menu, evt);
    // OR, if processing mode buttons:
    tunernomeButtonCb(&evt);
}
...
// The rest of your mainLoop function
```

# Things that need to be changed in mode header files

- Add `#include "swadge2024.h"`
- Naming conventions changed from `mode_tunernome.h` to `tunernome.h`. Don't forget to change your header guard names too:

```
_MODE_TUNERNOME_H_
_TUNERNOME_H_
```

# Things that need to be added to `enterMode` and `exitMode` functions

- `tunernome->menuRenderer = initMenuLogbookRenderer(&tunernome->logbookFont);`

- `deinitMenuLogbookRenderer(tunernome->renderer);`

# Things that need to be intelligently deleted

- `tunernome->disp`

# Things that need to be intelligently changed or replaced

```
modeTunernome
tunernomeMode

drawText(tunernome->disp, 
drawText(

fillDisplayArea(tunernome->disp, 
fillDisplayArea(

drawWsg(tunernome->disp, 
drawWsg(

plotLine(tunernome->disp, 
drawLine(

plotCircleQuadrants(tunernome->disp, 
drawCircleQuadrants(

plotCircleFilled(tunernome->disp, 
drawCircleFilled(

plotCircle(tunernome->disp, 
drawCircle(

plotRect(tunernome->disp, 
plotRect(

tunernome->disp->clearPx();
clearPxTft();

tunernome->disp->w
TFT_WIDTH

tunernome->disp->h
TFT_HEIGHT

ibm_vga8.h
ibm_vga8.height

mm.h
mm.height

radiostars.h
radiostars.height

loadFont(const char* name, font_t* font)
loadFont(const char* name, font_t* font, bool spiRam)

bool loadWsg(char* name, wsg_t* wsg)
bool loadWsg(char* name, wsg_t* wsg, bool spiRam)

menu->selectedRow
menu->currentItem

menu->rows
menu->items

initMeleeMenu(modeTunernome.modeName, &(tunernome->mm), tunernomeMainMenuCb);
initMenu(tunernomeMode.modeName, tunernomeMainMenuCb);

tunernomeMenuCb(const char* opt)
mainMenuCb(const char* label, bool selected, uint32_t settingVal)

drawMeleeMenu(tunernome->menu);
drawMenuLogbook(tunernome->menu, tunernome->renderer, elapsedUs);
```

# Things that can be find/replaced

```
swadgeMode
swadgeMode_t

NUM_LEDS
CONFIG_NUM_LEDS

#include "embeddednf.h"
#include "embeddedNf.h"

#include "embeddedout.h"
#include "embeddedOut.h"

embeddednf_data
embeddedNf_data

embeddedout_data
embeddedOut_data

FIXBPERO
FIX_B_PER_O

UP
PB_UP

DOWN
PB_DOWN

LEFT
PB_LEFT

RIGHT
PB_RIGHT

BTN_A
PB_A

BTN_B
PB_B

START
PB_START

SELECT
PB_SELECT

buzzer_play_sfx(
bzrPlaySfx(

buzzer_play_bgm(
bzrPlayBgm(

buzzer_stop(
bzrStop(

incMicGain()
incMicGainSetting()

decMicGain()
decMicGainSetting()

getMicGain()
getMicGainSetting()

setAndSaveLedBrightness(
setLedBrightnessSetting(

meleeMenu_t
menu_t

deinitMeleeMenu(
deinitMenu(

modeMainMenu
mainMenuMode
```
