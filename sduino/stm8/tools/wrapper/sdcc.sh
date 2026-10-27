#!/bin/sh
#
# sdcc.sh - Compila un archivo con SDCC con una interfaz parecida a gcc.
#
# Uso: sdcc.sh SDCC FUENTE OBJETO MARCA [opciones de sdcc...]
#
# - Los .cpp (solo el sketch .ino convertido por el IDE) se copian a .c y se
#   compilan como C. Se antepone SDUINO_SKETCH y se cambia "Serial.metodo("
#   por "Serial__metodo(" (Wire, EEPROM, SPI): la sintaxis estilo Arduino
#   son macros que llaman directamente a las funciones (ver sduino_obj.h).
# - SDCC genera .rel: se copia tambien como .o para que el IDE no recompile.
# - En Windows se ejecuta con busybox (tools/win).
#
# Parte de sduino. LGPL 2.1.

if ! command -v cp > /dev/null 2>&1; then
	# Windows: busybox no tiene cp/rm en el PATH; estan en tools/win
	cd "${0%/wrapper/*}"
	PATH=$(pwd)/win:$PATH
fi

SDCC=$1
SRC=$2
OBJ=$3
shift 4
REL=${OBJ%.o}.rel

case "$SRC" in
	*.cpp)
		CSRC="${SRC%pp}"
		{
			echo "#define SDUINO_SKETCH 1"
			# referencia a main() para que se enlace main.c del core
			echo "void main(void); void (*sduino_main_ref)(void) = main;"
			# En C 'x' es un int: .print('x') imprimiria el codigo numerico.
			# Se convierte en .print("x") para que se comporte como en Arduino.
			# "Serial.metodo(" -> "Serial__metodo(" (igual con Wire, EEPROM y SPI):
			# macros que llaman directamente a las funciones (sduino_obj.h).
			sed -E \
				-e "s/(\.print|\.println)\([[:space:]]*'\"'[[:space:]]*\)/\1(\"\\\\\"\")/g" \
				-e "s/(\.print|\.println)\([[:space:]]*'([^'\\\\]|\\\\.)'[[:space:]]*\)/\1(\"\2\")/g" \
				-e ":objeto" \
				-e "s/(^|[^A-Za-z0-9_.])(Serial|Wire|EEPROM|SPI)[[:space:]]*\.[[:space:]]*([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*\(/\1\2__\3(/" \
				-e "t objeto" \
				"$SRC"
		} > "$CSRC"
		"$SDCC" "$@" "$CSRC" -o "$OBJ"
		ERR=$?
		rm -f "$CSRC"
		;;
	*)
		"$SDCC" "$@" "$SRC" -o "$OBJ"
		ERR=$?
		;;
esac

# el IDE espera un .o: copia del .rel
if [ $ERR -eq 0 ] && [ -e "$REL" ] && [ "$REL" != "$OBJ" ]; then
	cp -f "$REL" "$OBJ"
fi

exit $ERR
