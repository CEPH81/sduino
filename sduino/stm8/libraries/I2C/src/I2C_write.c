/* I2C_write.c - Escritura (libreria I2C). LGPL 2.1. */
#include <Arduino.h>
#include <string.h>
#include "I2C.h"
#include "I2C_private.h"

/* --- escritura ----------------------------------------------------------- */

uint8_t I2C_write(uint8_t address, uint8_t registerAddress)
{
	return I2C_mapWrite(twi_writeReg(address, &registerAddress, 1, 0, 0, 1));
}

uint8_t I2C_write_reg(uint8_t address, uint8_t registerAddress, uint8_t data)
{
	return I2C_mapWrite(twi_writeReg(address, &registerAddress, 1, &data, 1, 1));
}

uint8_t I2C_write_s(uint8_t address, uint8_t registerAddress, char *data)
{
	return I2C_mapWrite(twi_writeReg(address, &registerAddress, 1,
				      (const uint8_t *) data, (uint8_t) strlen(data), 1));
}

uint8_t I2C_write_sn(uint8_t address, uint8_t registerAddress, uint8_t *data,
		     uint8_t numberBytes)
{
	return I2C_mapWrite(twi_writeReg(address, &registerAddress, 1, data, numberBytes, 1));
}

