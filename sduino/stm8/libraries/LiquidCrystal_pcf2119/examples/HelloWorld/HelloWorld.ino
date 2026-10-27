#include <I2C.h>
#include <LiquidCrystal_pcf2119.h>

// LCD controller settings
#define LCD_ADDR    0x3B    // I2C address: 0x3A or 0x3B, dep. on pin SA0 state
#if defined(ARDUINO_STM8S_MB)
#define LCD_RST     PE3     // reset pin (active high)
#else
#define LCD_RST     PD4     // reset pin (active high): PE3 no existe en esta placa
#endif
#define LCD_CHARSET 'R'     // character set (A, D, F, I, R or S)

// Set LCD address and reset pin
LiquidCrystal_pcf2119(lcd, LCD_ADDR, LCD_RST, LCD_CHARSET);

void setup()
{
	// initialize the LCD (16x2 display)
	lcd_begin(16,2);

	// Print a message.
	lcd_print_s("Hello, world!");
}

void loop()
{
	// Do nothing here...
}
