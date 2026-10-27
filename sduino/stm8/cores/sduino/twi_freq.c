/*
 * twi_freq.c - Velocidad del bus I2C (Wire.setClock / I2C_setSpeed).
 *
 * En un modulo aparte: la division de 32 bits que necesita solo se enlaza
 * si el sketch cambia la velocidad (por defecto 100 kHz, calculado al
 * compilar en twi.c).
 *
 * Parte de sduino. LGPL 2.1.
 */
#ifndef TWI_HOST_TEST
#include "Arduino.h"
#endif
#include "twi.h"

extern uint8_t twi_ccr, twi_ccrh, twi_trise;
void twi_apply(void);

/* Todo en 16 bits: el STM8 divide 16/16 por hardware (DIVW). La conversion
 * de Hz a kHz la hace la macro twi_setFrequency() de twi.h, al compilar si
 * la frecuencia es una constante. */
#define FCPU_KHZ ((uint16_t)(F_CPU / 1000UL))

void twi_setFrequencyKHz(uint16_t khz)
{
	uint16_t ccr;

	if (khz > 400) khz = 400;
	if (khz < 10)  khz = 10;
	if (khz > 100) {
		/* modo rapido, ciclo 2:1: T = 3 * CCR / F_CPU (redondeo hacia arriba
		 * para no pasar de la frecuencia pedida) */
		ccr = (uint16_t)(FCPU_KHZ + 3 * khz - 1) / (uint16_t)(3 * khz);
		if (ccr < 1) ccr = 1;
		twi_ccrh = I2C_CCRH_FS | (uint8_t)(ccr >> 8);
		twi_trise = (uint8_t)((F_CPU / 1000000UL) * 3 / 10 + 1);   /* 300 ns */
	} else {
		/* modo estandar: T = 2 * CCR / F_CPU */
		ccr = (uint16_t)(FCPU_KHZ + 2 * khz - 1) / (uint16_t)(2 * khz);
		if (ccr < 4) ccr = 4;
		twi_ccrh = (uint8_t)(ccr >> 8);
		twi_trise = (uint8_t)(F_CPU / 1000000UL + 1);             /* 1000 ns */
	}
	twi_ccr = (uint8_t) ccr;
	twi_apply();
}
