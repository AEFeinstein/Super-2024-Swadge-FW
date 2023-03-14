# Swadge-IDF-5.0

This is the firmware repository for the Super Magfest 2024 Swadge.

<!-- The corresponding hardware repository for the Super Magfest 2024 Swadge can be found here. -->

If you have any questions, feel free to create a Github ticket or email us at circuitboards@magfest.org.

This is living documentation, so if you notice anything incorrect, please open a ticket and/or submit a pull request with a fix!

## Table of Contents

* [Documentation](#documentation)
* [Continuous Integration](#continuous-integration)
* [Configuring a Windows Environment](#configuring-a-windows-environment)
* [Configuring a Linux Environment](#configuring-a-linux-environment)
* [Building and Flashing Firmware](#building-and-flashing-firmware)
* [Building and Running the Emulator](#building-and-running-the-emulator)
* [Configuring VSCode](#configuring-vscode)
* [Notes to Organize](#notes-to-organize)

## Documentation

Full Doxygen documentation is [hosted online here](https://adam.feinste.in/Swadge-IDF-5.0/). This details all APIs and has examples for how to use them. It was written to be referenced when writing Swadge modes.

The [Contribution Guide can be found here](/docs/CONTRIBUTING.md). It should be read before making a contribution.

## Continuous Integration

This project uses Github Actions to automatically build the firmware any time a change is pushed to main.

![Build Firmware and Emulator](https://github.com/AEFeinstein/Swadge-IDF-5.0/actions/workflows/build-firmware-and-emulator.yml/badge.svg)

## Configuring a Windows Environment

1. [install `git`](https://git-scm.com/download/win).
1. [install `python`](https://www.python.org/downloads/). Make sure to check "Add Python to environment variables" when installing
1. [Install `doxygen`](https://www.doxygen.nl/download.html)
1. [Install `cppcheck`](https://cppcheck.sourceforge.io/)
1. [Install `msys2`](https://www.msys2.org/)
1. Start an `msys2` shell and run the following command to install all required packages:
    ```bash
    pacman --noconfirm -S base-devel mingw-w64-x86_64-gcc mingw-w64-x86_64-clang zip
    ```
1. Add the following paths to the Windows path variable. [Here are some instructions on how to do that](https://www.architectryan.com/2018/03/17/add-to-the-path-on-windows-10/).
    * `C:\msys64\mingw64\bin`
    * `C:\msys64\usr\bin`
    * `C:\Program Files\doxygen\bin` 
    * `C:\Program Files\Cppcheck`
    
    You must add add those paths **after** the `python` paths and **before** `C:\Windows\System32`. This is because the build uses Windows `python`, not msys2's', and it uses msys2 `find.exe`, not System32's
When it's all set up, it should look something like this:

    ![image](https://user-images.githubusercontent.com/231180/224911026-0c6b1063-e4f2-4671-a804-bce004085a3a.png)

1. Clone the ESP IDF v5.0.1 and install the tools. Note that it will clone into `$HOME/esp/esp-idf`.
    ```powershell
    Set-ExecutionPolicy -Scope CurrentUser Unrestricted
    git clone -b v5.0.1 --recurse-submodules https://github.com/espressif/esp-idf.git $HOME/esp/esp-idf
    ~/esp/esp-idf/install.ps1
    ```

## Configuring a Linux Environment

1. Run the following comands, depending on your package manager, to install all necessary packages:
    * `apt`:
        ```bash
        sudo apt install build-essential xorg-dev libx11-dev libxinerama-dev libxext-dev mesa-common-dev libglu1-mesa-dev libasound2-dev libpulse-dev libasan8 clang-format cppcheck doxygen python3 python3-venv cmake
        ```
    * `dnf`:
        ```bash
        sudo dnf group install "C Development Tools and Libraries" "Development Tools"
        sudo dnf install libX11-devel libXinerama-devel libXext-devel mesa-libGLU-devel alsa-lib-devel pulseaudio-libs-devel libudev-devel cmake libasan8 clang-format cppcheck doxygen python3 python3-venv cmake
        ```
1. Clone the ESP IDF v5.0.1 and install the tools. Note that it will clone into `~/esp/esp-idf`.
    ```bash
    git clone -b v5.0.1 --recurse-submodules https://github.com/espressif/esp-idf.git ~/esp/esp-idf
    ~/esp/esp-idf/install.sh
    ```

## Building and Flashing Firmware

1. Clone this repository.
    ```powershell
    cd ~/esp/
    git clone https://github.com/AEFeinstein/Swadge-IDF-5.0.git
    cd Swadge-IDF-5.0
    ```
1. Make sure the IDF symbols are exported. This example is for Windows, so the actual command may be different for your OS. Note that `export.ps1` does not make any permanent changes and it must be run each time you open a new terminal for a build.
    ```powershell
    ~/esp/esp-idf/export.ps1
    ```
1. Switch the Swadge to USB power, hold down the PGM button (up on the D-Pad), and plug it into your computer. Note the serial port that enumerates.
1. Build and flash with a single command. Note in this example the ESP is connected to `COM8`, and the serial port will likely be different on your system.
    ```powershell
    idf.py -p COM8 -b 2000000 build flash
    ```

## Building and Running the Emulator

1. Clone this repository. You probably already have it from the prior step but it never hurt to mention.
1. Build the emulator.
    ```powershell
    make clean all
    ```
1. Run the emulator. 
   ```powershell
   ./swadge_emulator
   ```

## Configuring VSCode

[Visual Studio Code IDE](https://code.visualstudio.com/) is recommended for all OSes. The following plugins are recommended:
* [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) - Basic support
* [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack) - Basic support
* [Makefile Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.makefile-tools) - Basic support
* [Espressif IDF](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) - Integration with the IDF. When setting this up for the first time, point it at the IDF which was previously installed. Do not let it install a second copy. Remember that the IDF should exist in `~/esp/esp-idf` and the tools should exist in `~/.espressif/`.
* [C/C++ Advanced Lint](https://marketplace.visualstudio.com/items?itemName=jbenden.c-cpp-flylint) - Integration with `cppcheck`
* [Clang-Format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) - Integration with `clang-format`
* [Doxygen Documentation Generator](https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen) - Integration with `doxygen`
* [Todo Tree](https://marketplace.visualstudio.com/items?itemName=Gruntfuggly.todo-tree) - Handy to track "todos"

The `.vscode` folder already has tasks for making and cleaning the emulator. It also has launch settings for launching the emulator with `gdb` attached. To build the firmware from VSCode, use the espressif extension buttons on the bottom toolbar. The build icon looks like a cylinder. Hover over the other icons to see what they do.

## Notes to Organize

> **Note**
> 
> You must follow the instructions on this page, but I recommend reading through [the official ESP32-S2 Get Started Guide for setting up a development environment](https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32s2/get-started/index.html#installation) for more context or if something written here doesn't work anymore. When given the option, use command line tools instead of installer programs.

For Windows and Linux, I recommend setting up native tools. I don't recommend WSL in Windows. I haven't tried any setup on macOS yet.

From the official guide:
> Keep in mind that ESP-IDF does not support spaces in paths.

By default this guide sets up ESP-IDF in your home directory, so if your username has a space in it, please change all paths to something without a space, like `c:\esp\`. Also note that `ccache` uses a temporary directory in your home directory, and spaces in that path cause issues. `ccache` is enabled by default when running `export.ps1`, but it can be disabled by removing the following from `esp-idf/tools/tools.json`:
```
"export_vars": {
  "IDF_CCACHE_ENABLE": "1"
},
```

This project uses Github Actions to build the firmware each time code is committed to `main`. As a consequence, you can always read the [build-firmware-and-emulator.yml](.github/workflows/build-firmware-and-emulator.yml) to see how the Windows build environment is set up from scratch for both the firmware and emulator.

> **Warning**
> 
> Sometimes `install.ps1`, which is also called in that script, can be a bit finicky and not install everything it's supposed to. If it doesn't create a `~/.espressif/python_env` folder, try running it again. And again. And again. As a last resort you can try editing `install.ps1` and swap the `"Setting up Python environment"` and `"Installing ESP-IDF tools"` sections to set up the Python environment first.

### Troubleshooting
Reread the Get Started Guide, then google your issue, then ask me about it either in a Github issue or the Slack channel. All troubleshooting issues should be written down here for posterity.

If VSCode isn't finding IDF symbols, try running the export.ps1 script from a terminal, then launching code from that same session.

`vsc_esp.ps1`:
```bash
~/esp/esp-idf/export.ps1
code ~/esp/Swadge-IDF-5.0
```

Updating IDF?

### Tips
To add more source files, they either need to be in the main folder, and added to the CMakeLists.txt file there, or in a subdirectory of the components folder with it's own CMakeLists.txt. The folder names are specific. You can read up on the Build System if you're curious.

There are a lot of example projects in the IDF that are worth looking at.
