# TO use this Makefile to build the code in this directory, install the arduino-mk package
# (on debian based dsitros)
BOARD_TAG = arduino_zero_native
ARDUINO_PORT = /dev/ttyACM0
ARDUINO_LIBS = LiquidCrystal_I2C PubSubClient MFRC522 FlashStorage Wire SPI Ethernet
USER_LIB_PATH := $(realpath ../libraries)

# Embed a reference to the version in the binary
BUILD_IDENT = $(shell git describe --always --dirty)
CPPFLAGS += -D'BUILD_IDENT="$(BUILD_IDENT)"'

ARDUINO_DIR = /home/daniel/ARD/arduino-1.8.5
ARDMK_DIR = /home/daniel/ARD/Arduino-Makefile
ARDUINO_PACKAGE_DIR = /home/daniel/.arduino15/packages

# AVR_TOOLS_DIR – Directory where avr tools are installed

include /home/daniel/ARD/Arduino-Makefile/Sam.mk
