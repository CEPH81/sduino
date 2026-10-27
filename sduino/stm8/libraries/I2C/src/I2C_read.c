/* I2C_read.c - Lectura (libreria I2C). LGPL 2.1. */
#include <Arduino.h>
#include <string.h>
#include "I2C.h"
#include "I2C_private.h"

/* --- lectura ------------------------------------------------------------- */

/* Lee 'numberBytes' (0 cuenta como 1) en 'dataBuffer'. 0 = correcto. */
uint8_t I2C_readbuf(uint8_t address, uint8_t numberBytes, uint8_t *dataBuffer)
{
	uint8_t n;

	I2C_bytesAvailable = I2C_totalBytes = 0;
	if (!numberBytes) numberBytes = 1;
	n = twi_readFrom(address, dataBuffer, numberBytes, 1);
	if (n != numberBytes) return I2C_mapRead(twi_lastError());
	if (dataBuffer == I2C_rxbuf) I2C_bytesAvailable = I2C_totalBytes = n;
	return 0;
}

uint8_t I2C_read(uint8_t address, uint8_t numberBytes)
{
	if (numberBytes > MAX_BUFFER_SIZE) numberBytes = MAX_BUFFER_SIZE;
	return I2C_readbuf(address, numberBytes, I2C_rxbuf);
}

/* Escribe el numero de registro y lee con inicio repetido (sin STOP entre
 * medias), como necesitan muchos sensores. */
uint8_t I2C_readbuf_reg(uint8_t address, uint8_t registerAddress,
			uint8_t numberBytes, uint8_t *dataBuffer)
{
	uint8_t e = twi_writeReg(address, &registerAddress, 1, 0, 0, 0);
	if (e) return I2C_mapWrite(e);
	return I2C_readbuf(address, numberBytes, dataBuffer);
}

uint8_t I2C_read_reg(uint8_t address, uint8_t registerAddress, uint8_t numberBytes)
{
	if (numberBytes > MAX_BUFFER_SIZE) numberBytes = MAX_BUFFER_SIZE;
	return I2C_readbuf_reg(address, registerAddress, numberBytes, I2C_rxbuf);
}
