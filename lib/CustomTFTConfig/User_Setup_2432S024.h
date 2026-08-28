// Wiring de pantalla para la ESP32-2432S024. TFT_eSPI necesita esto
// como defines de compilacion (no lo puede resolver en runtime), por
// eso vive separado de src/boards/Board_2432S024.h.
//
// Comparte el mismo pinout que la 2432S028 (misma familia de
// referencia de Sunton), ver el aviso sobre GPIO21 vs GPIO27 de
// backlight en Board_2432S024.h si la luz de fondo no prende.

#define ILI9341_DRIVER

// Fuerza la pantalla al periferico HSPI en vez del VSPI que usa
// TFT_eSPI por defecto. Sin esto, TFT_eSPI y cualquier otro
// dispositivo SPI (touch dedicado, SD) terminan peleando por el mismo
// periferico VSPI aunque esten mapeados a pines fisicos distintos,
// porque ambos son en el fondo el mismo controlador de hardware.
#define USE_HSPI_PORT

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
