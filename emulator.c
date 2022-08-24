/*
 * emulator.c
 *
 * eonduino emulator
 * (c) JCGV, junio del 2022
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>

/*
 * terminal support
 */
static struct termios term;

static void term_done (void) {
    tcsetattr (STDIN_FILENO, TCSANOW, &term);
}

static void term_setup (void) {
    tcgetattr (STDIN_FILENO, &term);
    struct termios t = term;
    t.c_lflag &= ~ICANON;
    t.c_lflag &= ~ECHO;
    tcsetattr (STDIN_FILENO, TCSANOW, &t);
    atexit (term_done);
}

/*
 * support code
 */
typedef uint8_t byte;

#define PROGMEM
#define F(str)	    str
#define HIGH	    1
#define LOW	    0
#define SEXT8(n)    LIMIT32 ((long) (int8_t ) (n))
#define SEXT16(n)   LIMIT32 ((long) (int16_t) (n))
#define LIMIT32(n)  ((n) & 0x0fffffffful)

// ROM read support
static byte pgm_read_byte_near (const byte *p) {return *p;}

// PINS
enum {OUTPUT, INPUT_PULLUP};

static byte vled[64];
static void pinMode	 (unsigned pin, unsigned mode) {}
static int  digitalRead  (unsigned pin) {return HIGH;}
static void digitalWrite (unsigned pin, unsigned v) {vled[pin] = v;}

// idle
static void idle (void) {
    // wait for event
    struct pollfd pfd = {
	.fd	 = STDIN_FILENO,
	.events  = POLLIN,
	.revents = 0
    };
    poll (&pfd, 1, -1);
}

// serial console
static void s_print (const char *msg) {
    printf ("%s", msg);
    fflush (stdout);
}

static void s_println (const char *msg) {
    printf ("%s\n", msg);
    fflush (stdout);
}

static void delay   (unsigned ms) {}
static void s_write (unsigned c) {fputc (c, stdout); fflush (stdout);}
static void s_flush (void) {fflush (stdout);}
static void s_begin (unsigned b) {}
static bool s_ready (void) {return true;}

static bool s_available (void) {
    struct pollfd pfd = {
	.fd	 = STDIN_FILENO,
	.events  = POLLIN,
	.revents = 0
    };
    int rc = poll (&pfd, 1, 0);
    return rc > 0;
}

static byte s_read (void) {
    byte data = 0;
    read (STDIN_FILENO, &data, 1);
    return data;
}

static struct {
    void (*print) (const char *);
    void (*println) (const char *);
    void (*begin) (unsigned);
    void (*flush) (void);
    void (*write) (unsigned);
    bool (*availableForWrite) (void);
    bool (*available) (void);
    byte (*read)  (void);
} Serial = {s_print, s_println, s_begin, s_flush, s_write, s_ready, s_available, s_read};

// WIRE
static uint8_t iow, ior, iomode;
static uint8_t iobuf[32];
enum {IO_READ, IO_WRITE};

static void w_begin (void) {}
static bool w_available (void) {return true;}

static void w_beginT (unsigned port) {
    iobuf[0] = port;
    iow      = 1;
    iomode   = IO_WRITE;
}

static int w_request (unsigned port, unsigned v) {
    ior    = 0;
    iomode = IO_READ;
    //printf ("wire request %02x %02x\n", port, v);
    return v;
}

static int w_end (void);

static void w_write (unsigned reg) {
    iobuf[iow++] = reg;
    //printf ("write %02x at %i\n", reg, iow);
}

static unsigned w_read (void) {
    unsigned v = iobuf[ior++];
    //printf ("read %02x at %i\n", v, ior);
    return v;
}

static struct {
    void     (*beginTransmission) (unsigned port);
    int      (*requestFrom)	  (unsigned port, unsigned);
    int      (*endTransmission)   (void);
    void     (*write)		  (unsigned r);
    unsigned (*read)		  (void);
    bool     (*available)	  (void);
    void     (*begin)		  (void);
} Wire = {w_beginT, w_request, w_end, w_write, w_read, w_available, w_begin};

typedef struct RTCDateTime RTCDateTime;

// SPI
#define MSBFIRST    0
#define SPI_MODE0   0
#define SS	    53
static int SPISettings (unsigned mhz, unsigned msb, unsigned mode) {return 0;}

static byte	sram[0x20000];	// 128KB ram
static unsigned spi_st, spi_st2;
static unsigned spi_addr;

static void i_begin (void) {}
static void i_tran  (int)  {spi_st = 0;}
static void i_end   (void) {}

static unsigned i_transfer (unsigned byte);

static void i_page (const byte *p, unsigned n) {
    //printf ("write page %u at %06x\n", n, spi_addr);
    for (; n > 0; n--)
	i_transfer (*p++);
}

