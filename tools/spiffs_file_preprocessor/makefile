# Makefile by Adam, 2023

################################################################################
# Programs to use
################################################################################

CC = gcc
FIND = find

################################################################################
# Source Files
################################################################################

# This is a list of directories to scan for c files recursively
SRC_DIRS_RECURSIVE = ./src ./src-lib
# This is a list of directories to scan for c files not recursively
SRC_DIRS_FLAT = 
# This is a list of files to compile directly. There's no scanning here
SRC_FILES = 
# This is all the source directories combined
SRC_DIRS = $(shell $(FIND) $(SRC_DIRS_RECURSIVE) -type d) $(SRC_DIRS_FLAT)
# This is all the source files combined
SOURCES   = $(shell $(FIND) $(SRC_DIRS) -maxdepth 1 -iname "*.[c]") $(SRC_FILES)

# This is a list of all source files to format
SOURCES_TO_FORMAT = $(shell $(FIND) ./src -iname "*.[c|h]")

################################################################################
# Compiler Flags
################################################################################

# These are flags for the compiler, all files
CFLAGS = -g -std=c99

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
	-Wno-error=unused-but-set-variable \
	-Wno-old-style-declaration \
	-Wno-missing-field-initializers
	
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
	-Wswitch-enum \
	-Wcast-align \
	-Wformat-nonliteral \
	-Wno-switch-default \
	-Wunused \
	-Wunused-macros \
	-Wmissing-declarations \
	-Wmissing-prototypes \
	-Wcast-qual \
	-Wno-switch \
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

# Defines for all files
DEFINES_LIST = #CONFIG_GC9307_240x280=y
DEFINES = $(patsubst %, -D%, $(DEFINES_LIST))

################################################################################
# Includes
################################################################################

# Look for folders with .h files in these directories, recursively
INC_DIRS_RECURSIVE = ./src
# Treat every source directory as one to search for headers in, also add a few more
INC_DIRS = $(SRC_DIRS) $(shell $(FIND) $(INC_DIRS_RECURSIVE) -type d)
# Prefix the directories for gcc
INC = $(patsubst %, -I%, $(INC_DIRS) )

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
LIBS = m

# These are directories to look for library files in
LIB_DIRS = 

# This combines the flags for the linker to find and use libraries
LIBRARY_FLAGS = $(patsubst %, -L%, $(LIB_DIRS)) $(patsubst %, -l%, $(LIBS)) \
	-static-libgcc \
	-static-libstdc++ \
	-ggdb

################################################################################
# Build Filenames
################################################################################

# These are the files to build
EXECUTABLE = spiffs_file_preprocessor

################################################################################
# Targets for Building
################################################################################

# This list of targets do not build files which match their name
.PHONY: all clean format print-%

# Build everything!
all: $(EXECUTABLE)

# To build the main file, you have to compile the objects
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBRARY_FLAGS) -o $@

# This compiles each c file into an o file
./$(OBJ_DIR)/%.o: ./%.c
	@mkdir -p $(@D) # This creates a directory before building an object in it.
	$(CC) $(CFLAGS) $(CFLAGS_WARNINGS) $(CFLAGS_WARNINGS_EXTRA) $(DEFINES) $(INC) -c $< -o $@

# This clean everything
clean:
	-@rm -f $(OBJECTS) $(EXECUTABLE)

format:
	clang-format -i -style=file $(SOURCES_TO_FORMAT)

################################################################################
# Makefile Debugging
################################################################################

# Print any value from this makefile
print-%  : ; @echo $* = $($*)
