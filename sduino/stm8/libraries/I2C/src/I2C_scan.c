/* I2C_scan.c - Busqueda de dispositivos (libreria I2C). LGPL 2.1. */
#include <Arduino.h>
#include <string.h>
#include "I2C.h"
#include "I2C_private.h"

/* --- busqueda de dispositivos -------------------------------------------- */

void I2C_scan(void)
{
	uint8_t s, found = 0, e;

	Serial_println_s("Buscando dispositivos I2C...");
	for (s = 1; s < 0x78; s++) {
		e = twi_probe(s);
		if (e == TWI_OK) {
			Serial_print_s("Dispositivo en 0x");
			if (s < 16) Serial_print_c('0');
			Serial_println_ub(s, HEX);
			found++;
		} else if (e != TWI_ERR_NACK_ADDR) {
			Serial_println_s("Error en el bus (revise SDA/SCL y las resistencias de pull-up)");
			return;
		}
	}
	if (!found) Serial_println_s("No se encontro ningun dispositivo");
}

