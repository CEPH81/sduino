# Pruebas del controlador I2C (twi.c) en el PC, contra un modelo del I2C del STM8S.
#   make        compila y ejecuta
test_twi: test_twi.cpp twi_model.cpp ../../sduino/stm8/cores/sduino/twi.c ../../sduino/stm8/cores/sduino/twi_freq.c
	g++ -std=c++17 -O1 -w -fpermissive -o $@ test_twi.cpp

run: test_twi
	./test_twi

.DEFAULT_GOAL := run
clean:
	rm -f test_twi
