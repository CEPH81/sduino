/*
 * Wire_slave.c - Modo esclavo de Wire: Wire_beginSlave(direccion),
 * Wire_onReceive() y Wire_onRequest(). Usa twi_slave.c del core.
 *
 * Parte de sduino. LGPL 2.1.
 */
#include "Wire.h"

extern uint8_t Wire_rxBuffer[];
extern uint8_t Wire_rxIndex;
extern uint8_t Wire_rxLength;
extern uint8_t Wire_transmitting;
extern uint8_t (*Wire_slaveTransmit)(const uint8_t *data, uint8_t length);

static void (*user_onReceive)(int);
static void (*user_onRequest)(void);

/* Llamadas desde la interrupcion I2C */
static void onReceiveService(uint8_t *inBytes, int numBytes)
{
	uint8_t i;
	if (!user_onReceive) return;
	/* no pisar datos de un requestFrom() que aun no se han leido */
	if (Wire_rxIndex < Wire_rxLength) return;
	for (i = 0; i < (uint8_t) numBytes; i++) Wire_rxBuffer[i] = inBytes[i];
	Wire_rxIndex = 0;
	Wire_rxLength = (uint8_t) numBytes;
	user_onReceive(numBytes);
}

static void onRequestService(void)
{
	if (!user_onRequest) return;
	Wire_rxIndex = Wire_rxLength = 0;
	user_onRequest();
}

void Wire_beginSlave(uint8_t address)
{
	Wire_begin();
	Wire_transmitting = 0;
	Wire_slaveTransmit = twi_transmit;
	twi_attachSlaveTxEvent(onRequestService);
	twi_attachSlaveRxEvent(onReceiveService);
	twi_setAddress(address);
}

void Wire_onReceive(void (*function)(int))
{
	user_onReceive = function;
}

void Wire_onRequest(void (*function)(void))
{
	user_onRequest = function;
}
