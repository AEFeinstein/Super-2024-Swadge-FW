# Makefile by Adam, 2022

################################################################################
# What OS we're compiling on
################################################################################

IS_WSL := 0
ifeq ($(OS),Windows_NT)
    HOST_OS = Windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        HOST_OS = Linux
		# Check if this is WSL. 0 for not WSL, 1 for WSL
	    IS_WSL := $(shell uname -a | grep -i WSL | wc -l)
    else ifeq ($(UNAME_S),Darwin)
        HOST_OS = Darwin
    endif
endif

################################################################################
# Programs to use
################################################################################

ifeq ($(HOST_OS),Windows)
	CC = gcc
else ifeq ($(HOST_OS),Linux)
	CC = gcc
else ifeq ($(UNAME_S),Darwin)
	CC = gcc
endif

FIND:=find
ifeq ($(HOST_OS),Windows)
	FIND:=$(shell cygpath `where find | grep bin | grep -v " "`)
endif

# clang-format may actually be clang-format-17
CLANG_FORMAT:=clang-format
ifeq (, $(shell which $(CLANG_FORMAT)))
	CLANG_FORMAT:=clang-format-17
endif

ifeq ($(HOST_OS),Linux)
	ifneq (,$(shell getent group plugdev))
		UDEV_GROUP:=plugdev
	else
		UDEV_GROUP:=$(USER)
	endif
endif

################################################################################
# Source Files
################################################################################

CNFS_FILE = main/utils/cnfs_image.c

# This is a list of directories to scan for c files recursively
SRC_DIRS_RECURSIVE = emulator/src main
# This is a list of directories to scan for c files not recursively
SRC_DIRS_FLAT = emulator/src-lib
# This is a list of files to compile directly. There's no scanning here
# cnfs_image.c may not exist when the makefile is invoked, explicitly list it
SRC_FILES = $(CNFS_FILE)
# This is all the source directories combined
SRC_DIRS = $(shell $(FIND) $(SRC_DIRS_RECURSIVE) -type d) $(SRC_DIRS_FLAT)
# This is all the source files combined and deduplicated
SOURCES   = $(sort $(shell $(FIND) $(SRC_DIRS) -maxdepth 1 -iname "*.[c]") $(SRC_FILES))

# The emulator doesn't build components, but there is a target for formatting them
ALL_FILES = $(shell $(FIND) components $(SRC_DIRS_RECURSIVE) -iname "*.[c|h]")

################################################################################
# Includes
################################################################################

# Look for folders with .h files in these directories, recursively
INC_DIRS_RECURSIVE = emulator main
# Look for folders named "include" in these directories, recursively
INC_DIRS_INCLUDE = components
# Treat every source directory as one to search for headers in, also add a few more
INC_DIRS  = $(shell $(FIND) $(INC_DIRS_RECURSIVE) -type d)
INC_DIRS += $(shell $(FIND) $(INC_DIRS_INCLUDE) -type d -iname "include")
# Prefix the directories for gcc
INC = $(patsubst %, -I%, $(INC_DIRS) )

################################################################################
# Compiler Flags
################################################################################

# These are flags for the compiler, all files
CFLAGS = \
	-c \
	-g \
	-fdiagnostics-color=always \
	-ffunction-sections \
	-fdata-sections \
	-gdwarf-4 \
	-ggdb \
	-O2	\
	-fno-jump-tables \
	-finline-functions \
	-std=gnu17

ifneq ($(HOST_OS),Darwin)
# Incompatible flags for clang on MacOS
CFLAGS += \
	-static-libgcc \
	-static-libstdc++ \
	-fstrict-volatile-bitfields \
	-fno-tree-switch-conversion \
	-fno-omit-frame-pointer
else
# Required for OpenGL and some other libraries
CFLAGS += \
	-I/opt/X11/include \
	-I/opt/homebrew/include \
	-mmacosx-version-min=10.0
endif

ifeq ($(HOST_OS),Linux)
CFLAGS += \
	-fsanitize=address \
	-fsanitize=bounds-strict
ENABLE_GCOV=false

ifeq ($(ENABLE_GCOV),true)
    CFLAGS += -fprofile-arcs -ftest-coverage -DENABLE_GCOV
endif
endif

# These are warning flags that the IDF uses
CFLAGS_WARNINGS = \
	-Wall \
	-Werror=all \
	-Wno-error=unused-function \
	-Wno-error=unused-variable \
	-Wno-error=deprecated-declarations \
	-Wextra \
	-Wno-unused-parameter \
	-Wno-sign-compare \
	-Wno-enum-conversion \
	-Wno-error=unused-but-set-variable

