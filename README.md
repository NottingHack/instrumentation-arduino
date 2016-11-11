# Nottingham Hackspace Instrumentaion arduino firmwares

This repo contains all the arduino firmwares use by instrumentaion devices in the hack space.

This repo can be used in either with the Arduino IDE or with arduino-mk Makefiles

## Arduino IDE
The best way to use this repo is to set it as the path to your Sketchbook. That way the included libraries folder will be corecly used when compiling sketchs

## Arduino-mk and Makefiles
To use the arduino-mk makefiles you need to install arduino-mk and set the `ARDUINO_DIR` and `ARDMK_DIR`  enviroment variables
This is best done by adding them to your .bashrc .bash_profile scripts

for linux
instal for debian based systems
```
$ sudo apt-get install arduino-mk
```

.bash_rc
```
export ARDUINO_DIR=/home/sudar/apps/arduino-1.0.5
export ARDMK_DIR=/usr/share/arduino
```

for macOS
install arduino-mk using homebrew
```
$ brew tap sudar/arduino-mk
$ brew install arduino-mk
```
.bash_profile
```
export ARDUINO_DIR=/Applications/Arduino.app/Contents/Java
export ARDMK_DIR=/usr/local/opt/arduino-mk
```

