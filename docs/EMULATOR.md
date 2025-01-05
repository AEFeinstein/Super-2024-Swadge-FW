# Emulator User Manual {#emulator}

This document describes how to use the Swadge Emulator as a standalone application. For development
environment setup instructions, see [Configuring a Development Environment](#setup) instead.

## Installing

To install the emulator, go to the project's [releases page](https://github.com/AEFeinstein/Super-2024-Swadge-FW/releases/tag/snapshot)
and download the appropriate `SwadgeEmulator-<os-type>.zip` for your operating system. Windows, Linux, and
Mac (both Intel and ARM) are supported. Then, follow the instructions for your platform below.

### Windows

The Windows version of the emulator does not require any other software to operate. Simply extract the
`.zip` file anywhere you like and double-click on `swadge_emulator.exe`. The first time you open the
application, you may be warned about running applications from the internet and will need to click
"Run" to continue. You might also be prompted by the Windows firewall when starting certain Swadge modes.
This is only required if you wish to run two emulators at the same time using the simulated ESPNOW
wireless connection.

### Linux

The Linux version of the emulator does not require any other software to operate. Simply extract the
`.zip` file anywhere you like and run the `swadge_emulator` program, either by opening it from your
file browser or by running `./swadge_emulator` from the command-line. The Linux emulator includes a
script, `install.sh`, which can be run to install the Swadge Emulator as a desktop application, which
will allow you to open the emulator directly from many desktop environments, and to asssociate the
emulator with MIDI files using the "Open with..." option in your file browser. If you do not want these
features, there is no need to run the script. You will need to run this script again if you download a
new version of the emulator. By default, the installation script will install to `~/.local`, but you
can specify an alternate installation root such as `/usr/local` by passing it as an argument to
the `install.sh` script:

    ./install.sh /usr/local

### Mac

1. Install XQuartz: https://www.xquartz.org/
2. Run the "XQuartz" application. It won't open a window, but it should appear as running in the dock.
3. Right-click or control-click the "SwadgeEmulator" application, and select "Open".
4. The first time you open the application, you will be warned about malicious software. This is because this
   application is not signed, and you will need to click "Open" to continue. The "Open" option will only
   appear if SwadgeEmulator is opened with right-click (or control-click) and Open, and *not* when double-clicking.
5. The Swadge Emulator window should open to the main menu.

## Key Bindings

| Key    | Action         | Description |
|--------|----------------|------------------------------------------------------------|
| W      | Up             | Up on the Swadge D-Pad                                     |
| A      | Left           | Left on the Swadge D-Pad                                   |
| S      | Down           | Down on the Swadge D-Pad                                   |
| D      | Right          | Right on the Swadge D-Pad                                  |
| L      | A              | The Swadge's A Button                                      |
| K      | B              | The Swadge's B Button                                      |
| I      | Menu           | The Swadge's Menu Button, for Quick Settings and Exit Mode |
| O      | Pause          | The Swadge's Pause Button                                  |
| 1      | Touchpad Up    | The top of the touchpad                                    |
| 2      | Touchpad Left  | The left side of the touchpad                              |
| 3      | Touchpad Right | The right side of the touchpad                             |
| 4      | Touchpad Down  | The bottom of the touchpad                                 |
| Escape | Exit\*         | Exits the emulator, **only** when in fullscreen            |
| F4, \` | Toggle Console | Opens or closes the emulator console                       |
| F5     | Toggle FPS     | Shows or hides the FPS counter                             |
| F8     | Print Alloc    | Print all current allocated memory, if printing enabled    |
| F9     | Step Frame     | **When paused**, steps forward a single frame              |
| F10    | Pause Emulator | Pauses or unpauses the emulator                            |
| F11    | Screen Record  | Starts or stops recording the screen to a GIF file         |
| F12    | Screeshot      | Saves a screenshot to a PNG file                           |


## Command-line Arguments

The emulator supports a variety of command-line arguments that can enable extra functionality or
modify the normal behavior of the emulator. A full list of these arguments can always be found
by starting the Swadge Emulator using the `--help` or `-h` arguments. At the time of writing, here
is the current list of possible arguments:

```
Usage: swadge_emulator [OPTION...]
Emulates a swadge
     --fake-fps=RATE         Set a fake framerate. RATE can be a decimal number
     --fake-time             Use a fake timer that ticks at a constant
 -f, --fullscreen            Open in fullscreen mode
     --fuzz                  Enable fuzzing mode, which injects random input in order to test modes
     --fuzz-buttons[=y|n]    Set whether buttons are fuzzed
     --fuzz-touch[=y|n]      Set whether touchpad inputs are fuzzed
     --fuzz-time[=y|n]       Set whether frame durations are fuzzed
     --fuzz-motion[=y|n]     Set whether motion inputs are fuzzed
     --headless              Runs the emulator without a window.
     --hide-leds             Don't draw simulated LEDs next to the display
 -k, --keymap=LAYOUT         Use an alternative keymap. LAYOUT can be azerty, colemak, or dvorak
 -l, --lock                  Lock the emulator in the start mode
     --midi-file=FILE        Open and immediately play a MIDI file
 -m, --mode=MODE             Start the emulator in the swadge mode MODE instead of the main menu
     --mode-switch[=TIME]    Enable or set the timer to switch modes automatically
     --modes-list            Print out a list of all possible values for MODE
 -p, --playback=FILE         Play back recorded emulator inputs from a file
 -r, --record[=FILE]         Record emulator inputs to a file
 -s, --seed=SEED             Seed the random number generator with a specific value
 -c, --show-fps[=OPTION]     Display an FPS counter
 -t, --touch                 Simulate touch sensor readings with a virtual touchpad
     --vsync[=y|n]           Set whether VSync is enabled
 -h, --help                  Give this help list
     --usage                 Give a short usage message
```

There are quite a few which may be overwhelming, but most of them are grouped into a few basic categories.

### Display Arguments

These options affect how the Swadge Emulator is displayed, either by adding or hiding panes in the window,
or by drawing additional information on top of the screen.

`--fullscreen`: As the name suggests, this argument opens the emulator in fullscreen mode. When in fullscreen,
the Escape key can be used to exit the emulator.

`--hide-leds`: Hides the two emulated LED panes which appear on either side of the emulator window.

`--show-fps`: Displays an FPS counter below the emulator screen.

`--touch`: Displays a simulated tocuhpad below the emulator screen. Clicking on this touchpad will generate
touch events that will be read by any swadge mode that uses the touchpad.

`--vsync`: Controls whether VSync is enabled. When VSync is enabled (the default behavior), the Swadge
Emulator's frame rate will be capped at the monitor's refresh rate. If disabled with `--vsync no`, the
emulator will run as fast as possible. Note that this may not be supported by all platforms.

### Recording and Playing Inputs

These options allow inputs to the emulator to be recorded and played back later. This can be useful when
repeatedly performing the same actions during debugging, for sharing with others, or just for convenience.

`--record`: Record the inputs to the swadge emulator in a recording file. If no name is given, a default
recording filename will be generated in the form `rec-<timestamp>.csv`. While recording, all button presses
and touchpad inputs will be written to the recording file, in addition to:

* The original random number generator seed (on playback, this is equivalent to passing `--seed`).
* A screenshot event, whenever a screenshot is taken. Note that the filename is not included, so that the original
  screenshot is not overwritten on playback.

`--playback`: Play back inputs from a recording file, the name of which must be given as an argument. While
inputs are being played back, the emulator will still also accept input directly.

A recording file is a CSV (comma-separated value) file with three columns: Time, Type, and Value.

* `Time`: The timestamp of the action, in microseconds from the time the emulator was started
* `Type`: The type of the recorded action. Types and their meanings are described in the table below.
* `Value`: A value, if any, whose meaning depends on the entry type.

| Type           | Value Meaning         | Description                                                  |
|----------------|-----------------------|--------------------------------------------------------------|
| BtnDown        | Button name (below)   | A button press event. See below for valid `Value` options    |
| BtnUp          | Button name (below)   | A button release event. See below for valid `Value` options  |
| TouchPhi       | Touch location Angle  | The angle of a touchpad press, from 0 to 359                 |
| TouchR         | Touch location Radius | The radius of a touchpad press, from 0 to 1024               |
| TouchI         | Touch intensity       | The intensity of a touchpad press, from 0 to 300000          |
| AccelX         | Accelerometer X force | The acceleration on the X-axis, from -512 to 512             |
| AccelY         | Accelerometer Y force | The acceleration on the Y-axis, from -512 to 512             |
| AccelZ         | Accelerometer Z force | The acceleration on the Z-axis, from -512 to 512             |
| Fuzz           | -                     | Start fuzzing the emulator                                   |
| Quit           | -                     | Exit the emulator immediately                                |
| Screenshot     | Screenshot filename   | Take a screenshot, using a default filename if none is given |
| SetMode        | Mode name             | Switch swadge modes to the named mode                        |
| Seed           | Seed value            | Set the PRNG seed. This should be the first entry in a file  |
| RecordingStart | Recording filename    | Start recording the screen to a GIF, using a default filename if none is given |
| RecordingStop  | -                     | Stop recording the screen                                    |

Note that the `Fuzz`, `Quit`, and `SetMode` entries are never created during recording, and are instead
intended to be inserted manually if desired.

**Button Values**

| Value  |
|--------|
| Up     |
| Down   |
| Left   |
| Right  |
| A      |
| B      |
| Start  |
| Select |

### Fuzzing

These options are for the "fuzzing" functionality, which can help find bugs in modes by generating random
inputs. This includes random

`--fuzz`: Enables fuzzing mode. The default fuzzing behavior is to fuzz button presses, touchpad inputs, and
accelerometer motion, but these can individually be enabled with `--fuzz-*` arguments. Fuzzing the length
of time between each frame is also supported, but is not enabled by default.

`--fuzz-buttons`: Enable or disable fuzzing of buttons presses only.

`--fuzz-touch`: Enable or disable fuzzing of touchpad inputs only.

`--fuzz-time`: Enable the fuzzing of frame times, which will simulate a random amount of time elapsing after
every frame.

`--fuzz-motion`: Enable or disable fuzzing of accelometer motion only.

`--mode-switch`: Automatically switch to a random swadge mode after the specified number of seconds has
passed, repeatedly. For example, `swadge_emulator --mode-switch 5` would switch to a random mode every
5 seconds. If no value is given, modes will be switched every 10 seconds.

### Automation

These options are mainly useful for testing functionality or otherwise automating the emulator.

`--mode`: Starts the emulator directly in a specific Swadge Mode, rather than the introduction mode
or the main menu. For example, `swadge_emulator --mode Credits` will open the credits mode after starting.
You do not need to specify the full name of the mode -- if the mode argument matches the beginning of any
mode's name, that mode will be used. For example, `swadge_emulator --mode Co` will open `Colorchord`, but
`swadge_emulator --mode Cr` will open `Credits`. If the name is ambiguous, the first matching mode in the
list will be used; use `--modes` for the list and its order.

`--modes`: Lists all known swadge modes that can be started using the `--mode` argument.

`--headless`: Starts this emulator without a visible window. The emulator will still run and render
its graphics to an internal display, but there will be no way to directly interact with the emulator.

`--fake-fps`: Simulate a lower framerate without actually changing the speed at which the emulator runs.
For example, passing `--fake-fps 1` will cause each frame to have a duration of  second from the perspective
of a swadge mode. Because the number of actual frames per second doesn't change, this means that 60 seconds
(one simulated second per actual frame) will appear to pass every second.

`--fake-time`: Use a simulated timer that ticks at a constant rate every frame. If used with `--fake-fps`,
the fake frame rate and fake time will be aligned. This argument can be useful when recording or replaying
inputs to ensure that slight differences in frame timing do not cause inconsistencies.

`--lock`: Locks the swadge mode to the starting mode. This prevents all normal means of changing swadge
modes. The mode can still be changed automatically by `--mode-switch`, the console, and by a `SetMode'
command when replaying recorded inputs.

`--midi-file`: Loads and plays a local MIDI or KAR file using the MIDI Player mode.

`--seed`: Sets a specific seed to the pseudorandom number generator. This is useful when trying to reproduce
behavior that relies on `esp_random()`. If the seed is not set, a time-based one will be used. Note that a seed
from one system will not necessarily produce the same output if it is used on a different system.

### Miscellaneous

`--keymap`: Specify an alternative keyboard layout to use for mapping emulator inputs. Possible options
are `azerty`, `dvorak`, or `colemak`.

## Troubleshooting

### Stuck in Factory Test Mode
If the emulator opens to the factory test mode instead of either the main menu or the tutorial mode, this
is because it is unable to write to the `nvs.json` file in the current directory.


## Simulated ESPNOW Networking

The Swadge Emulator is capable of simulating the wireless ESPNOW connection between two swadges. This
functionality is enabled automatically by simply running two swadge emulator programs at the same time.
For best results, you should start each Swadge Emulator from a different directory, to avoid potential
corruption caused by two instances attempting to save data to the same `nvs.json` file at the same time.

**Note**: This functionality only supports local connections between two Swadge Emulators running on the
same machine. Networking between Swadge Emulators running on different machines is not supported at this
time.


## MIDI Instructions

MIDI Files (`.mid`, `.midi`, and `.kar`) can be played in directly by passing the name of
the MIDI file as a command-line argument to the Swadge Emulator. On Windows, you should also be able
to drag a MIDI file on top of `SwadgeEmulator.exe` to play it. On Linux, you should be able to open
MIDI files with the emulator using your file browser's "Open with..." option after running the included
install script.

The Swadge Emulator includes MIDI support, which simulates the USB-MIDI behavior of the real Swadge
using the system MIDI implementation. Note that MIDI implementation and behavior will vary between
platforms.

1. From the main menu, navigate down to "Music" and press the A button (L key)
2. In the Music menu, navigate down to "MIDI Player" and press the A button again
3. You should see the message "Ready!" at the top of the screen. This means the Swadge Emulator is
   now listening for MIDI messages. It will connect to the first available MIDI source it detects,
   or the first one that becomes available. You may need to restart the emulator after connecting
   a new device.
4. Press the Pause button (O key) to open or close the menu.

For details on the Swadge's MIDI support you can use for composing, see the [MIDI Specifications page](#MIDI).