# These are warning flags that I like
CFLAGS_WARNINGS_EXTRA = \
	-Wundef \
	-Wformat=2 \
	-Winvalid-pch \
	-Wmissing-format-attribute \
	-Wmissing-include-dirs \
	-Wpointer-arith \
	-Wunused-local-typedefs \
	-Wuninitialized \
	-Wshadow \
	-Wredundant-decls \
	-Wswitch \
	-Wcast-align \
	-Wformat-nonliteral \
	-Wno-switch-default \
	-Wunused \
	-Wunused-macros \
	-Wmissing-declarations \
	-Wmissing-prototypes \
	-Wcast-qual \
	-Wno-switch \
	-Wunused-result \
#	-Wstrict-prototypes \
#	-Wpedantic \
#	-Wconversion \
#	-Wsign-conversion \
#	-Wdouble-promotion

ifneq ($(HOST_OS),Darwin)
# Incompatible warnings for clang on MacOS
CFLAGS_WARNINGS += \
	-Wno-old-style-declaration

CFLAGS_WARNINGS_EXTRA += \
	-Wlogical-op \
	-Wjump-misses-init
endif

################################################################################
# Defines
################################################################################

# Create a variable with the git hash and branch name
GIT_HASH  = \"$(shell git rev-parse --short=7 HEAD)\"

# Used by the ESP SDK
DEFINES_LIST = \
	CONFIG_ESP_SYSTEM_PANIC=y\
	CONFIG_ESP_SYSTEM_GDBSTUB_RUNTIME=y\
	CONFIG_DEBUG_OUTPUT_USB=y\
	CONFIG_IDF_TARGET_ESP32S2=y \
	SOC_RMT_CHANNELS_PER_GROUP=4 \
	SOC_TOUCH_SENSOR_NUM=15 \
	SOC_ULP_SUPPORTED=y \
	SOC_PM_SUPPORT_EXT_WAKEUP=y \
	SOC_GPIO_SUPPORT_SLP_SWITCH=y \
	SOC_TIMER_GROUP_TIMERS_PER_GROUP=2 \
	SOC_TIMER_GROUPS=2 \
	SOC_I2C_NUM=2 \
	SOC_I2C_SUPPORT_SLAVE=y \
	SOC_LEDC_CHANNEL_NUM=8 \
	SOC_UART_NUM=2 \
	SOC_ADC_DIGI_RESULT_BYTES=2 \
	CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD=0 \
	CONFIG_LOG_MAXIMUM_LEVEL=3 \
	CONFIG_GC9307_240x280=y \
	CONFIG_TFT_MAX_BRIGHTNESS=200 \
	CONFIG_TFT_MIN_BRIGHTNESS=10 \
	CONFIG_NUM_LEDS=8 \
	configENABLE_FREERTOS_DEBUG_OCDAWARE=1 \
	_GNU_SOURCE \
	IDF_VER="v5.3.1" \
	ESP_PLATFORM \
	_POSIX_READER_WRITER_LOCKS \
	CFG_TUSB_MCU=OPT_MCU_ESP32S2 \
	CONFIG_SOUND_OUTPUT_SPEAKER=y

# If this is not WSL, use OpenGL for rawdraw
ifeq ($(IS_WSL),0)
	DEFINES_LIST += CNFGOGL
endif

# Extra defines
DEFINES_LIST += \
	GIT_SHA1=${GIT_HASH} \
	HAS_XINERAMA=1 \
	FULL_SCREEN_STEAL_FOCUS=1

DEFINES = $(patsubst %, -D%, $(DEFINES_LIST))

################################################################################
# Output Objects
################################################################################

# This is the directory in which object files will be stored
OBJ_DIR = emulator/obj

# This is a list of objects to build
OBJECTS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SOURCES))

################################################################################
# Linker options
################################################################################

# This is a list of libraries to include. Order doesn't matter

ifeq ($(HOST_OS),Windows)
    LIBS = opengl32 gdi32 user32 winmm WSock32
endif
ifeq ($(HOST_OS),Linux)
    LIBS = m X11 asound pulse rt GL GLX pthread Xext Xinerama
endif
ifeq ($(HOST_OS),Darwin)
    LIBS = m X11 GL pthread Xext Xinerama
endif

# These are directories to look for library files in
LIB_DIRS =

# On MacOS we need to ensure that X11 is added for OpenGL and some others
ifeq ($(HOST_OS),Darwin)
    LIB_DIRS = /opt/X11/lib /opt/homebrew/lib
endif

# This combines the flags for the linker to find and use libraries
LIBRARY_FLAGS = $(patsubst %, -L%, $(LIB_DIRS)) $(patsubst %, -l%, $(LIBS)) \
	-ggdb \
	-Wl,--wrap=cnfsGetFile \
	-Wl,--wrap=cnfsReadFile

