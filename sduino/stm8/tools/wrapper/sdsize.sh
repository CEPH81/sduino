#!/bin/sh
#
# sdsize.sh - Uso de memoria de un programa enlazado con SDCC, leido del
# archivo .map. Imprime dos lineas que lee el IDE:
#
#   FLASH <bytes>   HOME + GSINIT + GSFINAL + CONST + CODE + INITIALIZER
#   RAM <bytes>     DATA + INITIALIZED (sin contar la pila)
#
# Uso: sdsize.sh archivo.map
#
# Parte de sduino. LGPL 2.1.

# Arduino.mk (compilacion con make) pregunta por --help: con "map" le pasa el .map
if [ "$1" = "--help" ]; then
	echo "uso: sdsize.sh archivo.map"
	exit 0
fi

if ! command -v awk > /dev/null 2>&1; then
	cd "${0%/wrapper/*}"
	PATH=$(pwd)/win:$PATH
fi

awk '
/^[A-Z_]+ +[0-9A-F]+ +[0-9A-F]+ = / { size[$1] = $5 + 0 }
END {
	flash = size["HOME"] + size["GSINIT"] + size["GSFINAL"] + size["CONST"] + size["CODE"] + size["INITIALIZER"]
	ram   = size["DATA"] + size["INITIALIZED"]
	print "FLASH " flash
	print "RAM " ram
}' "$1"
