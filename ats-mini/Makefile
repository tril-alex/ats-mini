ARDUINO_CLI := arduino-cli
ARDUINO_LIBS = "PU2CLR SI4735" TFT_eSPI

VENDOR = esp32
ARCHITECTURE = esp32
BOARD = esp32s3

SDK = $(VENDOR):$(CORE)
FQBN = $(VENDOR):$(ARCHITECTURE):$(BOARD)

DEBUG_LEVEL := none
BOARD_OPTIONS = CDCOnBoot=cdc,FlashSize=8M,PSRAM=disabled,CPUFreq=80,USBMode=hwcdc,FlashMode=qio,PartitionScheme=default_8MB,DebugLevel=$(DEBUG_LEVEL)

PORT := /dev/cu.usbmodem1101

INO = ats-mini.ino
SRC = $(INO) Rotary.cpp
HEADERS = User_Setup.h Rotary.h patch_init.h
ELF = ./build/$(VENDOR).$(ARCHITECTURE).$(BOARD)/$(INO).elf

all: build

help:
	@echo 'Run this command to upload the firmware to your radio:'
	@echo
	@echo '  make upload PORT=/dev/cu.usbmodem1101'
	@echo

install-tools: install-sdk install-libs

install-sdk:
	$(ARDUINO_CLI) core install $(SDK)

install-libs:
	$(ARDUINO_CLI) lib install $(ARDUINO_LIBS)

build: $(ELF)

$(ELF): $(INO) $(SRC) $(HEADERS)
	$(ARDUINO_CLI) compile -b $(FQBN) -e --board-options $(BOARD_OPTIONS)

upload: build
	$(ARDUINO_CLI) upload -b $(FQBN) -p $(PORT)

clean:
	rm -Rf ./build/


.PHONY: all help install-tools install-sdk install-libs build upload clean
