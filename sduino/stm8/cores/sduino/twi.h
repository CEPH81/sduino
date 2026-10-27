/*
 * twi.h - Controlador I2C (TWI) del STM8S a nivel de registros.
 *
 * Lo usan las librerias Wire e I2C. Va en el core para que ambas
 * compartan el mismo codigo; solo se enlaza si un sketch lo usa.
 *
 * - Maestro por sondeo, sin la libreria SPL: codigo compacto.
 * - Tiempo de espera en todas las esperas (por defecto 25 ms): nunca se
 *   queda colgado si un dispositivo no responde o falta una resistencia.
 * - Recuperacion del bus: si SDA queda en bajo, genera hasta 9 pulsos de
 *   reloj y un STOP, y reinicia el periferico.
 * - 100 kHz, 400 kHz o cualquier frecuencia intermedia, calculada para el
 *   F_CPU real.
 * - Esclavo por interrupcion (onReceive/onRequest de Wire).
 *
 * Codigos de error (como Wire de Arduino):
 *   0 ok, 1 datos demasiado largos, 2 NACK a la direccion,
 *   3 NACK a un dato, 4 otro error (bus, arbitraje), 5 tiempo agotado
 *
 * Parte de sduino. LGPL 2.1.
 */
#ifndef SDUINO_TWI_H
#define SDUINO_TWI_H

#include <stdint.h>

#ifndef TWI_BUFFER_LENGTH
#define TWI_BUFFER_LENGTH 32
#endif

#define TWI_OK              0
#define TWI_ERR_LENGTH      1
#define TWI_ERR_NACK_ADDR   2
#define TWI_ERR_NACK_DATA   3
#define TWI_ERR_OTHER       4
#define TWI_ERR_TIMEOUT     5

/* Maestro */
void    twi_init(void);                       /* 100 kHz, timeout 25 ms */
void    twi_disable(void);
void    twi_setFrequencyKHz(uint16_t khz);    /* 10 .. 400 kHz */
/* en Hz; con una constante la division se hace al compilar */
#define twi_setFrequency(hz) twi_setFrequencyKHz((uint16_t)((uint32_t)(hz) / 1000UL))
void    twi_setTimeout(uint16_t ms);          /* 0 = esperar sin limite */
uint8_t twi_writeTo(uint8_t address, const uint8_t *data, uint8_t length, uint8_t sendStop);
/* cabecera (registro) + datos en una sola transaccion, sin copiar a un buffer */
uint8_t twi_writeReg(uint8_t address, const uint8_t *hdr, uint8_t hlen,
                     const uint8_t *data, uint8_t length, uint8_t sendStop);
uint8_t twi_readFrom(uint8_t address, uint8_t *data, uint8_t length, uint8_t sendStop);
uint8_t twi_probe(uint8_t address);           /* 0 si el dispositivo responde */
uint8_t twi_lastError(void);                  /* error de la ultima operacion */
uint8_t twi_recoverBus(void);                 /* 1 si SDA quedo libre */

/* Esclavo (interrupcion) */
void    twi_setAddress(uint8_t address);
void    twi_attachSlaveRxEvent(void (*function)(uint8_t *data, int length));
void    twi_attachSlaveTxEvent(void (*function)(void));
uint8_t twi_transmit(const uint8_t *data, uint8_t length);  /* dentro de onRequest */

/* Gancho del vector de interrupcion I2C (lo instala el modo esclavo) */
extern void (*twi_isr_hook)(void);

#endif
