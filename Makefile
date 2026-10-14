# sdl3dviewer - build with GNU make.
#   Linux:   make            (needs SDL3 dev package + pkg-config)
#   Windows: make            (from an MSYS2 UCRT64 shell)
#
# Targets: all (default, debug build), release, run, clean

ifeq ($(origin CC),default)
CC := gcc
endif
PKG_CONFIG ?= pkg-config

TARGET   := sdl3dviewer
BUILD    ?= debug
BUILDDIR := build/$(BUILD)

SRC       := $(wildcard src/*.c)
GLAD_SRC  := external/glad/src/gl.c
OBJ       := $(SRC:src/%.c=$(BUILDDIR)/%.o) $(BUILDDIR)/glad.o
DEP       := $(OBJ:.o=.d)

SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl3)
SDL_LIBS   := $(shell $(PKG_CONFIG) --libs sdl3)

CPPFLAGS := -Iexternal/glad/include $(SDL_CFLAGS) -MMD -MP
CFLAGS   := -std=c11
WARN     := -Wall -Wextra
LDLIBS   := $(SDL_LIBS) -lm

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

ifeq ($(BUILD),release)
CFLAGS  += -O2 -DNDEBUG
ifeq ($(OS),Windows_NT)
LDFLAGS += -mwindows
endif
else
CFLAGS  += -O0 -g
ifeq ($(OS),Windows_NT)
# MSYS2's sdl3.pc adds -mwindows; keep a console in debug builds for SDL_Log output.
LDLIBS  += -mconsole
else
CFLAGS  += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif
endif

BIN := $(BUILDDIR)/$(TARGET)$(EXE)

# On Windows, copy SDL3.dll next to the exe so it runs outside the MSYS2 environment.
ifeq ($(OS),Windows_NT)
SDL_DLL := $(shell $(PKG_CONFIG) --variable=prefix sdl3)/bin/SDL3.dll
RUNTIME := $(BUILDDIR)/SDL3.dll
endif

.PHONY: all release run clean

all: $(BIN) $(RUNTIME)

release:
	$(MAKE) BUILD=release

run: all
	./$(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ifeq ($(OS),Windows_NT)
$(RUNTIME): | $(BUILDDIR)
	cp "$(SDL_DLL)" $@
endif

$(BUILDDIR)/%.o: src/%.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) -c $< -o $@

# Generated GLAD loader: build without our warning flags.
$(BUILDDIR)/glad.o: $(GLAD_SRC) | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $@

clean:
	rm -rf build

-include $(DEP)
