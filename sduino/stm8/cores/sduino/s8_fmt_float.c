/*
 * s8_fmt_float.c - Serial.print(float): en un modulo aparte para que el
 * codigo de coma flotante solo se enlace si se imprime un float.
 *
 * Parte de sduino. LGPL 2.1.
 */
#include "Arduino.h"

size_t s8_fmt_float(writefunc_p w, uint8_t opt, va_list ap)
{
	return Print_print_fd(w, va_arg(ap, double), opt);
}
