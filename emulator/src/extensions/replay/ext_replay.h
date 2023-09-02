/*! \file ext_replay.h
 *
 * \section ext_replay Replay Emulator Extension
 *
 * The replay extension allows you to record swadge inputs -- button presses, touchpad inputs,
 * and accelerometer readings -- to a file, and then play them back later. And when playing back
 * inputs, several special actions are also supported: starting fuzzing, taking a screenshot,
 * and quitting the emulator. These must be manually added to a recording file, but they can used
 * to automatically target fuzzing to a specific screen, or to automatically generate a screenshot
 * of a specific screen.
 *
 * \section ext_format Recording File Format
 * If not given a custom name, recording files will be created in the current directory with the
 * name 'rec-TIMESTAMP.csv' As suggested by the name, this is simply a CSV file. The first line
 * contains the header, which specifies three columns: Time, Type, and Value. The rest of the lines
 * will be the individual input values or special actions.
 *
 * The first column, `Time`, is the timestamp, in microseconds, of the input. The next column,
 * `Type`, is the type of data or the special action to perform. Possible options are: `BtnDown`,
 * `BtnUp`, `TouchPhi`, `TouchR`, `TouchI`, `AccelX`, `AccelY`, or `AccelZ`, and `Screenshot`,
 * `Fuzz`, and `Quit`.
 *
 * The third column, `Value`, depends on the value of the `Type` column. For `BtnDown` and `BtnUp`,
 * this is the name of the button: `A`, `B`, `Up`, `Down`, `Left`, `Right`, `Select`, or `Start`.
 * For all `Touch*` and `Accel*` types, the value is an integer. For `Screenshot`, the third column
 * is the filename for the screenshot, or it may be left blank for an automatically generated filename.
 * For `Quit` and `Fuzz`, the third column is completely ignored.
 *
 * The following example recording file will generate a few button presses and touch events, then take
 * a screenshot after about 4 seconds, begin fuzzing until 10 seconds have passed, and then take another
 * screenshot before closing the emulator.
 * \code{.csv}
 * Time,Type,Value
 * 23558,AccelZ,242
 * 500383,BtnDown,A
 * 565291,BtnUp,A
 * 1498312,TouchPhi,319
 * 1498312,TouchR,132
 * 1498312,TouchI,1024
 * 1532527,TouchPhi,328
 * 1532527,TouchR,146
 * 1582694,TouchPhi,12
 * 1582694,TouchR,350
 * 1749475,TouchPhi,0
 * 1749475,TouchR,0
 * 1749475,TouchI,0
 * 2582065,BtnDown,Up
 * 2631439,BtnUp,Up
 * 2832071,BtnDown,Down
 * 2882992,BtnUp,Down
 * 3518793,BtnDown,Right
 * 3568577,BtnUp,Right
 * 3636235,BtnDown,Left
 * 3652407,BtnUp,Left
 * 3735765,Screenshot,beforeFuzz.bmp
 * 3802347,Fuzz,
 * 10000000,Screenshot,afterFuzz.bmp
 * 10000000,Quit,
 * \endcode
 */

#include "emu_ext.h"

extern emuExtension_t replayEmuExtension;

bool takeScreenshot(const char* name);
