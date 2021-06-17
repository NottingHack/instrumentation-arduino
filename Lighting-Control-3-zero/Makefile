# TO use this Makefile to build the code in this directory, install the arduino-mk package
# (on debian based dsitros)
BOARD_TAG = arduino_zero_native
ARDUINO_PORT = /dev/cu.usbmodem14501
ARDUINO_LIBS = PubSubClient SPIEEPROM Wire SPI Ethernet ArduinoRS485 ArduinoModbus WDTZero SDM_Energy_Meter
USER_LIB_PATH := $(realpath ../libraries)

# Embed a reference to the version in the binary
BUILD_IDENT = $(shell git describe --always --dirty)
CPPFLAGS += -D'BUILD_IDENT="$(BUILD_IDENT)"'

# Workaround ArduinoRS485 expecting to be mkeZero
CPPFLAGS += -D'A6=9'

# SDM_Energy_Meter
CPPFLAGS += -D'USE_HARDWARESERIAL'
CPPFLAGS += -D'NOT_A_PIN=PIO_NOT_A_PIN'

include $(ARDMK_DIR)/Sam.mk
