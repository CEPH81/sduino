/*
 * Wire.c - Libreria I2C (Wire) para sduino: modo maestro.
 * El controlador esta en el core (twi.c). Modo esclavo: Wire_slave.c.
 *
 * Basada en TwoWire de Arduino (Nicholas Zambetti, Todd Krein) y en la
 * version en C de Michael Mayer. LGPL 2.1.
 */
#include <string.h>
#include "Wire.h"

uint8_t Wire_rxBuffer[BUFFER_LENGTH];
uint8_t Wire_rxIndex;
uint8_t Wire_rxLength;

static uint8_t txAddress;
static uint8_t txBuffer[BUFFER_LENGTH];
static uint8_t txLength;
static uint8_t txOverflow;
uint8_t Wire_transmitting;
/* escritura en modo esclavo (dentro de onRequest): la instala Wire_slave.c,
 * asi un sketch maestro no enlaza el codigo del esclavo */
uint8_t (*Wire_slaveTransmit)(const uint8_t *data, uint8_t length);

void Wire_begin(void)
{
	Wire_rxIndex = Wire_rxLength = 0;
	txLength = 0;
	twi_init();
}

void Wire_end(void)
{
	twi_disable();
}

void Wire_setTimeout(uint16_t ms)
{
	twi_setTimeout(ms);
}

uint8_t Wire_lastError(void)
{
	return twi_lastError();
}

void Wire_beginTransmission(uint8_t address)
{
	Wire_transmitting = 1;
	txAddress = address;
	txLength = 0;
	txOverflow = 0;
}

uint8_t Wire_endTransmission1(uint8_t sendStop)
{
	uint8_t ret;
	if (txOverflow) {
		ret = TWI_ERR_LENGTH;
	} else {
		ret = twi_writeTo(txAddress, txBuffer, txLength, sendStop);
	}
	txLength = 0;
	Wire_transmitting = 0;
	return ret;
}

uint8_t Wire_endTransmission0(void)
{
	return Wire_endTransmission1(1);
}

uint8_t Wire_requestFrom3(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
	if (quantity > BUFFER_LENGTH) quantity = BUFFER_LENGTH;
	Wire_rxLength = twi_readFrom(address, Wire_rxBuffer, quantity, sendStop);
	Wire_rxIndex = 0;
	return Wire_rxLength;
}

uint8_t Wire_requestFrom2(uint8_t address, uint8_t quantity)
{
	return Wire_requestFrom3(address, quantity, 1);
}

size_t Wire_write(uint8_t data)
{
	if (Wire_transmitting) {
		if (txLength >= BUFFER_LENGTH) {
			txOverflow = 1;
			return 0;
		}
		txBuffer[txLength++] = data;
		return 1;
	}
	/* modo esclavo, dentro de onRequest */
	if (Wire_slaveTransmit && !Wire_slaveTransmit(&data, 1)) return 1;
	return 0;
}

size_t Wire_write_sn(const uint8_t *data, size_t quantity)
{
	size_t n = 0;
	while (quantity--) n += Wire_write(*data++);
	return n;
}

size_t Wire_write_s(const char *str)
{
	return Wire_write_sn((const uint8_t *) str, strlen(str));
}

int Wire_available(void)
{
	return Wire_rxLength - Wire_rxIndex;
}

int Wire_read(void)
{
	if (Wire_rxIndex < Wire_rxLength) return Wire_rxBuffer[Wire_rxIndex++];
	return -1;
}

int Wire_peek(void)
{
	if (Wire_rxIndex < Wire_rxLength) return Wire_rxBuffer[Wire_rxIndex];
	return -1;
}

void Wire_flush(void)
{
}
