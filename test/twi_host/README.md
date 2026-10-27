# Pruebas del controlador I2C (`twi.c`)

`twi.c` (el controlador I2C del core, que usan `Wire` e `I2C`) se compila en el
PC contra un **modelo de comportamiento del periferico I2C del STM8S**
(RM0016): banderas SB/ADDR/TXE/BTF/RXNE, ACK/POS/STOP, retencion del reloj y
esclavos simulados que registran lo que ven en el bus.

Se comprueba:

- sondeo de direcciones presentes y ausentes (NACK) y que el bus queda libre;
- escritura de registro + datos;
- lecturas de 1, 2, 3, 4, 10 y 32 bytes con inicio repetido: datos correctos,
  NACK solo en el ultimo byte, el esclavo no envia bytes de mas y STOP final;
- NACK a un dato a mitad de una escritura;
- bus bloqueado: tiempo agotado y recuperacion, tambien con `millis()` detenido
  (como dentro de una interrupcion);
- registros de velocidad para 10, 50, 100 y 400 kHz a 16 MHz;
- 3000 operaciones al azar con un bus de ritmo irregular.

Cada prueba se repite con el bus a distintas velocidades relativas a la CPU.

```
make
```
