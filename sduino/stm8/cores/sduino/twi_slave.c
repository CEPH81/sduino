/*
 * twi_slave.c - Modo esclavo I2C por interrupcion (Wire.begin(direccion)).
 *
 * - Recepcion: los bytes se guardan en un buffer y, al llegar el STOP (o un
 *   inicio repetido), se llama a la funcion de onReceive con lo recibido.
 * - Transmision: al ser direccionado para leer se llama a onRequest, que
 *   carga la respuesta con twi_transmit(); si el maestro pide mas bytes de
 *   los cargados se envia 0xFF.
 *
 * Parte de sduino. LGPL 2.1.
 */
#include "Arduino.h"
#include "twi.h"

static void (*slave_rx_cb)(uint8_t *, int);
static void (*slave_tx_cb)(void);

static uint8_t rx_buf[TWI_BUFFER_LENGTH];
static uint8_t rx_len;
static uint8_t tx_buf[TWI_BUFFER_LENGTH];
static uint8_t tx_len, tx_idx;
static uint8_t receiving;

static void rx_done(void)
{
	if (receiving) {
		receiving = 0;
		if (slave_rx_cb) slave_rx_cb(rx_buf, rx_len);
	}
}

static void twi_slave_isr(void)
{
	uint8_t sr1 = I2C->SR1;
	uint8_t sr2 = I2C->SR2;

	if (sr1 & I2C_SR1_ADDR) {
		uint8_t sr3 = I2C->SR3;             /* leer SR1 y SR3 borra ADDR */
		rx_done();                          /* inicio repetido tras una escritura */
		I2C->ITR |= I2C_ITR_ITBUFEN;        /* interrupciones de RXNE/TXE */
		if (sr3 & I2C_SR3_TRA) {
			tx_len = tx_idx = 0;
			if (slave_tx_cb) slave_tx_cb();
		} else {
			rx_len = 0;
			receiving = 1;
		}
	}
	if (sr2) {
		/* AF = el maestro termino la lectura con NACK (normal): no se carga
		 * mas DR y se paran las interrupciones de TXE hasta la proxima
		 * direccion, para no dejar un byte viejo ni repetir la interrupcion. */
		I2C->SR2 = 0;
		if (sr2 & I2C_SR2_AF) I2C->ITR &= (uint8_t)~I2C_ITR_ITBUFEN;
		sr1 &= (uint8_t)~I2C_SR1_TXE;
	}
	if (sr1 & I2C_SR1_RXNE) {
		uint8_t b = I2C->DR;
		if (rx_len < TWI_BUFFER_LENGTH) rx_buf[rx_len++] = b;
	}
	if (sr1 & I2C_SR1_TXE) {
		I2C->DR = (tx_idx < tx_len) ? tx_buf[tx_idx++] : 0xFF;
	}
	if (sr1 & I2C_SR1_STOPF) {
		I2C->CR2 |= I2C_CR2_ACK;            /* leer SR1 y escribir CR2 borra STOPF */
		rx_done();
	}
}

void twi_setAddress(uint8_t address)
{
	I2C->OARL = (uint8_t)(address << 1);
	I2C->OARH = I2C_OARH_ADDCONF;
	I2C->CR2 |= I2C_CR2_ACK;
	twi_isr_hook = twi_slave_isr;
	I2C->ITR = I2C_ITR_ITEVTEN | I2C_ITR_ITBUFEN | I2C_ITR_ITERREN;   /* eventos, buffer y errores */
}

void twi_attachSlaveRxEvent(void (*function)(uint8_t *, int))
{
	slave_rx_cb = function;
}

void twi_attachSlaveTxEvent(void (*function)(void))
{
	slave_tx_cb = function;
}

uint8_t twi_transmit(const uint8_t *data, uint8_t length)
{
	uint8_t i;
	if ((uint8_t)(tx_len + length) > TWI_BUFFER_LENGTH) return 1;
	for (i = 0; i < length; i++) tx_buf[tx_len++] = data[i];
	return 0;
}
