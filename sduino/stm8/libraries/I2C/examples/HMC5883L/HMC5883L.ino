/*
 * HMC5883L - Brujula de 3 ejes por I2C con la libreria I2C.
 *
 * Configura el HMC5883L en modo continuo y muestra X, Y, Z por Serial.
 * Conexion (STM8 Black / STM8S005K6): SDA = PB5, SCL = PB4, con
 * resistencias de pull-up de 4,7 kOhm.
 */
#include <I2C.h>

#define HMC5883L  0x1E

int16_t x, y, z;

// dos bytes en big endian (en C el orden de dos llamadas en una misma
// expresion no esta definido: se leen por separado)
int16_t read16(void)
{
  uint8_t hi = I2C_receive();
  return (int16_t)((hi << 8) | I2C_receive());
}

void setup()
{
  Serial_begin(115200);
  I2C_begin();
  if (I2C_write_reg(HMC5883L, 0x02, 0x00))      // registro de modo: continuo
    Serial_println_s("No responde el HMC5883L");
}

void loop()
{
  // lee 6 bytes desde el registro 0x03 (X, Z, Y en big endian)
  if (I2C_read_reg(HMC5883L, 0x03, 6) == 0) {
    x = read16();
    z = read16();
    y = read16();
    Serial_print_s("X: ");  Serial_print_i(x);
    Serial_print_s("  Y: "); Serial_print_i(y);
    Serial_print_s("  Z: "); Serial_println_i(z);
  }
  delay(500);
}
