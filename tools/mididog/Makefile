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

################################################################################
# Source Files
################################################################################



# This is a list of directories to scan for c files recursively
SRC_DIRS_RECURSIVE = src
# This is a list of directories to scan for c files not recursively
SRC_DIRS_FLAT =
# This is a list of files to compile directly. There's no scanning here
SRC_FILES = ../../main/midi/drums.c \
			../../main/utils/fp_math.c \
			../../main/midi/midiData.c \
			../../main/midi/midiFileParser.c \
			../../main/midi/midiPlayer.c \
			../../main/midi/midiUtil.c \
			../../main/utils/swSynth.c \
			../../main/midi/waveTables.c \
# This is all the source directories combined
SRC_DIRS = $(shell $(FIND) $(SRC_DIRS_RECURSIVE) -type d) $(SRC_DIRS_FLAT)
# This is all the source files combined and deduplicated
SOURCES   = $(sort $(shell $(FIND) $(SRC_DIRS) -maxdepth 1 -iname "*.[c]") $(SRC_FILES))

# The emulator doesn't build components, but there is a target for formatting them
ALL_FILES = $(shell $(FIND) components $(SRC_DIRS_RECURSIVE) -iname "*.[c|h]")

SUBMODULES = $(shell git config --file .gitmodules --name-only --get-regexp path | sed -nr 's/submodule.(.*).path/\1/p')

################################################################################
# Includes
################################################################################

# Look for folders with .h files in these directories, recursively
INC_DIRS_RECURSIVE = ../../main
# Look for folders named "include" in these directories, recursively
INC_DIRS_INCLUDE =
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
	MIDI_STANDALONE \
	_GNU_SOURCE \
	_POSIX_READER_WRITER_LOCKS

# If this is not WSL, use OpenGL for rawdraw
ifeq ($(IS_WSL),0)
	DEFINES_LIST += CNFGOGL
endif

# Extra defines
DEFINES_LIST += \
	GIT_SHA1=${GIT_HASH} \
	MIDI_STANDALONE=1

DEFINES = $(patsubst %, -D%, $(DEFINES_LIST))

################################################################################
# Output Objects
################################################################################

# This is the directory in which object files will be stored
OBJ_DIR = obj

# This is a list of objects to build
OBJECTS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SOURCES))

################################################################################
# Linker options
################################################################################

# This is a list of libraries to include. Order doesn't matter

ifeq ($(HOST_OS),Windows)
    LIBS =
    #LIBS = opengl32 gdi32 user32 winmm WSock32
endif
ifeq ($(HOST_OS),Linux)
    LIBS = m
    #LIBS = m X11 asound pulse rt GL GLX pthread Xext Xinerama
endif
ifeq ($(HOST_OS),Darwin)
	LIBS = m
    #LIBS = m X11 GL pthread Xext Xinerama
endif

# These are directories to look for library files in
LIB_DIRS =

# On MacOS we need to ensure that X11 is added for OpenGL and some others
ifeq ($(HOST_OS),Darwin)
	LIB_DIRS = /opt/homebrew/lib
    #LIB_DIRS = /opt/X11/lib /opt/homebrew/lib
endif

# This combines the flags for the linker to find and use libraries
LIBRARY_FLAGS = $(patsubst %, -L%, $(LIB_DIRS)) $(patsubst %, -l%, $(LIBS)) \
	-ggdb

# Incompatible flags for clang on MacOS
ifneq ($(HOST_OS),Darwin)
LIBRARY_FLAGS += \
	-static-libgcc \
	-static-libstdc++
else
#LIBRARY_FLAGS += \
    -framework Carbon \
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
	LIBRARY_FLAGS += -Wl,-Bstatic
	#LIBRARY_FLAGS += -Wl,-Bstatic -lpthread
endif

################################################################################
# Build Filenames
################################################################################

# These are the files to build
EXECUTABLE = mididog

################################################################################
# Targets for Building
################################################################################

# This list of targets do not build files which match their name
.PHONY: all clean print-%

# Build the executable
all: $(EXECUTABLE)

# To build the main file, you have to compile the objects
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBRARY_FLAGS) -o $@

# This compiles each c file into an o file
./$(OBJ_DIR)/%.o: ./%.c
	@mkdir -p $(@D) # This creates a directory before building an object in it.
	$(CC) $(CFLAGS) $(CFLAGS_WARNINGS) $(CFLAGS_WARNINGS_EXTRA) $(DEFINES) $(INC) $< -o $@

# This cleans emulator files
clean:
	-@rm -f $(OBJECTS) $(EXECUTABLE)

# Print any value from this makefile
print-%  : ; @echo $* = $($*)
