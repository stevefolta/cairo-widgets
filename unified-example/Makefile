PROGRAM := cairo-widgets-test
OBJECTS_DIR := build
CFLAGS += -Wall

-include Makefile.local

SOURCES := main.cpp ExampleWindowWidget.cpp
WIDGETS_SOURCES += Widget.cpp Button.cpp TextBox.cpp PopupMenu.cpp Label.cpp
WIDGETS_SOURCES += StringInputBox.cpp Checkbox.cpp CompoundWidget.cpp TimeSeconds.cpp
WIDGETS_SOURCES += CairoApp.cpp CairoWindow.cpp
SUBDIRS = CairoWidgets
LIBRARIES := cairo

WINDOW_BACKEND ?= xcb
ifeq "$(WINDOW_BACKEND)" "xcb"
	WIDGETS_SOURCES += XCBCairoApp.cpp XCBCairoWindow.cpp
	WIDGETS_SOURCES += XCBConnection.cpp XCBXKBKeyboard.cpp
	LIBRARIES += xcb xkbcommon xkbcommon-x11 xcb-xkb
	DEFINES += XCB_WINDOW_BACKEND
endif

SOURCES += $(foreach source,$(WIDGETS_SOURCES),CairoWidgets/$(source))

OBJECTS = $(foreach source,$(SOURCES),$(OBJECTS_DIR)/$(source:.cpp=.o))
OBJECTS_SUBDIRS = $(foreach dir,$(SUBDIRS),$(OBJECTS_DIR)/$(dir))
DEP_FILES = $(foreach object,$(OBJECTS),$(object:.o=.d))

ifndef VERBOSE_MAKE
	QUIET := @
endif

all: $(PROGRAM)

CXX ?= g++
CFLAGS += -std=c++17
CFLAGS += -MMD
CFLAGS += -g
CFLAGS += -Wno-unused-function
CFLAGS += -Wno-unused-result
CFLAGS += $(foreach dir,$(SUBDIRS),-I$(dir))
CFLAGS += $(foreach switch,$(DEFINES),-D$(switch))
LINK_FLAGS += $(foreach lib,$(LIBRARIES),-l$(lib))

$(OBJECTS_DIR)/%.o: %.cpp
	@echo Compiling $<...
	$(QUIET) $(CXX) -c $< -g $(CFLAGS) -o $@

$(OBJECTS): | $(OBJECTS_DIR)

$(PROGRAM): $(OBJECTS)
	@echo "Linking $@..."
	$(QUIET) $(CXX) $(filter-out $(OBJECTS_DIR),$^) -g $(LINK_FLAGS) -o $@

$(OBJECTS_DIR):
	@echo "Making $@..."
	$(QUIET) mkdir -p $(OBJECTS_DIR) $(OBJECTS_SUBDIRS)

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



