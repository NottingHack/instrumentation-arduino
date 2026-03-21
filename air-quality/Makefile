ARDMK_DIR = /usr/share/arduino
ARDUINO_LIB_PATH = /home/aaron/Arduino/libraries
BOARD_TAG = uno
ARDUINO_LIBS = PubSubClient EEPROM Wire SPI Ethernet Adafruit_SGP40 Adafruit_BusIO SensirionCore arduino-i2c-scd4x SCD4x PMAS003 SoftwareSerial INA219
USER_LIB_PATH := $(realpath ../libraries)

SIZE = /bin/true

ISP_PROG = atmelice_isp
ISP_PORT = atmelice_isp

AVRDUDE = /usr/bin/avrdude
AVRDUDE_CONF = /usr/share/arduino/hardware/tools/avrdude.conf
SIZEFLAGS = --mcu=$(MCU)

include  $(ARDMK_DIR)/Arduino.mk
