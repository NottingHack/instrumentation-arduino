# TO use this Makefile to build the code in this directory, install the arduino-mk package
# (on debian based dsitros)
BOARD_TAG = arduino_zero_native
ARDUINO_PORT = /dev/tty.usbserial-usbmodem14101
ARDUINO_LIBS = PubSubClient SPIEEPROM Wire SPI Ethernet ArduinoRS485 ArduinoModbus
USER_LIB_PATH := $(realpath ../libraries)

# Embed a reference to the version in the binary
BUILD_IDENT = $(shell git describe --always --dirty)
CPPFLAGS += -D'BUILD_IDENT="$(BUILD_IDENT)"'

# Workaround ArduinoRS485 expecting to be mkeZero
CPPFLAGS += -D'A6=9'

include $(ARDMK_DIR)/Sam.mk
