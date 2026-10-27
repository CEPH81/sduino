/*
 * twi.c - Controlador I2C maestro del STM8S (sondeo, a nivel de registros).
 *
 * Secuencias de lectura segun RM0016 (STM8S), seccion I2C "master receiver":
 * casos de 1, 2 y mas de 2 bytes, con las partes criticas sin interrupciones.
 *
 * Parte de sduino. LGPL 2.1.
 */
#ifndef TWI_HOST_TEST
#include "Arduino.h"
#endif
#include "twi.h"

/* Acceso a registros (se sustituye en las pruebas en el PC) */
#ifndef TWI_REG
#define TWI_REG(r)          (I2C->r)
#define TWI_CRITICAL_BEGIN  __critical {
#define TWI_CRITICAL_END    }
#endif
/* lectura cuyo valor no se usa (borra banderas como ADDR) */
#define TWI_DISCARD(r)      ((void)(uint8_t) TWI_REG(r))

/* Pines del I2C para la recuperacion del bus */
#if defined(STM8S208) || defined(STM8S207) || defined(STM8S007) || defined(STM8AF52Ax) || defined(STM8AF62Ax)
#define TWI_PORT    GPIOE
#define TWI_SCL     0x02    /* PE1 */
#define TWI_SDA     0x04    /* PE2 */
#else
#define TWI_PORT    GPIOB
#define TWI_SCL     0x10    /* PB4 */
#define TWI_SDA     0x20    /* PB5 */
#endif

#define SR2_ERRORS  (I2C_SR2_AF | I2C_SR2_ARLO | I2C_SR2_BERR)

static uint16_t twi_timeout = 25;
static uint16_t twi_t0;
static uint16_t twi_spins;          /* tope de vueltas: por si millis() no avanza */
/* reloj del bus: 100 kHz por defecto, calculado al compilar (twi_freq.c los cambia) */
uint8_t twi_ccr  = (uint8_t)((F_CPU + 2 * 100000UL - 1) / (2 * 100000UL));
uint8_t twi_ccrh = 0;
uint8_t twi_trise = (uint8_t)(F_CPU / 1000000UL + 1);
static uint8_t  twi_held;          /* bus tomado sin STOP (inicio repetido pendiente) */
static uint8_t  twi_error;

/* ---------------------------------------------------------- utilidades */

static void tstart(void)
{
	twi_t0 = (uint16_t) millis();
	twi_spins = 0;
}

/* Tiempo agotado. Ademas del limite en ms hay un tope de 65536 vueltas
 * (unos 150 ms a 16 MHz) para no colgarse si se usa con las interrupciones
 * deshabilitadas (dentro de una interrupcion millis() no avanza). */
static uint8_t texpired(void)
{
	if (!twi_timeout) return 0;
	if (++twi_spins == 0) return 1;
	return (uint16_t)((uint16_t) millis() - twi_t0) >= twi_timeout;
}

/* Espera a que SR1 tenga alguno de los bits de 'mask'.
 * Devuelve 0, TWI_ERR_NACK_DATA (AF), TWI_ERR_OTHER o TWI_ERR_TIMEOUT. */
static uint8_t wait_sr1(uint8_t mask)
{
	uint8_t sr2;
	tstart();
	while (!(TWI_REG(SR1) & mask)) {
		sr2 = TWI_REG(SR2);
		if (sr2 & SR2_ERRORS) {
			TWI_REG(SR2) = 0;
			return (sr2 & I2C_SR2_AF) ? TWI_ERR_NACK_DATA : TWI_ERR_OTHER;
		}
		if (texpired()) return TWI_ERR_TIMEOUT;
	}
	return TWI_OK;
}

/* Aplica la configuracion de reloj */
void twi_apply(void)
{
	TWI_REG(CR1)    = 0;
	TWI_REG(FREQR)  = (uint8_t)(F_CPU / 1000000UL);
	TWI_REG(CCRL)   = twi_ccr;
	TWI_REG(CCRH)   = twi_ccrh;
	TWI_REG(TRISER) = twi_trise;
	TWI_REG(CR1)    = I2C_CR1_PE;
}

