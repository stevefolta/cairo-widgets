PROGRAM := cairo-widgets-test
OBJECTS_DIR := build
CFLAGS += -Wall

-include Makefile.local

SOURCES := main.cpp ExampleWindowWidget.cpp
WIDGETS_SOURCES += Widget.cpp Button.cpp TextBox.cpp PopupMenu.cpp Label.cpp
WIDGETS_SOURCES += StringInputBox.cpp Checkbox.cpp CompoundWidget.cpp TimeSeconds.cpp
WIDGETS_SOURCES += CairoApp.cpp CairoWindow.cpp CairoFontconfigFont.cpp
SUBDIRS = CairoWidgets
LIBRARIES := cairo fontconfig
# Work around bug in Cairo headers:
CFLAGS += $(shell pkg-config --cflags cairo)

WINDOW_BACKEND ?= xcb
ifeq "$(WINDOW_BACKEND)" "xcb"
	WIDGETS_SOURCES += XCBCairoApp.cpp XCBCairoWindow.cpp
	WIDGETS_SOURCES += XCBConnection.cpp XCBXKBKeyboard.cpp
	LIBRARIES += xcb xkbcommon xkbcommon-x11 xcb-xkb
	DEFINES += XCB_WINDOW_BACKEND
endif
ifeq "$(WINDOW_BACKEND)" "wayland"
   WIDGETS_SOURCES += WaylandCairoApp.cpp WaylandCairoWindow.cpp
   WIDGETS_SOURCES += WaylandDisplay.cpp WaylandXKBKeyboard.cpp
   LIBRARIES += xkbcommon wayland-client wayland-cursor
   DEFINES += WAYLAND_WINDOW_BACKEND

   WAYLAND_PROTOCOLS := xdg-shell xdg-decoration-unstable-v1 cursor-shape-v1
   # Needed by cursor-shape-v1:
   WAYLAND_PROTOCOLS += tablet-unstable-v2
   WAYLAND_PROTOCOL_DIR ?= wayland-protocols
   WAYLAND_PROTOCOLS_SOURCE_DIR ?= /usr/share/wayland-protocols
   SUBDIRS += $(WAYLAND_PROTOCOL_DIR)
   WAYLAND_PROTOCOL_SOURCES = $(foreach protocol,$(WAYLAND_PROTOCOLS),$(WAYLAND_PROTOCOL_DIR)/$(protocol)-protocol.c)
   WAYLAND_PROTOCOL_HEADERS = $(foreach protocol,$(WAYLAND_PROTOCOLS),$(WAYLAND_PROTOCOL_DIR)/$(protocol)-client-protocol.h)
   SOURCES += $(WAYLAND_PROTOCOL_SOURCES)
endif

SOURCES += $(foreach source,$(WIDGETS_SOURCES),CairoWidgets/$(source))

OBJECTS = $(foreach source,$(SOURCES),$(OBJECTS_DIR)/$(patsubst %.c,%.o,$(source:.cpp=.o)))
OBJECTS_SUBDIRS = $(foreach dir,$(SUBDIRS),$(OBJECTS_DIR)/$(dir))
DEP_FILES = $(foreach object,$(OBJECTS),$(object:.o=.d))

ifndef VERBOSE_MAKE
	QUIET := @
endif

all: $(PROGRAM)

CXX ?= g++
CXX_CFLAGS += -std=c++17
CFLAGS += -MMD
CFLAGS += -g
CFLAGS += -Wno-unused-function
CFLAGS += -Wno-unused-result
CFLAGS += $(foreach dir,$(SUBDIRS),-I$(dir))
CFLAGS += $(foreach switch,$(DEFINES),-D$(switch))
LINK_FLAGS += $(foreach lib,$(LIBRARIES),-l$(lib))

$(OBJECTS_DIR)/%.o: %.cpp
	@echo Compiling $<...
	$(QUIET) $(CXX) -c $< -g $(CFLAGS) $(CXXFLAGS) -o $@

$(OBJECTS_DIR)/%.o: %.c
	@echo Compiling $<...
	$(QUIET) $(CC) -c $< -g $(CFLAGS) -o $@

$(OBJECTS): | $(OBJECTS_DIR)

$(PROGRAM): $(OBJECTS)
	@echo "Linking $@..."
	$(QUIET) $(CXX) $(filter-out $(OBJECTS_DIR),$^) -g $(LINK_FLAGS) -o $@

$(OBJECTS_DIR):
	@echo "Making $@..."
	$(QUIET) mkdir -p $(OBJECTS_DIR) $(OBJECTS_SUBDIRS)

$(WAYLAND_PROTOCOL_DIR)/%-protocol.c:
	@echo "Making $@..."
	$(QUIET) set -e; 	\
		XML_SOURCE=$(wildcard $(WAYLAND_PROTOCOLS_SOURCE_DIR)/*/*/$(@:$(WAYLAND_PROTOCOL_DIR)/%-protocol.c=%.xml)); 	\
		wayland-scanner private-code < $$XML_SOURCE > $@
$(WAYLAND_PROTOCOL_DIR)/%-client-protocol.h:
	@echo "Making $@..."
	$(QUIET) set -e; 	\
		XML_SOURCE=$(wildcard $(WAYLAND_PROTOCOLS_SOURCE_DIR)/*/*/$(@:$(WAYLAND_PROTOCOL_DIR)/%-client-protocol.h=%.xml)); 	\
		wayland-scanner client-header < $$XML_SOURCE > $@
.PHONY: wayland-protocols
wayland-protocols: $(WAYLAND_PROTOCOL_SOURCES) $(WAYLAND_PROTOCOL_HEADERS)

-include $(DEP_FILES)


.PHONY: run
run: $(PROGRAM)
	@./$(PROGRAM) $(RUN_ARGS)

.PHONY: clean
clean:
	rm -rf $(OBJECTS_DIR)

.PHONY: tags
tags:
	ctags -R .

.PHONY: edit-all
edit-all:
	@ $(EDITOR) $(filter-out main.h,$(foreach source,$(SOURCES),$(source:.cpp=.h) $(source)))

.PHONY: info
info:
	@echo "Objects: $(OBJECTS)"



