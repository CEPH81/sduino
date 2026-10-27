/*
 * Wire.h - Libreria I2C (Wire) para sduino / STM8S.
 *
 * Usa el controlador twi.c del core: registros directos, tiempo de espera
 * en todas las operaciones (no se cuelga), recuperacion del bus y
 * velocidad exacta (100 kHz por defecto, hasta 400 kHz).
 *
 * Dos formas de uso:
 *
 *   Estilo Arduino (en el sketch):        Estilo C de sduino:
 *     Wire.begin();                         Wire_begin();
 *     Wire.setClock(400000);                Wire_setClock(400000);
 *     Wire.beginTransmission(0x27);         Wire_beginTransmission(0x27);
 *     Wire.write(0x55);                     Wire_write(0x55);
 *     err = Wire.endTransmission();         err = Wire_endTransmission();
 *     Wire.requestFrom(0x68, 6);            Wire_requestFrom(0x68, 6);
 *     while (Wire.available())              while (Wire_available())
 *       b = Wire.read();                      b = Wire_read();
 *
 * Esclavo: Wire.begin(direccion) + Wire.onReceive(f) / Wire.onRequest(f).
 *
 * Pines: SCL = PB4, SDA = PB5 (STM8S103/105/005). Son de drenador abierto
 * verdadero: ponga resistencias de pull-up externas (4,7 kOhm a 3,3/5 V).
 *
 * endTransmission() devuelve: 0 ok, 1 demasiados datos, 2 NACK a la
 * direccion, 3 NACK a un dato, 4 otro error, 5 tiempo agotado.
 *
 * Basada en TwoWire de Arduino (Nicholas Zambetti, Todd Krein) y en la
 * version en C de Michael Mayer. LGPL 2.1.
 */
#ifndef TwoWire_h
#define TwoWire_h

#include <Arduino.h>
#include <twi.h>

#define BUFFER_LENGTH   TWI_BUFFER_LENGTH
#define WIRE_HAS_END    1
#define WIRE_HAS_TIMEOUT 1

/* --- interfaz en C --- */
void    Wire_begin(void);
void    Wire_beginSlave(uint8_t address);
void    Wire_end(void);
/* velocidad en Hz (10000..400000); con una constante no hay division en el micro */
#define Wire_setClock(hz)       twi_setFrequency(hz)
void    Wire_setTimeout(uint16_t ms);
uint8_t Wire_lastError(void);

void    Wire_beginTransmission(uint8_t address);
uint8_t Wire_endTransmission0(void);
uint8_t Wire_endTransmission1(uint8_t sendStop);
#define Wire_endTransmission()  Wire_endTransmission1(1)

uint8_t Wire_requestFrom2(uint8_t address, uint8_t quantity);
uint8_t Wire_requestFrom3(uint8_t address, uint8_t quantity, uint8_t sendStop);
uint8_t Wire_requestFrom5(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop);

size_t  Wire_write(uint8_t data);
size_t  Wire_write_s(const char *str);
size_t  Wire_write_sn(const uint8_t *data, size_t quantity);
int     Wire_available(void);
int     Wire_read(void);
int     Wire_peek(void);
void    Wire_flush(void);

void    Wire_onReceive(void (*function)(int));
void    Wire_onRequest(void (*function)(void));

#include <xmacro.h>
#include <Print.h>

/* requestFrom con 2, 3 o 5 argumentos */
#define Wire_requestFrom(...) VARFUNC(Wire_requestFrom, __VA_ARGS__)

/* Wire_print_s(), Wire_println_i()... como Serial */
XPrintMethods(Wire, Wire)

/* --- sintaxis estilo Arduino en el sketch: Wire.begin()... ---
 * sdcc.sh convierte "Wire.metodo(" en "Wire__metodo(" y estas macros
 * llaman directamente a las funciones en C (ver sduino_obj.h). */
#include <sduino_obj.h>
#ifdef SDUINO_SKETCH
#define Wire__begin(...)              S8_OVERLOAD(Wire__begin, __VA_ARGS__)
#define Wire__begin0()                Wire_begin()
#define Wire__begin1(address)         Wire_beginSlave(address)
#define Wire__end()                   Wire_end()
#define Wire__setClock(hz)            Wire_setClock(hz)
#define Wire__setTimeout(ms)          Wire_setTimeout(ms)
/* setWireTimeout(us, reset) de Arduino: aqui siempre se reinicia el bus */
#define Wire__setWireTimeout(...)     S8_OVERLOAD(Wire__setWireTimeout, __VA_ARGS__)
#define Wire__setWireTimeout0()       Wire_setTimeout(25)
#define Wire__setWireTimeout1(us)     Wire_setTimeout((uint16_t)(((us) + 999UL) / 1000UL))
#define Wire__setWireTimeout2(us, r)  Wire_setTimeout((uint16_t)(((us) + 999UL) / 1000UL))
#define Wire__getWireTimeoutFlag()    (Wire_lastError() == TWI_ERR_TIMEOUT)
#define Wire__clearWireTimeoutFlag()  ((void) 0)
#define Wire__lastError()             Wire_lastError()
#define Wire__beginTransmission(a)    Wire_beginTransmission(a)
#define Wire__endTransmission(...)    S8_OVERLOAD(Wire__endTransmission, __VA_ARGS__)
#define Wire__endTransmission0()      Wire_endTransmission1(1)
#define Wire__endTransmission1(stop)  Wire_endTransmission1(stop)
#define Wire__requestFrom(...)        S8_OVERLOAD(Wire__requestFrom, __VA_ARGS__)
#define Wire__requestFrom2(a, n)      Wire_requestFrom2(a, n)
#define Wire__requestFrom3(a, n, s)   Wire_requestFrom3(a, n, s)
#define Wire__requestFrom5(a, n, ia, is, s) Wire_requestFrom5(a, n, ia, is, s)
#define Wire__write(...)              S8_OVERLOAD(Wire__write, __VA_ARGS__)
#define Wire__write1(x)               _Generic((x), char: Wire_write, signed char: Wire_write, \
                                               unsigned char: Wire_write, _Bool: Wire_write, \
                                               int: Wire_write, unsigned int: Wire_write, \
                                               long: Wire_write, unsigned long: Wire_write, \
                                               default: Wire_write_s)(x)
#define Wire__write2(buf, n)          Wire_write_sn((const uint8_t *)(buf), n)
#define Wire__available()             Wire_available()
#define Wire__read()                  Wire_read()
#define Wire__peek()                  Wire_peek()
#define Wire__flush()                 Wire_flush()
#define Wire__onReceive(f)            Wire_onReceive(f)
#define Wire__onRequest(f)            Wire_onRequest(f)
#define Wire__print(...)              S8_PRINT(Wire_write, __VA_ARGS__)
#define Wire__println(...)            S8_PRINTLN(Wire_write, __VA_ARGS__)
#endif

#endif
