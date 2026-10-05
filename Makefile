# specifes the directory to place the build files.
OUT_DIR = build
INSTALL_DIR = bin
DIST_DIR = dist

#CC specifies which C compiler we're using
CC = gcc

#CPPC specifies which C++ compiler we're using
CPPC = g++

XCOMP = no
TAG = ""
NIGHTLY = no

ifeq ($(OS), Windows_NT)
	ifeq ($(XCOMP), yes)
		FIND = find
	else
		FIND = /bin/find
	endif
else
	FIND = find
endif

#OBJS specifies which files to compile as part of the project
SRCS := $(filter-out %example_implot.cpp, $(shell $(FIND) src -name "*.cpp"))
OBJS = $(SRCS:%=$(OUT_DIR)/%.o)
NATIVE_SRCS = src/tinyfd/tinyfiledialogs.c src/whereami/whereami.c
NATIVE_OBJS = $(NATIVE_SRCS:%=$(OUT_DIR)/%.o)

#BIN_NAME specifies the name of our exectuable
BIN_NAME = MyApp
ZIP_NAME = GTE_$(OS).zip

EXTRA_INCLUDES = -Isrc/imgui -Isrc/imgui/backends -Isrc/imgui/ext/implot -Isrc/whereami

ifeq ($(NIGHTLY), yes)
	TAG = _$(shell date '+%Y%m%d')
endif

OS ?= $(shell uname)

ifneq ($(origin WINDOW_TITLE), undefined)
	DEFINES = -D WINDOW_TITLE="\"$(WINDOW_TITLE)\""
endif

ifeq ($(OS), Windows_NT)
	ifeq ($(XCOMP), yes)
		CC = i686-w64-mingw32-gcc-posix
		CPPC = i686-w64-mingw32-g++-posix
	endif
	BIN_NAME := $(BIN_NAME).exe

	ZIP_NAME = bin/GTE_Win32$(TAG).zip
	SDL_ROOT = ../SDL2-2.26.2/x86_64-w64-mingw32

	#INCLUDE_PATHS specifies the additional include paths we'll need
	INCLUDE_PATHS = -I$(SDL_ROOT)/include/SDL2 $(EXTRA_INCLUDES)

	#LIBRARY_PATHS specifies the additional library paths we'll need
	LIBRARY_PATHS = -L$(SDL_ROOT)/lib

	OBJS += $(NATIVE_OBJS)

	#COMPILER_FLAGS specifies the additional compilation options we're using
	# -Wl,-subsystem,windows gets rid of the console window
	# change subsystem,windows to subsystem,console to get printfs on command line
	COMPILER_FLAGS = -Wl,-subsystem,console
	DEFINES += -D _WIN32

	#LINKER_FLAGS specifies the libraries we're linking against
	LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -Wl,-Bstatic -mwindows -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lcomdlg32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -lsetupapi
else
	OBJS += $(NATIVE_OBJS)
	COMPILER_FLAGS = -g `sdl2-config --cflags` $(EXTRA_INCLUDES)
	LINKER_FLAGS = `sdl2-config --libs`
endif


DEFINES += -D CPU_6502_STATIC -D CPU_6502_USE_LOCAL_HEADER -D CMOS_INDIRECT_JMP_FIX

#This is the target that compiles our executable
.PHONY: all bin dist install
all: bin dist

bin: $(OUT_DIR)/$(BIN_NAME)
dist: $(OUT_DIR)/$(ZIP_NAME)
	@mkdir -p $(DIST_DIR)
	cp $^ $(DIST_DIR)

install: bin
	@mkdir -p $(INSTALL_DIR)/bin
	install -t $(INSTALL_DIR)/bin $(OUT_DIR)/$(BIN_NAME)
ifeq ($(OS), Windows_NT)
	install -t $(INSTALL_DIR)/bin $(SDL_ROOT)/bin/SDL2.dll
endif

$(OUT_DIR)/$(ZIP_NAME): bin
	@mkdir -p $(@D)/img
ifeq ($(OS), Windows_NT)
	cp $(SDL_ROOT)/bin/SDL2.dll $(OUT_DIR)
endif
	cd $(OUT_DIR); zip -9 -y -r -q $(ZIP_NAME) $(BIN_NAME) SDL2.dll img commit_hash.txt

$(OUT_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(CPPC) -c $< -o $@ $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(DEFINES) -std=c++20

$(OUT_DIR)/%.c.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $< -o $@ $(INCLUDE_PATHS) $(COMPILER_FLAGS) $(DEFINES)

$(OUT_DIR)/$(BIN_NAME): $(OBJS)
	$(CPPC) $(COMPILER_FLAGS) -o $@ $^ $(LIBRARY_PATHS) $(LINKER_FLAGS) -std=c++20

clean:
	rm -rf $(OUT_DIR)

clean-all: clean
	rm -rf $(INSTALL_DIR)
	rm -rf $(DIST_DIR)
