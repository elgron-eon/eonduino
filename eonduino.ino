/*
 * eonduino.ino
 *
 * eon computer with arduino mega
 * (c) JCGV, junio del 2022
 *
 */
#ifndef EMULATOR
#include <Wire.h>
#include <SPI.h>
#endif

// config
#define VERSION     "0.0.2"
#define DELAYMS     20
#define RESETCOUNT  15

// emulator compatibility macros
#ifndef LIMIT32
#define LIMIT32(n)  n
#endif
#ifndef SEXT8
#define SEXT8(n)    ((long) (short int) (n))
#endif
#ifndef SEXT16
#define SEXT16(n)   ((long) (int) (n))
#endif

// pins used
#define zRX	0
#define zTX	1
#define zRESET	13
#define zBAD	12
#define zRST	11
#define zRUN	10

// basic types
typedef unsigned int  word16;
typedef unsigned long word32;

// output format buffer
static char sbuf[128];

// ROM
#include "rom.h"

// RAM
#include "ram.h"

// eeprom
#include "eeprom.h"

// dcache
#include "dcache.h"

// real time clock
#include "rtc.h"

// cpu
static word32 cpu_in  (word32 port);
static void   cpu_out (word32 port, word32 v);

#ifndef EMULATOR
static void idle () {}
#endif
#include "cpu.h"

// I/O support
static byte rtc_iobuf[8];
static byte rtc_ionow;

static word32 cpu_in (word32 port) {
    word32 v = 0;
    switch (port) {
	case 0x00:
	    // console write ready
	    v = Serial.availableForWrite () > 0 ? 1 : 0;
	    break;
	case 0x01:
	    // console read (after IRQ signal)
	    v = Serial.read () & 0x7F;
	    break;
	case 0x02:
	    // rtc control port ready
	    v = 1;
	    break;
	case 0x03:
	    // read RTC data
	    v = rtc_iobuf[rtc_ionow++];
	    break;
	default:
	    sprintf (sbuf, "\t| IN %lx = %04lx\n", port, v);
	    Serial.print (sbuf);
	    break;
    }
    return v;
}

static void cpu_out (word32 port, word32 v) {
    switch (port) {
	case 0x01:
	    Serial.write ((byte) v);
	    break;
	case 0x03:
	    // rtc command (0 = read, 1 = write)
	    if (v == 0) {
		RTCDateTime dt = rtc_getDateTime ();
		rtc_iobuf[0] = dt.year >> 8;
		rtc_iobuf[1] = dt.year;
		rtc_iobuf[2] = dt.month;
		rtc_iobuf[3] = dt.day;
		rtc_iobuf[4] = dt.dayOfWeek;
		rtc_iobuf[5] = dt.hour;
		rtc_iobuf[6] = dt.minute;
		rtc_iobuf[7] = dt.second;
		rtc_ionow    = 0;
		eon_irq (IRQ_RTC);
	    } else {
		rtc_ionow = 0;
	    }
	    break;
	case 0x04:
	    // rtc new data
	    rtc_iobuf[rtc_ionow++] = v;
	    if (rtc_ionow == 8) {
		RTCDateTime dt;
		dt.year      = (rtc_iobuf[0] << 8) | rtc_iobuf[1];
		dt.month     = rtc_iobuf[2];
		dt.day	     = rtc_iobuf[3];
		dt.dayOfWeek = rtc_iobuf[4];
		dt.hour      = rtc_iobuf[5];
		dt.minute    = rtc_iobuf[6];
		dt.second    = rtc_iobuf[7];
		rtc_setDateTime (dt);
	    }
	    break;
	case 0xff:  // debug notification
	    eon_halt ();
	    break;
	default:
	    sprintf (sbuf, "\t| OUT %lx at %04lx\n", v, port);
	    Serial.print (sbuf);
	    break;
    }
}

// dump rom to eeprom
static void rom_dump (bool verify_only) {
    #define chunk   32
    #define wchunk  16
    unsigned top = sizeof (zROM);

    if (!verify_only) {
	Serial.print ("\ndump ROM to EEPROM:");
	for (unsigned addr = 0; addr < top; addr += wchunk) {
	    // read page from internal rom
	    static byte data[wchunk];
	    for (unsigned i = 0; i < wchunk; i++)
		data[i] = pgm_read_byte_near (zROM + (addr + i));

	    // progress
	    if (addr % 1024 == 0) Serial.println ("");
	    Serial.print (".");

	    // write to eeprom
	    eeprom_write   (addr, data, wchunk);
	    delay	   (10);
	}
	delay (100);
    }

    Serial.print ("\nverify EEPROM:");
    for (unsigned addr = 0; addr < top; addr += chunk) {
	// read page from internal rom
	static byte data[chunk];
	for (unsigned i = 0; i < chunk; i++)
	    data[i] = pgm_read_byte_near (zROM + (addr + i));

	// progress
	if (addr % 1024 == 0) Serial.println ("");
	Serial.print (".");

	// read from eeprom
	static byte eeprom[chunk];
	eeprom_read (addr, eeprom, chunk);
	if (memcmp (data, eeprom, chunk) != 0) {
	    sprintf (sbuf, "\nEEPROM verify addr %08x fail:", addr);
	    Serial.println (sbuf);
	    for (unsigned i = 0; i < chunk; i++) {
		sprintf (sbuf, " %02x/%02x", data[i], eeprom[i]);
		Serial.print (sbuf);
	    }
	    break;
	}
    }
    Serial.println ("\ndone");
}

// setup: called once to initialize all stuff
void setup () {
    // init console
    Serial.begin (115200);
    Serial.flush ();

    // wire library
    Wire.begin ();

    // outputs
    pinMode (zRUN, OUTPUT);
    pinMode (zRST, OUTPUT);
    pinMode (zBAD, OUTPUT);
    digitalWrite (zRUN, LOW);
    digitalWrite (zRST, LOW);
    digitalWrite (zBAD, LOW);

    // inputs (active low)
    pinMode (zRESET, INPUT_PULLUP);

    // say hello
    Serial.print (F ("\t| eonduino " VERSION));

    // reset cpu
    eon_reset ();

    // ram setup
    ram_init ();

    // RTC setup
    rtc_begin ();

    // dump rom to eeprom
    if (1) rom_dump (true);
}

// the loop function runs over and over again forever
void loop () {
    // step
    eon_go (64);

    // IRQs
    word32 pending = 0;
    if (Serial.available ()) pending |= IRQ_CON;
    if (pending) eon_irq (pending);
}
