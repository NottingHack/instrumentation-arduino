# TO use this Makefile to build the code in this directory, install the arduino-mk package
# (on debian based dsitros)
ARDMK_DIR = /usr/share/arduino
BOARD_TAG = uno
ARDUINO_LIBS = PubSubClient EEPROM SPI Ethernet SoftwareSerial ModbusMaster
USER_LIB_PATH := $(realpath ../libraries)

ISP_PROG = atmelice_isp
ISP_PORT = atmelice_isp

AVRDUDE = /usr/bin/avrdude
AVRDUDE_CONF = /usr/share/arduino/hardware/tools/avrdude.conf
SIZEFLAGS = --mcu=$(MCU)

# Embed a reference to the version in the binary
BUILD_IDENT = $(shell git describe --always --dirty)
CPPFLAGS += -D'BUILD_IDENT="$(BUILD_IDENT)"'
OPTIMIZATION_LEVEL=s

BOARD_CLOCK = 8000000

include  $(ARDMK_DIR)/Arduino.mk


# /usr/bin/avrdude -V -p m328pb -D -c atmelice_isp -U lfuse:w:0x62:m -U hfuse:w:0xD5:m -U efuse:w:0xFF:m -U lock:w:0xFF:m  -e -U flash:w:build-uno/EnergyMonitoring_.hex:i
