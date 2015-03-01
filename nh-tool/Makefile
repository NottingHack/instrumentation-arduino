# TO use this Makefile to build the code in this directory, install the arduino-mk package
# (on debian based dsitros)
BOARD_TAG = uno
ARDUINO_PORT = /dev/ttyUSB0
ARDUINO_LIBS = LiquidCrystal_I2C PubSubClient MFRC522 EEPROM Wire SPI Ethernet
USER_LIB_PATH := $(realpath ../libs)
include /usr/share/arduino/Arduino.mk
