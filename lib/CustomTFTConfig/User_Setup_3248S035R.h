// Wiring de pantalla para la ESP32-3248S035R (3.5", ST7796, touch
// resistivo compartiendo bus con la pantalla). TFT_eSPI necesita esto
// como defines de compilacion.
//
// A diferencia de la 2432S028, aca SI activamos el soporte de touch
// integrado de TFT_eSPI (TOUCH_CS) en vez de instanciar
// XPT2046_Touchscreen aparte, porque el touch y la pantalla comparten
// el mismo bus fisico en esta placa. TOUCH_CS tiene que coincidir con
// TOUCH_CS_PIN de src/boards/Board_3248S035R.h.

#define ST7796_DRIVER

#define TFT_WIDTH   320
#define TFT_HEIGHT  480

#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14

#define TOUCH_CS 33

#define SPI_FREQUENCY        55000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY  2500000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_GFXFF
