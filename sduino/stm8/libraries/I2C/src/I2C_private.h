/* I2C_private.h - Estado compartido entre los modulos de la libreria I2C */
#ifndef I2C_PRIVATE_H
#define I2C_PRIVATE_H
#define MAX_BUFFER_SIZE 32
extern uint8_t I2C_rxbuf[MAX_BUFFER_SIZE];
extern uint8_t I2C_bytesAvailable;
extern uint8_t I2C_totalBytes;
uint8_t I2C_mapWrite(uint8_t e);
uint8_t I2C_mapRead(uint8_t e);
#endif
