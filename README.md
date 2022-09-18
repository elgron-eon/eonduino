# eonduino
[EON](https://github.com/elgron-eon/eon-cpu) cpu system with arduino and real hardware. An emulator is included.
This is a very simple 32 bit EON cpu system, with 8KB ROM and 128KB RAM.
The estimated clock frecuency is about 1MHz.

# hardware parts
* avr Mega (5v) (almost any avr board will work) or esp8266 nodemcuv2 (recommended, 3.3v)
* 24LC256 serial EEPROM, 128Kbit, I2C, 3.3 or 5v
* 23LC1024 serial RAM, 1Mbit, SPI, 3.3 or 5v
* DS1307 serial RTC, I2C, 3.3 or 5v
* breadboard, resistors, leds, one button switch and dupont wires

# build
Prerequisites:
* eonrom.img version 0.1.0 from [eonrom](https://github.com/elgron-eon/eonrom)
* [arduino-cli](https://github.com/arduino/arduino-cli)
* a serial terminal emulator. I use [picocom](https://github.com/npat-efault/picocom), but any other will work.

to build emulator, just type `make`  
to build arduino image, type `make compile`  
and finally type `make upload && make com` to enjoy your system !

# diagram
check the gallery images. From top to bottom:
* EEPROM
* I2C bus
* RTC clock
* space for SPI card reader (not included in this model, it's only 3.3v)
* SPI bus
* SRAM
* leds: red = error, blue = reset, green = run
* reset button

# gallery
![foto1](photo/foto1.jpg?raw=true)
![foto2](photo/foto2.jpg?raw=true)
![plain](photo/without-avr.jpg?raw=true)

# avr mega pinout
* reset button: pin13(active low)
* leds: pin10(RUN) pin11(RESET) pin12(ERROR)
* i2c bus: pin20(SDA) pin21(SCL)
* spi bus: pin52(SCK) pin50(MISO) pin 51(MOSI) ping 53(SS active low)

# esp8266 mega pinout
* reset button: D0(active low)
* leds: LED_BUILTIN(RUN)
* i2c bus: D2(SDA) D1(SCL)
* spi bus: D5(SCK) D6(MISO) pin D7(MOSI) D16(SS active low)

# eeprom pinout
```
                  ++++++++
             SDA -|      |- GND
             SCL -|      |- A2 unconnected
  unconnected WP -|      |- A1 unconnected 
             VCC -|  \/  |- A0 unconnected
                  ++++++++
```
# i2c bus
i2c bus needs 2K pullup resitors, at least in SDA(blue). SCL(orange) resistor can be omitted

# rtc pinout
```
    unconnected -+
    unconnected -+
            SCL -+
            SDA -+
            VCC -+
            GND -+
```

# spi bus
* SCK: orange
* MISO: white
* MOSI: blue

# sram pinout
```
                ++++++++
          MOSI -|      |- GND
           SCK -|      |- 10K - VCC
     VCC - 10K -|      |- MISO
           VCC -|  \/  |- SS - 10K - VCC (active low)
                ++++++++
```
