# eonduino
EON cpu system with arduino and real hardware. Also a simple emulator is included.
This is a very simple 32 bit EON cpu system, with 4KB ROM and 128KB RAM.
The estimated clock frecuency is about 1MHz.

# hardware parts
* avr Mega (almost any avr board will work)
* 24LC256 serial EEPROM, 128Kbit, I2C, 3.3 or 5v
* 23LC1024 serial RAM, 1Mbit, SPI, 3.3 or 5v
* DS1307 serial RTC, I2C, 3.3 or 5v
* breadboard, resistors, leds, one button swith and dupont wires

# build
Prerequisites:
* eonrom.img from [eonrom](https://github.com/elgron-eon/eonrom)
* [arduino-cli](https://github.com/arduino/arduino-cli)
* a serial terminal emulator. I use [picocom](https://github.com/npat-efault/picocom), but any other will work.

to build emulator, just type `make`  
to build arduino image, type `make compile`  
and finally type `make upload && make com` to enjoy your system !

# Gallery

![foto1](photo/foto1.jpg?raw=true)
![foto2](photo/foto2.jpg?raw=true)
![plain](photo/without-avr.jpg?raw=true)
