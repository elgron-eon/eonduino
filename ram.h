/*
 * ram.h
 *
 * SPI serial RAM supports (like 23LC1024)
 * (c) JCGV, junio del 2022
 *
 */

// commands
#define RAM_RDMR    0x05    // Read the Mode Register
#define RAM_WRMR    0x01    // Write to the Mode Register
#define RAM_READ    0x03
#define RAM_WRITE   0x02
#define RAM_RSTIO   0xFF    // Reset memory to SPI mode

// page bytes
#define SRAM_PAGE   32

// modes
#define ByteMode    0x00    // one byte
#define PageMode    0x80    // full page (32 bytes)
#define Sequential  0x40    // memory blocks

#define BEGIN	{SPI.beginTransaction (SPISettings (1000000, MSBFIRST, SPI_MODE0)); digitalWrite (SS, LOW);}
#define END	{digitalWrite (SS, HIGH); SPI.endTransaction ();}

static void ram_setMode (byte mode) {
    if (0) {
	BEGIN
	SPI.transfer (RAM_RDMR);
	byte prev = SPI.transfer (0x00);
	END

	sprintf (sbuf, "RAM prev mode = %02x new mode = %02x SS = %u\n", prev, mode, SS);
	Serial.print (sbuf);
    }

    BEGIN
    SPI.transfer (RAM_WRMR);
    SPI.transfer (mode);
    END
}

#if 0
static byte ram_readByte (uint32_t address) {
    BEGIN
    SPI.transfer (RAM_READ);
    SPI.transfer ((byte) (address >> 16));  // send high byte of address
    SPI.transfer ((byte) (address >> 8));   // send middle byte of address
    SPI.transfer ((byte) address);	    // send low byte of address
    char data = SPI.transfer (0x00);	    // read the byte at that address
    END
    return data;
}

static void ram_writeByte (uint32_t address, byte data) {
    BEGIN
    SPI.transfer (RAM_WRITE);
    SPI.transfer ((byte) (address >> 16));
    SPI.transfer ((byte) (address >> 8));
    SPI.transfer ((byte) address);
    SPI.transfer (data);
    END
}
#endif

static void ram_writePage (uint32_t address, byte *data) {
    BEGIN
    SPI.transfer (RAM_WRITE);
    SPI.transfer ((byte) (address >> 16));
    SPI.transfer ((byte) (address >> 8));
    SPI.transfer ((byte) address);
#ifdef EMULATOR
    SPI.transferPage (data, SRAM_PAGE);
#else
    SPI.transfer (data, SRAM_PAGE);
#endif
    END
}

static void ram_readPage (uint32_t address, byte *data) {
    BEGIN
    SPI.transfer (RAM_READ);
    SPI.transfer ((byte) (address >> 16));
    SPI.transfer ((byte) (address >> 8));
    SPI.transfer ((byte) address);
    for (uint16_t i=0; i < SRAM_PAGE; i++)
	data[i] = SPI.transfer (0x00);
    END
}

static void ram_init (void) {
    // Slave Select PIN setup
    pinMode (SS, OUTPUT);
    digitalWrite (SS, HIGH);

    // init SPI bus
    SPI.begin ();

    // set transfer mode
    ram_setMode (Sequential);

    // test RAM
    if (0) {
	sprintf (sbuf, "\t| SS ram is %i", SS); Serial.println (sbuf);

	// test data
	static byte data[SRAM_PAGE];
	for (unsigned i = 0; i < SRAM_PAGE; i++)
	    data[i] = i;

	// write to RAM && reread
	unsigned addr = 0x0400;
	ram_writePage (addr, data);
	ram_readPage  (addr, data);

	// compare
	bool fail = false;
	for (unsigned i = 0; i < SRAM_PAGE; i++) {
	    sprintf (sbuf, " %02x/%02x", i, data[i]);
	    Serial.print (sbuf);
	    if (i != data[i]) fail = true;
	}
	if (fail)
	    Serial.println (F ("\n\t| RAM test fail"));
	else
	    Serial.println (F ("\n\t| RAM test pass"));
    }
}
