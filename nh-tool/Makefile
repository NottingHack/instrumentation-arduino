# TO use this Makefile to build the code in this directory, install the arduino-mk package
# (on debian based dsitros)
BOARD_TAG = uno
ARDUINO_PORT = /dev/ttyUSB0
ARDUINO_LIBS = LiquidCrystal_I2C PubSubClient MFRC522 EEPROM Wire SPI Ethernet
USER_LIB_PATH := $(realpath ../Libraries)

# Embed a reference to the version in the binary
BUILD_IDENT = $(shell git describe --always --dirty)
CPPFLAGS += -D'BUILD_IDENT="$(BUILD_IDENT)"'

include /usr/share/arduino/Arduino.mk