# Incompatible flags for clang on MacOS
ifneq ($(HOST_OS),Darwin)
LIBRARY_FLAGS += \
	-static-libgcc \
	-static-libstdc++
else
LIBRARY_FLAGS += \
    -framework Foundation \
	-framework CoreFoundation \
	-framework CoreMIDI \
	-framework AudioToolbox
endif

ifeq ($(HOST_OS),Linux)
LIBRARY_FLAGS += \
	-fsanitize=address \
	-fsanitize=bounds-strict \
	-fno-omit-frame-pointer \
	-static-libasan
ifeq ($(ENABLE_GCOV),true)
    LIBRARY_FLAGS += -lgcov -fprofile-arcs -ftest-coverage
endif
endif

ifeq ($(HOST_OS),Windows)
	LIBRARY_FLAGS += -Wl,-Bstatic -lpthread
endif

################################################################################
# Build Filenames
################################################################################

# These are the files to build
EXECUTABLE = swadge_emulator

################################################################################
# Targets for Building
################################################################################

# This list of targets do not build files which match their name
.PHONY: all assets bundle clean docs format cppcheck firmware clean-firmware $(CNFS_FILE) print-%

# Build the executable
all: $(EXECUTABLE)

assets:
	$(MAKE) -C ./tools/assets_preprocessor/
	./tools/assets_preprocessor/assets_preprocessor -i ./assets/ -o ./assets_image/

# To build the main file, you have to compile the objects
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBRARY_FLAGS) -o $@

# This compiles each c file into an o file
./$(OBJ_DIR)/%.o: ./%.c
	@mkdir -p $(@D) # This creates a directory before building an object in it.
	$(CC) $(CFLAGS) $(CFLAGS_WARNINGS) $(CFLAGS_WARNINGS_EXTRA) $(DEFINES) $(INC) $< -o $@

# To create the c file with assets, run these tools
$(CNFS_FILE):
	$(MAKE) -C ./tools/assets_preprocessor/
	./tools/assets_preprocessor/assets_preprocessor -i ./assets/ -o ./assets_image/
	$(MAKE) -C ./tools/cnfs/
	./tools/cnfs/cnfs_gen assets_image/ main/utils/cnfs_image.c main/utils/cnfs_image.h

bundle: SwadgeEmulator.app

SwadgeEmulator.app: $(EXECUTABLE) build/SwadgeEmulator.icns emulator/resources/Info.plist
	rm -rf SwadgeEmulator.app
	mkdir -p SwadgeEmulator.app/Contents/{MacOS,Resources,libs}
	cat emulator/resources/Info.plist | sed "s/##GIT_HASH##/$(GIT_HASH)/" > SwadgeEmulator.app/Contents/Info.plist
	echo "APPLSwadgeEmulator" > SwadgeEmulator.app/Contents/PkgInfo
	cp build/SwadgeEmulator.icns SwadgeEmulator.app/Contents/Resources/
	vtool -set-build-version macos 10.0 10.0 -replace -output SwadgeEmulator.app/Contents/MacOS/SwadgeEmulator $(EXECUTABLE)
	dylibbundler -od -b -x ./SwadgeEmulator.app/Contents/MacOS/SwadgeEmulator -d ./SwadgeEmulator.app/Contents/libs/


build/SwadgeEmulator.icns: emulator/resources/icon.png
	rm -rf build/SwadgeEmulator.iconset
	mkdir -p build/SwadgeEmulator.iconset
	sips -z 16 16     $< --out build/SwadgeEmulator.iconset/icon_16x16.png
	sips -z 32 32     $< --out build/SwadgeEmulator.iconset/icon_16x16@2x.png
	sips -z 32 32     $< --out build/SwadgeEmulator.iconset/icon_32x32.png
	sips -z 64 64     $< --out build/SwadgeEmulator.iconset/icon_32x32@2x.png
	sips -z 128 128   $< --out build/SwadgeEmulator.iconset/icon_128x128.png
	sips -z 256 256   $< --out build/SwadgeEmulator.iconset/icon_128x128@2x.png
	sips -z 256 256   $< --out build/SwadgeEmulator.iconset/icon_256x256.png
	sips -z 512 512   $< --out build/SwadgeEmulator.iconset/icon_256x256@2x.png
	sips -z 512 512   $< --out build/SwadgeEmulator.iconset/icon_512x512.png
	sips -z 1024 1024 $< --out build/SwadgeEmulator.iconset/icon_512x512@2x.png
	iconutil -c icns -o build/SwadgeEmulator.icns build/SwadgeEmulator.iconset
	rm -r build/SwadgeEmulator.iconset

