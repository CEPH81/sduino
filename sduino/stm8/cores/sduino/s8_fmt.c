/*
 * s8_fmt.c - print()/println()/write() de la sintaxis estilo Arduino
 * (sduino_obj.h). Cada formateador lee de la lista variable el dato con su
 * tipo ya promovido. El de float esta en s8_fmt_float.c.
 *
 * Parte de sduino. LGPL 2.1.
 */
#include "Arduino.h"

size_t s8_print(writefunc_p w, s8_fmt_t fmt, uint8_t opt, ...)
{
	size_t n;
	va_list ap;
	va_start(ap, opt);
	n = fmt(w, opt, ap);
	va_end(ap);
	return n;
}

size_t s8_println(writefunc_p w, s8_fmt_t fmt, uint8_t opt, ...)
{
	size_t n;
	va_list ap;
	va_start(ap, opt);
	n = fmt(w, opt, ap);
	va_end(ap);
	return n + Print_println(w);
}

size_t s8_write_s(writefunc_p w, const char *s)
{
	return Print_print_s(w, s);
}

size_t s8_write_b(writefunc_p w, uint8_t b)
{
	return w(b);
}

size_t s8_fmt_char(writefunc_p w, uint8_t opt, va_list ap)
{
	(void) opt;
	return w((uint8_t) va_arg(ap, int));
}

size_t s8_fmt_int(writefunc_p w, uint8_t opt, va_list ap)
{
	return Print_print_ib(w, (long) va_arg(ap, int), opt);
}

size_t s8_fmt_uint(writefunc_p w, uint8_t opt, va_list ap)
{
	return Print_print_ub(w, (unsigned long) va_arg(ap, unsigned int), opt);
}

size_t s8_fmt_long(writefunc_p w, uint8_t opt, va_list ap)
{
	return Print_print_ib(w, va_arg(ap, long), opt);
}

size_t s8_fmt_ulong(writefunc_p w, uint8_t opt, va_list ap)
{
	return Print_print_ub(w, va_arg(ap, unsigned long), opt);
}

size_t s8_fmt_str(writefunc_p w, uint8_t opt, va_list ap)
{
	(void) opt;
	return Print_print_s(w, va_arg(ap, const char *));
}
