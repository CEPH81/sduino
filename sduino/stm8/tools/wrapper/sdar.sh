#!/bin/sh
#
# sdar.sh - Agrega un objeto a una libreria con sdar.
#
# Uso: sdar.sh SDAR LIBRERIA.a OBJETO.o MARCA [opciones de sdar...]
#
# sdar trabaja con .rel; la libreria se copia tambien como .lib, que es
# el nombre que espera el enlazador de SDCC.
#
# Parte de sduino. LGPL 2.1.

if ! command -v cp > /dev/null 2>&1; then
	cd "${0%/wrapper/*}"
	PATH=$(pwd)/win:$PATH
fi

SDAR=$1
LIB=$2
OBJ=${3%.o}.rel
shift 4

"$SDAR" "$@" "$LIB" "$OBJ"
ERR=$?
cp -f "$LIB" "${LIB%.a}.lib"
exit $ERR
