/*
 * sduino_obj.h - Sintaxis estilo Arduino (C++) en C para SDCC, sin coste.
 *
 * SDCC solo compila C, pero en el sketch se puede escribir como en Arduino:
 *
 *     Serial.begin(9600);
 *     Serial.print("Valor: ");  Serial.println(x);
 *     Serial.println(x, HEX);   Serial.println(3.1416, 3);
 *     Wire.begin();  Wire.beginTransmission(0x27);  Wire.write(0x55);
 *     EEPROM.put(0, dato);      SPI.transfer(0x9F);
 *
 * Como funciona:
 * - Al compilar el sketch (.ino), el script sdcc.sh cambia "Serial.metodo("
 *   por "Serial__metodo(" (lo mismo para Wire, EEPROM y SPI). Cada
 *   Objeto__metodo es una macro de su libreria que llama directamente a la
 *   funcion en C: no hay tablas de punteros y solo se enlaza lo que se usa.
 * - Los metodos con varias versiones (print, println, write, begin,
 *   requestFrom, endTransmission...) cuentan los argumentos y, con _Generic,
 *   eligen al compilar segun el tipo del dato: el codigo para imprimir float
 *   solo se enlaza si se imprime un float.
 * - Las macros solo existen en el sketch (SDUINO_SKETCH).
 *
 * Diferencias con C++:
 * - Los objetos con constructor (LiquidCrystal lcd(...), Servo...) siguen
 *   con la sintaxis de sduino (lcd_print_s(), ...): ver su documentacion.
 * - Serial.print('A') imprime "A" como en Arduino (sdcc.sh lo convierte en
 *   "A"); con una variable char tambien se imprime el caracter.
 *
 * Parte de sduino. LGPL 2.1.
 */
#ifndef SDUINO_OBJ_H
#define SDUINO_OBJ_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <Print.h>

/* Formateadores: escriben con 'w' el argumento variable segun su tipo */
typedef size_t (*s8_fmt_t)(writefunc_p w, uint8_t opt, va_list ap);

size_t s8_fmt_char (writefunc_p w, uint8_t opt, va_list ap);   /* char -> caracter */
size_t s8_fmt_int  (writefunc_p w, uint8_t opt, va_list ap);   /* int, signed char */
size_t s8_fmt_uint (writefunc_p w, uint8_t opt, va_list ap);   /* unsigned int, uint8_t, bool */
size_t s8_fmt_long (writefunc_p w, uint8_t opt, va_list ap);
size_t s8_fmt_ulong(writefunc_p w, uint8_t opt, va_list ap);
size_t s8_fmt_float(writefunc_p w, uint8_t opt, va_list ap);   /* opt = decimales */
size_t s8_fmt_str  (writefunc_p w, uint8_t opt, va_list ap);   /* char * */

/* print/println genericos: 'w' es la funcion write(uint8_t) del objeto */
size_t s8_print  (writefunc_p w, s8_fmt_t fmt, uint8_t opt, ...);
size_t s8_println(writefunc_p w, s8_fmt_t fmt, uint8_t opt, ...);
size_t s8_write_s(writefunc_p w, const char *s);
size_t s8_write_b(writefunc_p w, uint8_t b);

/* Seleccion del formateador segun el tipo (en tiempo de compilacion) */
#define S8_FMT(x) _Generic((x), \
	char: s8_fmt_char, signed char: s8_fmt_int, unsigned char: s8_fmt_uint, \
	_Bool: s8_fmt_uint, int: s8_fmt_int, unsigned int: s8_fmt_uint, \
	long: s8_fmt_long, unsigned long: s8_fmt_ulong, float: s8_fmt_float, \
	default: s8_fmt_str)
/* con base (HEX, BIN...) o decimales: un char se imprime como numero */
#define S8_FMTB(x) _Generic((x), \
	char: s8_fmt_uint, signed char: s8_fmt_int, unsigned char: s8_fmt_uint, \
	_Bool: s8_fmt_uint, int: s8_fmt_int, unsigned int: s8_fmt_uint, \
	long: s8_fmt_long, unsigned long: s8_fmt_ulong, float: s8_fmt_float, \
	default: s8_fmt_str)
#define S8_OPT(x)  _Generic((x), float: 2, default: 10)

/* Conteo de argumentos (tambien cero) sin extensiones de GNU */
#define S8_ARG8(_0,_1,_2,_3,_4,_5,_6,_7,...) _7
#define S8_HASCOMMA(...) S8_ARG8(__VA_ARGS__,1,1,1,1,1,1,0,0)
#define S8_TRIG(...) ,
#define S8_CAT5(a,b,c,d,e) a##b##c##d##e
#define S8_ISEMPTY_(a,b,c,d) S8_HASCOMMA(S8_CAT5(S8_EMPTYCASE_,a,b,c,d))
#define S8_EMPTYCASE_0001 ,
#define S8_ISEMPTY(...) S8_ISEMPTY_(S8_HASCOMMA(__VA_ARGS__), \
	S8_HASCOMMA(S8_TRIG __VA_ARGS__), S8_HASCOMMA(__VA_ARGS__ ()), \
	S8_HASCOMMA(S8_TRIG __VA_ARGS__ ()))
#define S8_NARG_(...) S8_ARG8(__VA_ARGS__,7,6,5,4,3,2,1,0)
#define S8_CAT2(a,b) a##b
#define S8_CAT(a,b) S8_CAT2(a,b)
#define S8_NE0(n) n
#define S8_NE1(n) 0
#define S8_NARG(...) S8_CAT(S8_NE, S8_ISEMPTY(__VA_ARGS__))(S8_NARG_(__VA_ARGS__))
/* nombre##N(argumentos) segun el numero de argumentos */
#define S8_OVERLOAD(name, ...) S8_CAT(name, S8_NARG(__VA_ARGS__))(__VA_ARGS__)

/* Bloques para los objetos que "heredan" de Print (w = su write(uint8_t)) */
#define S8_PRINT(w, ...)        S8_CAT(S8_PRINT_, S8_NARG(__VA_ARGS__))(w, __VA_ARGS__)
#define S8_PRINT_1(w, x)        s8_print(w, S8_FMT(x), S8_OPT(x), x)
#define S8_PRINT_2(w, x, b)     s8_print(w, S8_FMTB(x), b, x)
#define S8_PRINTLN(w, ...)      S8_CAT(S8_PRINTLN_, S8_NARG(__VA_ARGS__))(w, __VA_ARGS__)
#define S8_PRINTLN_0(w, unused) Print_println(w)
#define S8_PRINTLN_1(w, x)      s8_println(w, S8_FMT(x), S8_OPT(x), x)
#define S8_PRINTLN_2(w, x, b)   s8_println(w, S8_FMTB(x), b, x)
/* write(x): texto o un byte; write(buffer, n) */
#define S8_WRITE(w, ...)        S8_CAT(S8_WRITE_, S8_NARG(__VA_ARGS__))(w, __VA_ARGS__)
/* (SDCC 3.9 no convierte un literal "..." en puntero dentro de _Generic: los
 * textos van por default) */
#define S8_WRITE_1(w, x)        _Generic((x), char: s8_write_b, signed char: s8_write_b, \
                                         unsigned char: s8_write_b, _Bool: s8_write_b, \
                                         int: s8_write_b, unsigned int: s8_write_b, \
                                         long: s8_write_b, unsigned long: s8_write_b, \
                                         default: s8_write_s)(w, x)
#define S8_WRITE_2(w, b, n)     Print_print_sn(w, (const uint8_t *)(b), n)

#ifndef F
#define F(s) (s)
#endif

#endif
