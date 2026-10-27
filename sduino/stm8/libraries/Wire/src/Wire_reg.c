/*
 * Wire_reg.c - requestFrom() con direccion interna (5 argumentos).
 * En un modulo aparte: solo se enlaza si se usa.
 *
 * Parte de sduino. LGPL 2.1.
 */
#include "Wire.h"

/* Lectura de un registro interno: escribe 'isize' bytes de direccion
 * (el mas significativo primero) con inicio repetido y luego lee. */
uint8_t Wire_requestFrom5(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop)
{
	if (isize) {
		Wire_beginTransmission(address);
		if (isize > 3) isize = 3;
		while (isize-- > 0) Wire_write((uint8_t)(iaddress >> (isize * 8)));
		if (Wire_endTransmission1(0)) return 0;
	}
	return Wire_requestFrom3(address, quantity, sendStop);
}

