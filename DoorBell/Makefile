# TO use this Makefile to build the code in this directory, install the arduino-mk package
# (on debian based dsitros)
BOARD_TAG = uno
ARDUINO_PORT = /dev/ttyUSB0
ARDUINO_LIBS = PubSubClient EEPROM Wire SPI Ethernet OneWire DallasTemperature
USER_LIB_PATH := $(realpath ../libraries)

# Embed a reference to the version in the binary
BUILD_IDENT = $(shell git describe --always --dirty)
CPPFLAGS += -D'BUILD_IDENT="$(BUILD_IDENT)"'

include /usr/share/arduino/Arduino.mk