static void twi_waitStop(void);

/* Termina una operacion fallida dejando el bus libre */
static uint8_t twi_fail(uint8_t err)
{
	twi_held = 0;
	if (err == TWI_ERR_NACK_ADDR || err == TWI_ERR_NACK_DATA) {
		TWI_REG(CR2) |= I2C_CR2_STOP;           /* el esclavo dijo NACK: STOP normal */
		twi_waitStop();                         /* y se vuelve con el bus libre */
	} else {
		twi_recoverBus();                       /* bus colgado o error: reiniciar */
	}
	twi_error = err;
	return err;
}

/* START + direccion. Al volver con 0, ADDR sigue a 1 (lo borra quien llama). */
static uint8_t twi_start(uint8_t slarw)
{
	uint8_t e;

	if (!twi_held) {
		tstart();
		while (TWI_REG(SR3) & I2C_SR3_BUSY) {
			if (texpired()) return twi_fail(TWI_ERR_TIMEOUT);
		}
	}
	twi_held = 0;
	TWI_REG(CR2) = (TWI_REG(CR2) & (uint8_t)~I2C_CR2_POS) | I2C_CR2_ACK | I2C_CR2_START;
	if ((e = wait_sr1(I2C_SR1_SB))) return twi_fail(e);
	TWI_REG(DR) = slarw;
	e = wait_sr1(I2C_SR1_ADDR);
	if (e == TWI_ERR_NACK_DATA) e = TWI_ERR_NACK_ADDR;
	if (e) return twi_fail(e);
	return TWI_OK;
}

/* Espera a que el hardware termine de generar el STOP */
static void twi_waitStop(void)
{
	tstart();
	while (TWI_REG(CR2) & I2C_CR2_STOP) {
		if (texpired()) { twi_recoverBus(); return; }
	}
}

/* ------------------------------------------------------------- publico */

void twi_init(void)
{
	twi_held = 0;
	twi_apply();
}

void twi_disable(void)
{
	TWI_REG(CR1) = 0;
}

void twi_setTimeout(uint16_t ms)
{
	twi_timeout = ms;
}

uint8_t twi_lastError(void)
{
	return twi_error;
}

/* Escribe 'hlen' bytes de cabecera (p. ej. el numero de registro) seguidos
 * de 'length' bytes de datos, en una sola transaccion. */
uint8_t twi_writeReg(uint8_t address, const uint8_t *hdr, uint8_t hlen,
		     const uint8_t *data, uint8_t length, uint8_t sendStop)
{
	uint8_t e, sent = hlen | length;

	if ((e = twi_start((uint8_t)(address << 1)))) return e;
	TWI_DISCARD(SR3);                        /* borra ADDR */
	for (;;) {
		if (!hlen) {
			if (!length) break;
			hdr = data; hlen = length; length = 0;   /* sigue con los datos */
		}
		if ((e = wait_sr1(I2C_SR1_TXE))) return twi_fail(e);
		TWI_REG(DR) = *hdr++;
		hlen--;
	}
	/* espera a que el ultimo byte salga y se confirme (BTF). Si solo se
	 * envio la direccion (sondeo del bus) no hay nada que esperar. */
	if (sent && (e = wait_sr1(I2C_SR1_BTF))) return twi_fail(e);
	if (sendStop) {
		TWI_REG(CR2) |= I2C_CR2_STOP;
		twi_waitStop();
	} else {
		twi_held = 1;                           /* el siguiente START es repetido */
	}
	twi_error = TWI_OK;
	return TWI_OK;
}

uint8_t twi_writeTo(uint8_t address, const uint8_t *data, uint8_t length, uint8_t sendStop)
{
	return twi_writeReg(address, data, length, 0, 0, sendStop);
}

uint8_t twi_probe(uint8_t address)
{
	return twi_writeTo(address, 0, 0, 1);
}

