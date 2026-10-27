# sduino para Arduino IDE 2.x — STM8S

Núcleo Arduino para microcontroladores **STM8S** compilado con **SDCC**, adaptado al
**Arduino IDE 2.x** a partir de [sduino de Michael Mayer](https://github.com/tenbaht/sduino).

Placas principales de esta versión:

| Placa | Micro | Flash | RAM | EEPROM |
|---|---|---|---|---|
| **STM8S105K4T6 Breakout Board (STM8 Black)** | STM8S105K4T6 | 16 KB | 2 KB | 1 KB |
| **STM8S005K6T6 (pinout STM8 Black)** — nueva | STM8S005K6T6 | 32 KB | 2 KB | 128 B |
| **STM8S003F3P6 (pinout STM8S103F3)** — nueva | STM8S003F3P6 | 8 KB | 1 KB | 128 B |

También siguen: STM8S103F3 (placa azul), STM8S105C6 Discovery, sduino UNO (STM8S105K6) y sduino MB (STM8S208MB).

## Novedades de la 0.6.0

- **Arduino IDE 2.x**: compila, genera los prototipos de funciones y carga con
  **ST-Link sin elegir puerto**. Funciona en Windows (con el `busybox` incluido), Linux y macOS.
- **Sintaxis estilo Arduino sin coste**: en el sketch se puede escribir
  `Serial.begin(9600)`, `Serial.println(x, HEX)`, `Wire.beginTransmission(0x27)`,
  `EEPROM.put(0, dato)`, `SPI.transfer(b)`… Cada llamada se convierte al compilar en
  una llamada directa a la función en C: no ocupa más que la sintaxis C (`Serial_print_s()`…)
  y solo se enlaza lo que se usa. La sintaxis C de siempre sigue funcionando.
- **I²C nuevo** (controlador `twi` en el core, compartido por `Wire` e `I2C`):
  - nunca se cuelga: tiempo de espera en cada paso, tope de vueltas aunque `millis()` no avance
    y **recuperación del bus** (9 pulsos de reloj + STOP) si un esclavo deja SDA en bajo;
  - velocidad exacta para el F_CPU real, de 10 a 400 kHz (`Wire.setClock(400000)`);
  - secuencias de lectura de 1, 2 y N bytes según el manual (RM0016) e inicio repetido real;
  - modo esclavo por interrupción (`Wire.begin(direccion)`, `onReceive`, `onRequest`);
  - `Wire`: un 7 % menos de flash de media (hasta un 11 %); librería `I2C`: un 16 % menos.
- **Menos flash en todos los ejemplos** (comparado con sduino 0.5, misma placa y compilador):
  pantallas LCD por I²C −18 %, OLED SSD1306 −21 %, Stepper −2 %, el resto −0,5 a −1 %;
  −5,5 % en total y ningún ejemplo ocupa más que antes. 18 ejemplos que antes no
  compilaban en la STM8 Black ahora sí.
- `Serial`: buffers de 64 bytes en los micros con 2 KB de RAM (16 en los de 1 KB).
- `isDigit()`, `isAlpha()`, `toUpperCase()`… (`<ctype.h>` incluido, como en Arduino).
- Correcciones: ejemplos de LiquidCrystal que no compilaban en las placas de 32 patas,
  funciones que faltaban en LiquidCrystal_I2C/pcf2119, ejemplo HMC5883L actualizado,
  `EEPROM.length()` correcto en STM8S005/003, librerías marcadas para `stm8` (el IDE no
  las confunde con las de AVR que tenga instaladas).

## Instalación

### A) Gestor de placas (cuando esté publicada la release)

1. **Archivo → Preferencias → URLs adicionales del gestor de placas**:
   ```
   https://raw.githubusercontent.com/CEPH81/sduino/development/package_sduino_ceph81_index.json
   ```
2. **Herramientas → Placa → Gestor de placas**, busque **sduino** e instale
   *STM8S (sduino para Arduino IDE 2.x)*. Se descarga también el compilador SDCC.

### B) Manual (repositorio clonado)

En Windows, con [git](https://git-scm.com) instalado, en PowerShell:

```powershell
git clone -b development https://github.com/CEPH81/sduino.git $env:TEMP\sduino-inst
powershell -ExecutionPolicy Bypass -File $env:TEMP\sduino-inst\instalar_windows.ps1
```

El script clona el repositorio en `Documentos\Arduino\sduino-repo`, crea el enlace
`Documentos\Arduino\hardware\sduino` y descarga SDCC 3.9. Para actualizar más tarde:
`git pull` en `sduino-repo` (o volver a ejecutar el script).

A mano (Windows/Linux/macOS): la carpeta `sduino` del repositorio debe quedar en
`<sketchbook>/hardware/sduino` y SDCC 3.9 (compilación 11242 de
[sduino 0.5](https://github.com/tenbaht/sduino/releases/tag/v0.5)) descomprimido en
`<sketchbook>/hardware/sduino/tools/sdcc/3.9.1/` (con `bin/sdcc` dentro).

### Controlador del ST-Link en Windows

`stm8flash` usa libusb: instale el controlador **WinUSB** para el ST-Link con
[Zadig](https://zadig.akeo.ie) (*Options → List All Devices*, elija *STM32 STLink*,
controlador *WinUSB*, *Replace Driver*). En Linux, añada una regla udev para el ST-Link.

## Uso

1. **Herramientas → Placa → STM8S (sduino)** → *STM8S105K4T6 Breakout Board (STM8 Black)*
   o *STM8S005K6T6 (pinout STM8 Black)*.
2. **Reloj**: *Interno 16 MHz (HSI)* (por defecto), *Cristal externo 16 MHz (HSE)* o
   *Cristal externo 8 MHz (HSE)*. Si el cristal no arranca, el micro sigue con el oscilador
   interno ajustado a la misma frecuencia, así que `millis()`, `delay()` y el puerto serie
   siguen funcionando bien.
3. **Método de carga**: *ST-Link/V2 (stm8flash)* (por defecto, sin puerto) o
   *Serie (stm8gal, bootloader ROM)*.
4. **Optimización**: *Tamaño de código* (por defecto), *Tamaño mínimo* (compila más
   lento) o *Velocidad*.
5. Pulse **Subir**.

Conexión del ST-Link V2: **SWIM ↔ PD1**, **NRST ↔ NRST**, **GND ↔ GND**, **3,3 V ↔ 3V3**.

Si el micro está protegido contra lectura (algunas placas vienen así de fábrica):
**Herramientas → Grabar bootloader** lo desbloquea (borra la memoria y restaura los
option bytes).

### Pines de la STM8 Black y la STM8S005K6

| Pin | Puerto | Función | | Pin | Puerto | Función |
|---|---|---|---|---|---|---|
| 0 | PC3 | PWM | | 13 | PA1 | |
| 1 | PC4 | PWM | | 14 | PA2 | |
| 2 | PC5 | SPI SCK | | 15 | PF4 | |
| 3 | PC6 | SPI MOSI | | 16 | PB5 | **SDA**, A0 |
| 4 | PC7 | SPI MISO | | 17 | PB4 | **SCL**, A1 |
| 5 | PD0 | PWM | | 18 | PB3 | A2 |
| 6 | PD1 | SWIM (ST-Link) | | 19 | PB2 | A3 |
| 7 | PD2 | PWM | | 20 | PB1 | A4 |
| 8 | PD3 | PWM | | 21 | PB0 | A5 |
| 9 | PD4 | PWM | | 22 | PE5 | LED_BUILTIN, SPI SS |
| 10 | PD5 | **TX** (Serial) | | 23 | PC1 | PWM |
| 11 | PD6 | **RX** (Serial) | | 24 | PC2 | PWM |
| 12 | PD7 | | | | | |

También se pueden usar los nombres: `digitalWrite(PD3, HIGH)`.

**I²C**: las patas SDA/SCL del STM8S son de drenador abierto real y **no tienen pull-up
interna**: ponga resistencias de 4,7 kΩ a 3,3 V o 5 V.

## Escribir sketches

```cpp
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
}

void loop() {
  Wire.beginTransmission(0x27);
  Wire.write(0x55);
  byte err = Wire.endTransmission();   // 0 ok, 2 NACK direccion, 5 tiempo agotado...
  Serial.print("Resultado: ");
  Serial.println(err);
  delay(1000);
}
```

Reglas a tener en cuenta (SDCC solo compila C):

- `Serial`, `Wire`, `EEPROM` y `SPI` admiten la sintaxis con punto. Las librerías con
  objetos que se crean en el sketch (`LiquidCrystal`, `Servo`, `Stepper`…) usan la forma
  de sduino: `LiquidCrystal(lcd, …);` y luego `lcd_print_s("hola")`, `lcd_setCursor(0, 1)`.
  Ver los ejemplos de cada librería.
- No hay `String` ni clases propias en el sketch.
- Los ejemplos de **Archivo → Ejemplos** (sección de la placa) están adaptados.

## Depuración

El botón de depurar del IDE 2.x solo funciona con micros ARM Cortex-M: el STM8 no tiene
soporte ahí. Para depurar paso a paso un STM8 con ST-Link use
[PlatformIO](https://docs.platformio.org/en/latest/platforms/ststm8.html) (OpenOCD +
stm8-gdb) o la extensión [STM8 Debug para VS Code](https://github.com/github0null/stm8-debug).

## Pruebas

- `test/twi_host`: el controlador I²C se prueba en el PC contra un modelo del periférico
  I²C del STM8S (lecturas de 1/2/N bytes, NACK, bus bloqueado, 3000 operaciones al azar):
  `make -C test/twi_host`.
- Todos los ejemplos (71) compilan para la STM8 Black, la STM8S005K6 y la STM8S003F3.

## Créditos y licencia

Basado en [sduino](https://github.com/tenbaht/sduino) de Michael Mayer (LGPL 2.1) y en el
núcleo de Arduino. Librería estándar de periféricos (SPL) de STMicroelectronics.
Adaptación al IDE 2.x, STM8S005K6 e I²C nuevo: CEPH81.
La documentación original (en inglés) sigue en `docs/`.
