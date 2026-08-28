// Wiring de pantalla para la ESP32-2432S028. TFT_eSPI necesita esto
// como defines de compilacion (no lo puede resolver en runtime), por
// eso vive separado de src/boards/Board_2432S028.h.
//
// Si en el futuro agregamos una placa con otro driver (ST7796,
// GC9A01) va a necesitar su propio User_Setup, seleccionado desde el
// build_flags del environment correspondiente en platformio.ini.

#define ILI9341_DRIVER

#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14

#define SPI_FREQUENCY        40000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY  2500000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_GFXFF