# This cleans emulator files
clean:
	$(MAKE) -C ./tools/assets_preprocessor/ clean
	$(MAKE) -C ./tools/cnfs clean
	-@rm -f $(OBJECTS) $(EXECUTABLE)
	-@rm -rf ./docs/html
	-@rm -rf ./main/utils/cnfs/cnfs_image.c

# This cleans everything
fullclean: clean
	idf.py fullclean
	git clean -dfX
	git clean -df
	git clean -fX
	git clean -f
	$(MAKE) -C ./tools/sandbox_test clean
	$(MAKE) -C ./tools/hidapi_test clean
	$(MAKE) -C ./tools/bootload_reboot_stub clean
	$(MAKE) -C ./tools/font_maker clean
	$(MAKE) -C ./tools/swadgeterm clean
	$(MAKE) -C ./tools/reboot_into_bootloader clean

################################################################################
# Utility targets
################################################################################

docs:
	-wget -nc -O plantuml.jar https://github.com/plantuml/plantuml/releases/download/v1.2023.4/plantuml-1.2023.4.jar
	doxygen ./Doxyfile

format:
	$(CLANG_FORMAT) -i -style=file $(ALL_FILES)

################################################################################
# Firmware targets
################################################################################

clean-firmware:
	idf.py clean
	$(MAKE) -C ./tools/assets_preprocessor/ clean
	-@rm -rf ./docs/html
	-@rm -rf ./assets_image/*

firmware:
	idf.py build

# For now, works on Linux.  You can copy-paste these for Windows.

ifeq ($(HOST_OS),Windows)
usbflash :
	tools/reflash_and_monitor.bat
else
usbflash :
	# In case we are already in the bootloader...
	($(MAKE) -C tools/bootload_reboot_stub reboot)||(true)
	# Command reboot out of game into bootloader.
	$(MAKE) -C tools/reboot_into_bootloader
	idf.py flash
	sleep 1.2
	$(MAKE) -C tools/bootload_reboot_stub reboot
	$(MAKE) -C tools/swadgeterm monitor
endif

monitor :
	$(MAKE) -C tools/swadgeterm monitor

/etc/udev/rules.d/99-swadge.rules :
	printf "KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", MODE=\"0664\", GROUP=\"%s\", ATTRS{idVendor}==\"1209\", ATTRS{idProduct}==\"4269\"\n" $(UDEV_GROUP) > /tmp/99-swadge.rules
	printf "KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"1209\", ATTRS{idProduct}==\"4269\", GROUP=\"%s\", MODE=\"0660\"\n" $(UDEV_GROUP) >> /tmp/99-swadge.rules
	printf "KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", MODE=\"0664\", GROUP=\"%s\", ATTRS{idVendor}==\"303a\", ATTRS{idProduct}==\"00??\"\n" $(UDEV_GROUP) >> /tmp/99-swadge.rules
	printf "KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"303a\", ATTRS{idProduct}==\"00??\", GROUP=\"%s\", MODE=\"0660\"\n" $(UDEV_GROUP) >> /tmp/99-swadge.rules
	sudo cp -a /tmp/99-swadge.rules /etc/udev/rules.d/99-swadge.rules

installudev : /etc/udev/rules.d/99-swadge.rules
	getent group plugdev >/dev/null && sudo usermod -aG plugdev $(USER) || true
	sudo udevadm control --reload
	sudo udevadm trigger

################################################################################
# cppcheck targets
################################################################################

CPPCHECK_FLAGS= \
	--enable=warning \
	--inconclusive \
	--library=posix \
	--language=c \
	--platform=unix32 \
	--std=c++17 \
	--suppress=missingIncludeSystem \
	--output-file=./cppcheck_result.txt \
	-j12 \
	-D__linux__=1

CPPCHECK_DIRS= \
	main \
	components \
	emulator/src

CPPCHECK_IGNORE= \
	$(shell $(FIND) emulator/src-lib -type f) \
	$(shell $(FIND) main/asset_loaders -type f -iname "*heatshrink*")

CPPCHECK_IGNORE_FLAGS = $(patsubst %,-i%, $(CPPCHECK_IGNORE))

cppcheck:
	cppcheck $(CPPCHECK_FLAGS) $(DEFINES) $(INC) $(CPPCHECK_DIRS) $(CPPCHECK_IGNORE_FLAGS)

gen-coverage:
	lcov --capture --directory ./emulator/obj/ --output-file ./coverage.info
	genhtml ./coverage.info --output-directory ./coverage
	firefox ./coverage/index.html &

# Print any value from this makefile
print-%  : ; @echo $* = $($*)
