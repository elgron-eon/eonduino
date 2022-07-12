all: zeon

clean:
	rm -fr rom.h zeon eonduino

rom.h: eonrom.bin
	@echo "generating rom.h ..."
	@echo "static const unsigned char zROM[4096] PROGMEM = {" > $@
	@bin2c $^  >> $@
	@echo "};" >> $@

zeon: emulator.c eonduino.ino rom.h rtc.h ram.h cpu.h dcache.h eeprom.h
	musl-gcc -D_GNU_SOURCE -std=c11 -O2 -Wall -o $@ emulator.c

compile: eonduino.ino rom.h rtc.h ram.h cpu.h dcache.h eeprom.h
	rm -fr eonduino && mkdir eonduino && cp $^ eonduino
	arduino-cli compile -e -b arduino:avr:mega eonduino

upload:
	arduino-cli upload -b arduino:avr:mega eonduino -p /dev/ttyACM0

com:
	rm -f /tmp/z1.log && picocom --logfile /tmp/z1.log -b 115200 -r -l -f n --imap lfcrlf /dev/ttyACM0

