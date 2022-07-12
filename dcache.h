/*
 * dcache.h
 *
 * data cache
 * (c) JCGV, junio del 2022
 *
 */
#define WAYS	2	// 2/4 WAYS
#define DSIZE	4096
#define NLINES	(4096 / SRAM_PAGE / WAYS)


#define FLAG_FREE   0x01
#define FLAG_DIRTY  0x02

static byte dcache_raw[DSIZE];

static struct {
    word32  tag[WAYS];
    byte    lru;
} dlines[NLINES];

static word32 dcache_use, dcache_hit, dcache_load, dcache_evicted, dcache_wback;

static void dcache_init (void) {
    dcache_use = dcache_hit = dcache_load = dcache_evicted = dcache_wback;
    for (unsigned i = 0; i < NLINES; i++)
	for (unsigned w = 0; w < WAYS; w++)
	    dlines[i].tag[w] = FLAG_FREE;
}

void line_dump (unsigned line, const char *title) {
    sprintf (sbuf, "\t|line %02x LRU %02x %s:", line, dlines[line].lru, title);
    Serial.println (sbuf);
    for (unsigned w = 0; w < WAYS; w++) {
	word32 t = dlines[line].tag[w];
	int    l = sprintf (sbuf, "\t| #%u ", w);
	if (t & FLAG_FREE)
	    l += sprintf (sbuf + l, "FREE");
	else {
	    l += sprintf (sbuf + l, "%c%08lx", t & FLAG_DIRTY ? '!' : ' ', t);
	    byte *data = &dcache_raw[line * WAYS * SRAM_PAGE + w * SRAM_PAGE];
	    for (int i = 0; i < SRAM_PAGE; i++)
		l += sprintf (sbuf + l, "%s%02x", i % 4 == 0 ? " " : "", data[i]);
	}
	Serial.println (sbuf);
    }
}

static word32 dcache (word32 addr, bool write, unsigned bytes, word32 value) {
    // get line
    word32   mask = SRAM_PAGE - 1;
    word32   tag  = addr & ~mask;
    unsigned line = (addr / SRAM_PAGE) % NLINES;
    //sprintf (sbuf, "\t| addr %08lx line %u/%u tag %08lx mask %04lx", addr, line, NLINES, tag, mask);
    //Serial.println (sbuf);
    dcache_use++;

    // find set & free entry
    int free = -1;
    int set  = -1;
    for (unsigned s = 0; s < WAYS; s++) {
	word32 t = dlines[line].tag[s];
	if (t & FLAG_FREE)
	    free = s;
	else if ((t & ~mask) == tag) {
	    set  = s;
	    dcache_hit++;
	    break;
	}
    }


    // not found, free entry ?
    if (set < 0 && free < 0) {
	// cache eviction
	dcache_evicted++;
	//line_dump (line, "evicted");

	// select set to drop
#if WAYS==2
	free = dlines[line].lru & 1;
#else
#error "unsupported WAYS in dcache update LRU"
#endif

	// write back needed ?
	word32 tag = dlines[line].tag[free];
	if (tag & FLAG_DIRTY) {
	    dcache_wback++;
	    byte *p = &dcache_raw[line * WAYS * SRAM_PAGE + free * SRAM_PAGE];
	    ram_writePage (tag ^ FLAG_DIRTY, p);
	}
    }

    // cache fill
    if (set < 0) {
	set	= free;
	byte *p = &dcache_raw[line * WAYS * SRAM_PAGE + set * SRAM_PAGE];
	if (tag < sizeof (zROM)) {
	    if (0) {
		for (unsigned i = tag; i < tag + SRAM_PAGE; i++)
		    *p++ = pgm_read_byte_near (zROM + i);
	    } else {
		eeprom_read (tag, p, SRAM_PAGE);
	    }
	} else
	    ram_readPage (tag, p);
	dlines[line].tag[set] = tag;
	dcache_load++;
	//line_dump (line, "cache load");
    }

    // update LRU
#if WAYS == 2
    dlines[line].lru = set ? 0 : 1;
#else
#error "unsupported WAYS in dcache update LRU"
#endif

    // do operation
    byte *p = &dcache_raw[line * WAYS * SRAM_PAGE + set * SRAM_PAGE + (addr & mask)];
    if (write) {
	switch (bytes) {
	    case 1: *p = value; break;
	    case 2: p[0] = value >> 8; p[1] = value; break;
	    case 4: p[0] = value >> 24; p[1] = value >> 16; p[2] = value >> 8; p[3] = value; break;
	}
	dlines[line].tag[set] |= FLAG_DIRTY;
    } else {
	switch (bytes) {
	    case 1: value = *p; break;
	    case 2: value = (p[0] << 8) | p[1]; break;
	    case 4: value = (((word32) p[0]) << 24) | (((word32) p[1]) << 16) | (((word32) p[2]) << 8) | p[3]; break;
	}
    }
    return value;
}
