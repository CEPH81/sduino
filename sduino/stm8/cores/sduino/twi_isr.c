/*
 * twi_isr.c - Vector de interrupcion I2C.
 *
 * Es minimo a proposito: siempre se enlaza (el vector esta declarado en
 * stm8s_it.h) y solo llama al gancho que instala el modo esclavo de
 * twi_slave.c. Asi el codigo del esclavo solo ocupa flash si se usa.
 *
 * Parte de sduino. LGPL 2.1.
 */
#include "Arduino.h"
#include "twi.h"

void (*twi_isr_hook)(void);

INTERRUPT_HANDLER(I2C_IRQHandler, ITC_IRQ_I2C)
{
	if (twi_isr_hook) twi_isr_hook();
}
