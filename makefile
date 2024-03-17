# Makefile by Adam, 2022

################################################################################
# What OS we're compiling on
################################################################################

ifeq ($(OS),Windows_NT)
    HOST_OS = Windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        HOST_OS = Linux
    else ifeq ($(UNAME_S),Darwin)
        HOST_OS = Darwin
    endif
endif

################################################################################
# Programs to use
################################################################################

ifeq ($(HOST_OS),Windows)
	CC = x86_64-w64-mingw32-gcc.exe
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

################################################################################
# Source Files
################################################################################

# This is a list of directories to scan for c files recursively
SRC_DIRS_RECURSIVE = emulator main
# This is a list of directories to scan for c files not recursively
SRC_DIRS_FLAT = 
# This is a list of files to compile directly. There's no scanning here
SRC_FILES = 
# This is all the source directories combined
SRC_DIRS = $(shell $(FIND) $(SRC_DIRS_RECURSIVE) -type d) $(SRC_DIRS_FLAT)
# This is all the source files combined
SOURCES   = $(shell $(FIND) $(SRC_DIRS) -maxdepth 1 -iname "*.[c]") $(SRC_FILES)

# The emulator doesn't build components, but there is a target for formatting them
ALL_FILES = $(shell $(FIND) components $(SRC_DIRS_RECURSIVE) -iname "*.[c|h]" -not -name "rawdraw_sf.h" -not -name "getopt_win.h" -not -name "cJSON*")

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
	-static-libstdc++ \
	-fdiagnostics-color=always \
	-ffunction-sections \
	-fdata-sections \
	-gdwarf-4 \
	-ggdb \
	-O2 \
	-fno-jump-tables \
	-finline-functions \
	-std=gnu17

ifneq ($(HOST_OS),Darwin)
# Incompatible flags for clang on MacOS
CFLAGS += \
	-static-libgcc \
	-fstrict-volatile-bitfields \
	-fno-tree-switch-conversion
else
# Required for OpenGL and some other libraries
CFLAGS += \
	-I/opt/X11/include \
	-I/opt/homebrew/include
endif

ifeq ($(HOST_OS),Linux)
CFLAGS += \
	-fsanitize=address \
	-fsanitize=bounds-strict \
	-fno-omit-frame-pointer

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
	-Wno-error=unused-but-set-variable \
	-Wno-old-style-declaration

# These are warning flags that I like
CFLAGS_WARNINGS_EXTRA = \
	-Wundef \
	-Wformat=2 \
	-Winvalid-pch \
	-Wlogical-op \
	-Wmissing-format-attribute \
	-Wmissing-include-dirs \
	-Wpointer-arith \
	-Wunused-local-typedefs \
	-Wuninitialized \
	-Wshadow \
	-Wredundant-decls \
	-Wjump-misses-init \
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

################################################################################
# Defines
################################################################################

# Create a variable with the git hash and branch name
GIT_HASH  = \"$(shell git rev-parse --short=7 HEAD)\"

# Used by the ESP SDK
DEFINES_LIST = \
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
	IDF_VER="v5.2.1" \
	ESP_PLATFORM \
	_POSIX_READER_WRITER_LOCKS \
	CFG_TUSB_MCU=OPT_MCU_ESP32S2 \
	CONFIG_SOUND_OUTPUT_SPEAKER=y

# Extra defines
DEFINES_LIST += \
	EMULATOR=1 \
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
    LIBS = m X11 GL pulse pthread Xext Xinerama
endif

# These are directories to look for library files in
LIB_DIRS = 

# On MacOS we need to ensure that X11 is added for OpenGL and some others
ifeq ($(HOST_OS),Darwin)
    LIB_DIRS = /opt/X11/lib /opt/homebrew/lib
endif

# This combines the flags for the linker to find and use libraries
LIBRARY_FLAGS = $(patsubst %, -L%, $(LIB_DIRS)) $(patsubst %, -l%, $(LIBS)) \
	-static-libstdc++ \
	-ggdb

# Incompatible flags for clang on MacOS
ifneq ($(HOST_OS),Darwin)
LIBRARY_FLAGS += \
	-static-libgcc
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

################################################################################
# Build Filenames
################################################################################

# These are the files to build
EXECUTABLE = swadge_emulator

################################################################################
# Targets for Building
################################################################################

# This list of targets do not build files which match their name
.PHONY: all assets clean docs format cppcheck firmware clean-firmware print-%

# Build everything!
all: $(EXECUTABLE) assets

assets:
	$(MAKE) -C ./tools/spiffs_file_preprocessor/
	./tools/spiffs_file_preprocessor/spiffs_file_preprocessor -i ./assets/ -o ./spiffs_image/

# To build the main file, you have to compile the objects
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBRARY_FLAGS) -o $@

# This compiles each c file into an o file
./$(OBJ_DIR)/%.o: ./%.c
	@mkdir -p $(@D) # This creates a directory before building an object in it.
	$(CC) $(CFLAGS) $(CFLAGS_WARNINGS) $(CFLAGS_WARNINGS_EXTRA) $(DEFINES) $(INC) $< -o $@

# This cleans emulator files
clean:
	$(MAKE) -C ./tools/spiffs_file_preprocessor/ clean
	-@rm -f $(OBJECTS) $(EXECUTABLE)
	-@rm -rf ./docs/html
	-@rm -rf ./spiffs_image/*

# This cleans everything
fullclean: clean
	idf.py fullclean
	git clean -fX
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
	$(MAKE) -C ./tools/spiffs_file_preprocessor/ clean
	-@rm -rf ./docs/html
	-@rm -rf ./spiffs_image/*

firmware:
	idf.py build

# For now, works on Linux.  You can copy-paste these for Windows.

ifeq ($(HOST_OS),Windows)
usbflash :
	tools/reflash_and_monitor.bat
else
usbflash :
	$(MAKE) -C tools/reboot_into_bootloader
	sleep 1.2
	idf.py flash
	sleep 1.2
	$(MAKE) -C tools/bootload_reboot_stub reboot
	sleep 2.5
	$(MAKE) -C tools/swadgeterm monitor
endif

monitor :
	$(MAKE) -C tools/swadgeterm monitor

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
	emulator/src/rawdraw_sf.h \
	emulator/src/getopt_win.h \
	emulator/sound \
	emulator/src/components/hdw-nvs/cJSON.c \
	emulator/src/components/hdw-nvs/cJSON.h \
	main/asset_loaders/heatshrink_common.h \
	main/asset_loaders/heatshrink_config.h \
	main/asset_loaders/heatshrink_decoder.c \
	main/asset_loaders/heatshrink_decoder.h \
	main/asset_loaders/heatshrink_helper.c \
	main/asset_loaders/heatshrink_helper.h

CPPCHECK_IGNORE_FLAGS = $(patsubst %,-i%, $(CPPCHECK_IGNORE))

cppcheck:
	cppcheck $(CPPCHECK_FLAGS) $(DEFINES) $(INC) $(CPPCHECK_DIRS) $(CPPCHECK_IGNORE_FLAGS)

gen-coverage:
	lcov --capture --directory ./emulator/obj/ --output-file ./coverage.info
	genhtml ./coverage.info --output-directory ./coverage
	firefox ./coverage/index.html &

# Print any value from this makefile
print-%  : ; @echo $* = $($*)
