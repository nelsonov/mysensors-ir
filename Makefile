PROJECT = IrSensor

PORT = /dev/ttyUSB0

FBQN = arduino:avr:pro:cpu=8MHzatmega328

SRCBASE = /home/lnelson/src/Arduino

cc = ARDUINO_SKETCHBOOK_DIR=${SRCBASE} arduino-cli compile --fqbn $FBQN

install = ARDUINO_SKETCHBOOK_DIR=/home/lnelson/src/Arduino  arduino-cli upload --fqbn ${FBQN} -p ${PORT} ${PROJECT}

