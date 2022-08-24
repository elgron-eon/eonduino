#BOARD = arduino:avr:mega
#PORT  = /dev/ttyACM0
#BFLAG = BOARD_MEGA

BOARD = esp8266:esp8266:nodemcuv2
PORT  = /dev/ttyUSB0
BFLAG = BOARD_MCUV2

all: zeon

clean:
	rm -fr rom.h zeon eonduino

rom.h: eonrom.bin
	@echo "generating rom.h ..."
	@echo "static const unsigned char zROM[8192] PROGMEM = {" > $@
	@bin2c $^  >> $@
	@echo "};" >> $@

zeon: emulator.c eonduino.ino rom.h rtc.h ram.h cpu.h dcache.h eeprom.h
	musl-gcc -D_GNU_SOURCE -std=c11 -O2 -Wall -o $@ emulator.c

compile: eonduino.ino rom.h rtc.h ram.h cpu.h dcache.h eeprom.h
	rm -fr eonduino && mkdir eonduino && cp $^ eonduino
	arduino-cli compile -e -b ${BOARD} --build-property "build.extra_flags=-D${BFLAG}" eonduino

upload:
	arduino-cli upload -b ${BOARD} eonduino -p ${PORT}

com:
	rm -f /tmp/z1.log && picocom --logfile /tmp/z1.log -b 115200 -r -l -f n --imap lfcrlf ${PORT}

