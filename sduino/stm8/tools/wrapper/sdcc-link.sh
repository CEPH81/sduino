#!/bin/sh
#
# sdcc-link.sh - Enlaza el programa con SDCC y genera el .hex.
#
# Uso: sdcc-link.sh SDCC BASE [opciones y archivos...]
#
# BASE es la ruta de salida sin extension. Genera BASE.ihx (y BASE.map,
# que usa sdsize.sh) y lo copia como BASE.hex para la carga.
# Los .o se pasan como .rel y los .a como .lib (nombres que espera sdld).
#
# Parte de sduino. LGPL 2.1.

if ! command -v cp > /dev/null 2>&1; then
	cd "${0%/wrapper/*}"
	PATH=$(pwd)/win:$PATH
fi

SDCC=$1
BASE=$2
shift 2

n=$#
for a; do
	case "$a" in
		*.o)	a="${a%.o}.rel" ;;
		*.a)	cp -f "$a" "${a%.a}.lib"; a="${a%.a}.lib" ;;
	esac
	set -- "$@" "$a"
done
shift $n

"$SDCC" "$@" -o "$BASE.ihx"
ERR=$?
if [ $ERR -eq 0 ]; then
	cp -f "$BASE.ihx" "$BASE.hex"
fi
exit $ERR