uint8_t twi_readFrom(uint8_t address, uint8_t *data, uint8_t length, uint8_t sendStop)
{
	uint8_t e, n = length;

	(void) sendStop;    /* el STM8 siempre termina la lectura con STOP */
	if (!length) return 0;
	if ((e = twi_start((uint8_t)((address << 1) | 1)))) return 0;

	if (n == 1) {
		TWI_REG(CR2) &= (uint8_t)~I2C_CR2_ACK;
		TWI_CRITICAL_BEGIN
			TWI_DISCARD(SR3);                    /* borra ADDR */
			TWI_REG(CR2) |= I2C_CR2_STOP;
		TWI_CRITICAL_END
		if ((e = wait_sr1(I2C_SR1_RXNE))) goto fail;
		*data = TWI_REG(DR);
	} else if (n == 2) {
		TWI_REG(CR2) |= I2C_CR2_POS;
		TWI_CRITICAL_BEGIN
			TWI_DISCARD(SR3);
			TWI_REG(CR2) &= (uint8_t)~I2C_CR2_ACK;
		TWI_CRITICAL_END
		if ((e = wait_sr1(I2C_SR1_BTF))) goto fail;
		TWI_CRITICAL_BEGIN
			TWI_REG(CR2) |= I2C_CR2_STOP;
			data[0] = TWI_REG(DR);
		TWI_CRITICAL_END
		data[1] = TWI_REG(DR);
		TWI_REG(CR2) &= (uint8_t)~I2C_CR2_POS;
	} else {
		TWI_DISCARD(SR3);
		while (n > 3) {
			if ((e = wait_sr1(I2C_SR1_RXNE))) goto fail;
			*data++ = TWI_REG(DR);
			n--;
		}
		/* quedan 3: N-2 en DR y N-1 en el registro de desplazamiento */
		if ((e = wait_sr1(I2C_SR1_BTF))) goto fail;
		TWI_REG(CR2) &= (uint8_t)~I2C_CR2_ACK;
		TWI_CRITICAL_BEGIN
			*data++ = TWI_REG(DR);
			TWI_REG(CR2) |= I2C_CR2_STOP;
			*data++ = TWI_REG(DR);
		TWI_CRITICAL_END
		if ((e = wait_sr1(I2C_SR1_RXNE))) goto fail;
		*data = TWI_REG(DR);
	}
	twi_waitStop();
	twi_error = TWI_OK;
	return length;

fail:
	twi_fail(e);
	return 0;
}

/* Libera un bus bloqueado: un esclavo que se quedo a mitad de un byte
 * mantiene SDA en bajo. Se generan hasta 9 pulsos de SCL y un STOP, y se
 * reinicia el periferico. Devuelve 1 si SDA quedo en alto. */
uint8_t twi_recoverBus(void)
{
#ifndef TWI_HOST_TEST
	uint8_t i, ok;

	TWI_REG(CR1) = 0;                         /* los pines vuelven a ser GPIO */
	TWI_PORT->ODR |= TWI_SCL | TWI_SDA;
	TWI_PORT->DDR |= TWI_SCL | TWI_SDA;       /* salidas de drenador abierto */
	for (i = 0; i < 9 && !(TWI_PORT->IDR & TWI_SDA); i++) {
		TWI_PORT->ODR &= (uint8_t)~TWI_SCL;
		delayMicroseconds(5);
		TWI_PORT->ODR |= TWI_SCL;
		delayMicroseconds(5);
	}
	/* STOP: SDA sube con SCL en alto */
	TWI_PORT->ODR &= (uint8_t)~TWI_SDA;
	delayMicroseconds(5);
	TWI_PORT->ODR |= TWI_SDA;
	delayMicroseconds(5);
	ok = (TWI_PORT->IDR & TWI_SDA) ? 1 : 0;
	TWI_PORT->DDR &= (uint8_t)~(TWI_SCL | TWI_SDA);

	TWI_REG(CR2) = I2C_CR2_SWRST;             /* reinicio del periferico */
	TWI_REG(CR2) = 0;
	twi_held = 0;
	twi_apply();
	return ok;
#else
	twi_held = 0;
	twi_host_recover();
	return 1;
#endif
}
