#include "HardwareSerial.c.h"
/**
 */
int HardwareSerial_available(void)
{
  return (uint8_t)(rx_buffer.head - rx_buffer.tail) & (SERIAL_BUFFER_SIZE - 1);
}