static struct {
    void	(*begin) (void);
    void	(*beginTransaction) (int);
    void	(*endTransaction) (void);
    unsigned	(*transfer) (unsigned);
    void	(*transferPage) (const byte *, unsigned);
} SPI = {i_begin, i_tran, i_end, i_transfer, i_page};

/*
 * ino code
 */
#define EMULATOR
#include "eonduino.ino"

void no_warn_unused (void) {
    RTCDateTime dt = {0,}; rtc_setDateTime (dt);
}

/*
 * i/o support
 */
static int w_end (void) {
    if (iomode == IO_WRITE) {
	unsigned port = iobuf[0];
	switch (port) {
	    case DS3231_ADDRESS:
		switch (iobuf[1]) {
		    case DS3231_REG_CONTROL:
			iobuf[0] = 0x00;
			break;
		    case DS3231_REG_TIME: {
			    time_t t = time (NULL);
			    struct tm tm;
			    gmtime_r (&t, &tm);
			    iobuf[6] = dec2bcd (tm.tm_year + 1900 - 2000);
			    iobuf[5] = dec2bcd (tm.tm_mon + 1);
			    iobuf[4] = dec2bcd (tm.tm_mday);
			    iobuf[3] = tm.tm_wday;
			    iobuf[2] = dec2bcd (tm.tm_hour);
			    iobuf[1] = dec2bcd (tm.tm_min);
			    iobuf[0] = dec2bcd (tm.tm_sec);
			} break;
		}
		break;
	    case EEPROM_ADDRESS: {
		    unsigned addr = (iobuf[1] << 8) | iobuf[2];
		    //printf ("eeprom addr %04x\n", addr);
		    unsigned i = 0;
		    for (unsigned top = addr + SRAM_PAGE; addr < top; addr++)
			iobuf[i++] = zROM[addr];
		} break;
	    default:
		printf ("I/O write port %02x:", port);
		for (int i = 1; i < iow; i++)
		    printf (" %02X", iobuf[i]);
		printf ("\n");
		exit   (1);
		break;
	}
    } else {
	//printf ("end\n");
    }
    return 0;
}

static unsigned i_transfer (unsigned byte) {
    switch (spi_st) {
	case 0: // RAM cmd
	    switch (byte) {
		case RAM_WRMR:
		    spi_st = 1;
		    break;
		case RAM_WRITE:
		    spi_addr = 0;
		    spi_st   = 2;
		    spi_st2  = 5;
		    break;
		case RAM_READ:
		    spi_addr = 0;
		    spi_st   = 2;
		    spi_st2  = 6;
		    break;
		default:
		    printf ("SPI cmd unknnown %02x\n", byte);
		    break;
	    }
	    break;
	case 1: // ignore all
	    break;
	case 2: // hight addr byte
	    spi_addr = byte;
	    spi_st   = 3;
	    break;
	case 3: // middle addr byte
	    spi_addr = (spi_addr << 8) | byte;
	    spi_st   = 4;
	    break;
	case 4: // low addr byte
	    spi_addr = (spi_addr << 8) | byte;
	    spi_st   = spi_st2;
	    break;
	case 5: // write bytes
	    sram[spi_addr++] = byte;
	    break;
	case 6: // read bytes
	    byte = sram[spi_addr++];
	    break;
    }
    return byte;
}

/*
 * entry point
 */
int main (int argc, char **argv) {
    // setup
    term_setup ();
    setup ();

    // loop
    while (eon.st != S_DONE)
	loop ();

    // report
    if (vled[zBAD]) {
	printf ("fail led at pc %08lx status %04lx:", eon.pc, eon.s[REG_STATUS]);
	for (unsigned i = 0; i < 16; i++)
	    printf ("%sR%02u=%08x.%08x", i % 4 == 0 ? "\n\t" : " ", i, (uint32_t) (eon.r[i] >> 32), (uint32_t) eon.r[i]);
	printf ("\n");

	// dump line 0
	line_dump ( 0, "Zero Line");
	line_dump (58, "SP   Line");
	line_dump (63, "RSP  Line");

	// dump compiled word
	unsigned begin = 0x4000;
	unsigned end   = begin + 128;
	for (; begin < end; begin += 16) {
	    // get data
	    uint8_t s[16];

	    // print offset & raw bytes
	    printf ("%04x ", begin);
	    for (unsigned i = 0; i < 16; ++i) {
		s[i] = dcache (begin + i, false, 1, 0);
		printf (" %02x", s[i]);
	    }

	    // print ascii
	    printf ("  ");
	    for (unsigned i = 0; i < 16; ++i) {
		int d = s[i];
		printf ("%c", d > 31 && d < 127 ? d : '.');
	    }
	    printf ("\n");
	}
	return 0;

	// dump cache status
	for (unsigned i = 0; i < NLINES; i++) {
	    char title[8];
	    sprintf (title, "L#%02x", i);
	    line_dump (i, title);
	}
    }
    return 0;
}
