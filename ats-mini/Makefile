ARDUINO_CLI := arduino-cli

PROFILE = esp32s3
FQBN = esp32.esp32.esp32s3

DEBUG_LEVEL =
BUILD_PROPERY = --build-property "build.extra_flags=-DDEBUG=$(DEBUG_LEVEL)"

PORT := /dev/cu.usbmodem1101

INO = ats-mini.ino
HEADERS = Common.h Themes.h Menu.h User_Setup.h Rotary.h patch_init.h
ELF = ./build/$(FQBN)/$(INO).elf

SRC = \
	$(INO) Rotary.cpp Draw.cpp Menu.cpp Station.cpp Battery.cpp \
	Themes.cpp Remote.cpp

all: build

help:
	@echo 'Run this command to upload the firmware to your radio:'
	@echo
	@echo '  make upload PORT=/dev/cu.usbmodem1101'
	@echo

build: $(ELF)

$(ELF): $(INO) $(SRC) $(HEADERS)
	$(ARDUINO_CLI) compile -e -p $(PROFILE) $(BUILD_PROPERY)

upload: build
	$(ARDUINO_CLI) upload -m $(PROFILE) -p $(PORT)

clean:
	$(ARDUINO_CLI) cache clean
	rm -Rf ./build/


.PHONY: all help build upload clean
