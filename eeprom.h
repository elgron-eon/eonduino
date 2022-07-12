/*
 * eeprom.h
 *
 * EEPROM support, 24LC256
 * (c) JCGV, julio del 2022
 *
 */
#define EEPROM_ADDRESS		    0x50
#define EEPROM_READ		    0x01
#define EEPROM_PAGE		    64

static void eeprom_write (unsigned addr, const byte *ptr, unsigned nb) {
    Wire.beginTransmission (EEPROM_ADDRESS);
    Wire.write (addr >> 8);
    Wire.write (addr);
    for (unsigned i = 0; i < nb; i++)
	Wire.write (*ptr++);
    Wire.endTransmission ();
}

static void eeprom_read (unsigned addr, byte *ptr, unsigned nb) {
    Wire.beginTransmission (EEPROM_ADDRESS);
    Wire.write (addr >> 8);
    Wire.write (addr);
    Wire.endTransmission ();

    Wire.requestFrom (EEPROM_ADDRESS, nb);
    for (unsigned i = 0; i < nb; i++)
	*ptr++ = Wire.read ();
    Wire.endTransmission ();
}
