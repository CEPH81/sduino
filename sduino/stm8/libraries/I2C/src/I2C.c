/*
  I2C.c - Libreria I2C (API de Wayne Truchsess) para sduino / STM8S.

  Version 7: usa el controlador twi.c del core (el mismo que Wire), a nivel
  de registros y sin la libreria SPL de ST:
  - menos flash (no enlaza stm8s_i2c.c),
  - velocidad correcta para cualquier F_CPU (antes suponia 16 MHz),
  - nunca se cuelga: tiempo de espera en cada paso, tope de vueltas aunque
    millis() no avance, y recuperacion del bus si un esclavo deja SDA en bajo,
  - inicio repetido real en I2C_read_reg().

  La API y los codigos de retorno son los de siempre (ver I2C.h).

  Historia: port del I2C de Wayne Truchsess (2011-2012) reescrito para el
  STM8S por Michael Mayer (2017-2018). Version 7 para sduino / IDE 2.x.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

#include <Arduino.h>
#include <string.h>
#include "I2C.h"
#include "I2C_private.h"

uint8_t I2C_rxbuf[MAX_BUFFER_SIZE];
uint8_t I2C_bytesAvailable;
uint8_t I2C_totalBytes;

/* Traduce un error de twi.c a los codigos historicos de esta libreria */
uint8_t I2C_mapWrite(uint8_t e)
{
	switch (e) {
	case TWI_OK:            return 0;
	case TWI_ERR_NACK_ADDR: return 2;
	case TWI_ERR_NACK_DATA: return 3;
	case TWI_ERR_TIMEOUT:   return 1;
	default:                return LOST_ARBTRTN;
	}
}

uint8_t I2C_mapRead(uint8_t e)
{
	if (e == TWI_ERR_NACK_ADDR) return 5;
	if (e == TWI_OK)            return 0;
	return 6;
}

/* --- configuracion ------------------------------------------------------ */

void I2C_begin(void)
{
	I2C_bytesAvailable = I2C_totalBytes = 0;
	twi_setTimeout(10);             /* como la libreria original */
	twi_init();                     /* 100 kHz */
}

void I2C_end(void)
{
	twi_disable();
}

void I2C_timeOut(uint16_t ms)
{
	twi_setTimeout(ms);
}

void I2C_setSpeed(uint8_t fast)
{
	twi_setFrequency(fast ? 400000UL : 100000UL);
}

/* Las patas I2C del STM8S son de drenador abierto real y no tienen
 * resistencia de pull-up interna: hay que ponerlas fuera (4,7 kOhm). */
void I2C_pullup(uint8_t activate)
{
	(void) activate;
}

/* --- datos recibidos ----------------------------------------------------- */

uint8_t I2C_available(void)
{
	return I2C_bytesAvailable;
}

uint8_t I2C_receive(void)
{
	if (!I2C_bytesAvailable) return 0;
	return I2C_rxbuf[I2C_totalBytes - I2C_bytesAvailable--];
}

