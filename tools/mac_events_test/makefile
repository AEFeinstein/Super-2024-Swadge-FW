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

CC = gcc

FIND:=find


################################################################################
# Source Files
################################################################################

SOURCES   := main.c

################################################################################
# Includes
################################################################################

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

# Required for OpenGL and some other libraries
CFLAGS += \
	-mmacosx-version-min=10.0

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

################################################################################
# Defines
################################################################################

# Create a variable with the git hash and branch name
GIT_HASH  = \"$(shell git rev-parse --short=7 HEAD)\"

# Used by the ESP SDK
DEFINES_LIST = \
	_GNU_SOURCE \
	_POSIX_READER_WRITER_LOCKS

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

#LIBS = m X11 GL pthread Xext Xinerama
LIBS =

# These are directories to look for library files in
LIB_DIRS =

# On MacOS we need to ensure that X11 is added for OpenGL and some others
#ifeq ($(HOST_OS),Darwin)
#    LIB_DIRS = /opt/X11/lib
#endif

# This combines the flags for the linker to find and use libraries
LIBRARY_FLAGS = $(patsubst %, -L%, $(LIB_DIRS)) $(patsubst %, -l%, $(LIBS)) \
	-ggdb

LIBRARY_FLAGS += \
    -framework Carbon

################################################################################
# Build Filenames
################################################################################

# These are the files to build
EXECUTABLE = test
BUNDLE = test.app
ICONS = test.icns

################################################################################
# Targets for Building
################################################################################

# This list of targets do not build files which match their name
.PHONY: all bundle clean print-%

# Build the executable
all: $(EXECUTABLE)

# To build the main file, you have to compile the objects
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBRARY_FLAGS) -o $@

# This compiles each c file into an o file
./$(OBJ_DIR)/%.o: ./%.c
	@mkdir -p $(@D) # This creates a directory before building an object in it.
	$(CC) $(CFLAGS) $(CFLAGS_WARNINGS) $(CFLAGS_WARNINGS_EXTRA) $(DEFINES) $(INC) $< -o $@

bundle: $(BUNDLE)

$(BUNDLE): $(EXECUTABLE) $(ICONS) Info.plist
	rm -rf $(BUNDLE)
	mkdir -p $(BUNDLE)/Contents/{MacOS,Resources,libs}
	cp Info.plist $(BUNDLE)/Contents/Info.plist
	echo "APPLTest" > $(BUNDLE)/Contents/PkgInfo
	cp $(ICONS) $(BUNDLE)/Contents/Resources/
	vtool -set-build-version macos 10.0 10.0 -replace -output $(BUNDLE)/Contents/MacOS/test $(EXECUTABLE)
	dylibbundler -od -b -x ./$(BUNDLE)/Contents/MacOS/test -d ./$(BUNDLE)/Contents/libs/


$(ICONS): ../../emulator/resources/icon.png
	rm -rf test.iconset
	mkdir -p test.iconset
	sips -z 16 16     $< --out test.iconset/icon_16x16.png
	sips -z 32 32     $< --out test.iconset/icon_16x16@2x.png
	sips -z 32 32     $< --out test.iconset/icon_32x32.png
	sips -z 64 64     $< --out test.iconset/icon_32x32@2x.png
	sips -z 128 128   $< --out test.iconset/icon_128x128.png
	sips -z 256 256   $< --out test.iconset/icon_128x128@2x.png
	sips -z 256 256   $< --out test.iconset/icon_256x256.png
	sips -z 512 512   $< --out test.iconset/icon_256x256@2x.png
	sips -z 512 512   $< --out test.iconset/icon_512x512.png
	sips -z 1024 1024 $< --out test.iconset/icon_512x512@2x.png
	iconutil -c icns -o $(ICONS) test.iconset
	rm -r test.iconset

# This cleans emulator files
clean:
	-@rm -rf $(OBJECTS) $(EXECUTABLE) $(ICONS) test.iconset $(BUNDLE)


################################################################################
# Firmware targets
################################################################################

# Print any value from this makefile
print-%  : ; @echo $* = $($*)
