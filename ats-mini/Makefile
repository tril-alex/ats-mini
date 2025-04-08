ARDUINO_CLI := arduino-cli

PROFILE	= esp32s3
FQBN	= esp32.esp32.esp32s3
INO	= ats-mini.ino
ELF	= ./build/$(FQBN)/$(INO).elf
PORT	= /dev/cu.usbmodem1101

DEBUG_LEVEL =

# USE_REMOTE   : Enable serial port control and monitoring
# TUNE_HOLDOFF : Hold off display updates while tuning
# BFO_MENU_EN  : Add BFO menu option for debugging
# THEME_EDITOR : Enable setting and printing of theme colors
# DEBUG1_PRINT : Highest level - Primary information
# DEBUG2_PRINT :               - Function call results
# DEBUG3_PRINT :               - Misc
# DEBUG4_PRINT : Lowest level  - EEPROM
DEFINES = \
	-DDEBUG=$(DEBUG_LEVEL) \
	-DUSE_REMOTE -DNO_THEME_EDITOR -DTUNE_HOLDOFF -DNO_BFO_MENU_EN \
	-DDEBUG1_PRINT=0 \
	-DDEBUG2_PRINT=0 \
	-DDEBUG3_PRINT=0 \
	-DDEBUG4_PRINT=0

OPTIONS = \
	--build-property "build.extra_flags=$(DEFINES)" \
	--warnings all

HEADERS = \
	Common.h Themes.h Menu.h Storage.h User_Setup.h Rotary.h \
	patch_init.h

SRC = \
	$(INO) Utils.cpp Rotary.cpp Draw.cpp Menu.cpp Station.cpp \
	Battery.cpp Storage.cpp Themes.cpp Remote.cpp

all: build

help:
	@echo 'Run this command to upload the firmware to your radio:'
	@echo
	@echo '  make upload PORT=/dev/cu.usbmodem1101'
	@echo

build: $(ELF)

$(ELF): $(INO) $(SRC) $(HEADERS)
	$(ARDUINO_CLI) compile -e -p $(PROFILE) $(OPTIONS)

upload: build
	$(ARDUINO_CLI) upload -m $(PROFILE) -p $(PORT)

clean:
	$(ARDUINO_CLI) cache clean
	rm -Rf ./build/


.PHONY: all help build upload clean
