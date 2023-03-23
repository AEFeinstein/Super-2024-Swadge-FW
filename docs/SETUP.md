# Configuring a Development Environment {#setup}

## General Notes

It is strongly recommend that you follow the instructions on this page to set up your development environment, including the ESP-IDF. It is also possible to follow [Espressif's instructions to install ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32s2/get-started/index.html#installation) through a standalone installer or an IDE. This can be done if you're sure you know what you're doing or the process written here doesn't work anymore.

It is recommended to use native tools (i.e. Windows programs on Windows), not Windows Subsystem for Linux (WSL) or a virtual machine.

macOS is not officially supported. It is likely possible to build the firmware on macOS, but the emulator requires [rawdraw](https://github.com/cntools/rawdraw), which does not support macOS.

Espressif's installation guide notes limitations for the ESP-IDF's path:
> The installation path of ESP-IDF and ESP-IDF Tools must not be longer than 90 characters.
>
> The installation path of Python or ESP-IDF must not contain white spaces or parentheses.
>
> The installation path of Python or ESP-IDF should not contain special characters (non-ASCII) unless the operating system is configured with “Unicode UTF-8” support.

If the path to your home directory has spaces in it, then installation paths should be changed to something without a space, like `c:\esp\`. Also note that `ccache` uses a temporary directory in your home directory, and spaces in that path cause issues. `ccache` is enabled by default when running `export.ps1`, but it can be disabled by removing the following from `esp-idf/tools/tools.json`:
```
"export_vars": {
  "IDF_CCACHE_ENABLE": "1"
},
```

## Configuring a Windows Environment

The continuous integration for this project runs on a Windows instance. This means one can read [build-firmware-and-emulator.yml](https://github.com/AEFeinstein/Swadge-IDF-5.0/blob/main/.github/workflows/build-firmware-and-emulator.yml) to see how the Windows build environment is set up from scratch for both the firmware and emulator, though it does not install extra development tools. It is recommend to follow the following guide.

1. [Install `git`](https://git-scm.com/download/win).
1. [Install `python`](https://www.python.org/downloads/). Make sure to check "Add Python to environment variables" when installing.
1. [Install `doxygen`](https://www.doxygen.nl/download.html).
1. [Install `cppcheck`](https://cppcheck.sourceforge.io/).
1. [Install `msys2`](https://www.msys2.org/).
1. Start an `msys2` shell and run the following command to install all required packages:
    ```bash
    pacman --noconfirm -S base-devel mingw-w64-x86_64-gcc mingw-w64-x86_64-clang zip
    ```
1. Add the following paths to the Windows path variable. [Here are some instructions on how to do that](https://www.architectryan.com/2018/03/17/add-to-the-path-on-windows-10/).
    * `C:\msys64\mingw64\bin`
    * `C:\msys64\usr\bin`
    * `C:\Program Files\doxygen\bin` 
    * `C:\Program Files\Cppcheck`
    
    You must add the `msys2` paths **after** the `python` paths and **before** `C:\Windows\System32`. This is because the build uses Windows `python`, not msys2's, and it uses msys2 `find.exe`, not System32's. When it's all set up, it should look something like this:
    
    ![image](https://user-images.githubusercontent.com/231180/224911026-0c6b1063-e4f2-4671-a804-bce004085a3a.png)

1. Clone the ESP-IDF v5.0.1 and install the tools. Note that it will clone into `$HOME/esp/esp-idf`.
    ```powershell
    Set-ExecutionPolicy -Scope CurrentUser Unrestricted
    git clone -b v5.0.1 --recurse-submodules https://github.com/espressif/esp-idf.git $HOME/esp/esp-idf
    ~/esp/esp-idf/install.ps1
    ```
    **Warning**
    
    Sometimes `install.ps1` can be a bit finicky and not install everything it's supposed to. If it doesn't create a `$HOME/.espressif/python_env` folder, try running a few more times. As a last resort you can try editing `install.ps1` and swap the `"Setting up Python environment"` and `"Installing ESP-IDF tools"` sections to set up the Python environment first.

## Configuring a Linux Environment

1. Run the following commands, depending on your package manager, to install all necessary packages:
    * `apt`:
        ```bash
        sudo apt install build-essential xorg-dev libx11-dev libxinerama-dev libxext-dev mesa-common-dev libglu1-mesa-dev libasound2-dev libpulse-dev libasan8 clang-format cppcheck doxygen python3 python3-pip python3-venv cmake
        ```
    * `dnf`:
        ```bash
        sudo dnf group install "C Development Tools and Libraries" "Development Tools"
        sudo dnf install libX11-devel libXinerama-devel libXext-devel mesa-libGLU-devel alsa-lib-devel pulseaudio-libs-devel libudev-devel cmake libasan8 clang-format cppcheck doxygen python3 python3-pip python3-venv cmake
        ```
1. Clone the ESP-IDF v5.0.1 and install the tools. Note that it will clone into `~/esp/esp-idf`.
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
1. Make sure the ESP-IDF symbols are exported. This example is for Windows, so the actual command may be different for your OS. Note that `export.ps1` does not make any permanent changes and it must be run each time you open a new terminal for a build.
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
* [Espressif IDF](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) - Integration with ESP-IDF. When setting this up for the first time, point it at ESP-IDF which was previously installed. Do not let it install a second copy. Remember that ESP-IDF should exist in `~/esp/esp-idf` and the tools should exist in `~/.espressif/`.
* [C/C++ Advanced Lint](https://marketplace.visualstudio.com/items?itemName=jbenden.c-cpp-flylint) - Integration with `cppcheck`
* [Clang-Format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) - Integration with `clang-format`
* [Doxygen Documentation Generator](https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen) - Integration with `doxygen`
* [Todo Tree](https://marketplace.visualstudio.com/items?itemName=Gruntfuggly.todo-tree) - Handy to track "to-do items"

The `.vscode` folder already has tasks for making and cleaning the emulator. It also has launch settings for launching the emulator with `gdb` attached. To build the firmware from VSCode, use the Espressif extension buttons on the bottom toolbar. The build icon looks like a cylinder. Hover over the other icons to see what they do.

If VSCode isn't finding ESP-IDF symbols, try running the `export.ps1` script from a terminal, then launching code from that same session. For convenience, you can use a small script which exports the ESP-IDF symbols and launches VSCode.

`vsc_esp.sh`:
```bash
~/esp/esp-idf/export.ps1
code ~/esp/Swadge-IDF-5.0
```

## Updating ESP-IDF

On occasion the ESP-IDF version used to build this project will increment. The easiest way to update ESP-IDF is to delete the existing one, by default installed at `~/esp/esp-idf/`, and the tools, by default installed at `~/.espressif/`, and follow the guide above to clone the new ESP-IDF and run the install script.