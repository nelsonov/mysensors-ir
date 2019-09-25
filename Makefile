PORT = /dev/ttyESP
IP = 192.168.42.140
PROJECT = IrSensor
FBQN = arduino:avr:pro:cpu=8MHzatmega328
URL = http://$(IP)/console/baud
UPLOADSPEED=57600

CMD = arduino-cli
COMPILE = $(CMD) compile
UPLOAD = $(CMD) upload
CURLCMD = /usr/bin/curl -s
TRCMD = tr -cd 0-9

PROJECTBASE = /srv/src/Arduino/git_projects/mysensors-ir
SRCBASE = $(PROJECTBASE)/$(PROJECT)
BUILD = $(PROJECTBASE)/build

HEX = build/$(PROJECT).ino.hex

ORIGSPEED = $(shell $(CURLCMD) $(URL) | $(TRCMD))

build : $(HEX)

$(HEX) : $(SRCBASE)/*.ino $(SRCBASE)/*.h
	$(COMPILE) --verbose --build-path $(BUILD) \
	--fqbn $(FBQN) $(PROJECT)

install : getspeed $(HEX)
	$(CURLCMD) $(URL)?rate=$(UPLOADSPEED)
	@echo
	$(UPLOAD) --verbose --verify \
	--fqbn $(FBQN) -p $(PORT) $(PROJECT)
	$(CURLCMD) $(URL)?rate=$(ORIGSPEED)
	@echo

getspeed:
	@echo "Original speed: $(ORIGSPEED)"

.PHONY: getspeed build

