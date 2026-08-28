// Wiring de pantalla para la ESP32-3248S035C (3.5", ST7796, touch
// capacitivo GT911 por I2C). El touch va completamente aparte de
// TFT_eSPI (ver src/TouchCapacitive.h), asi que este archivo no
// define nada de touch, solo la pantalla.

#define ST7796_DRIVER

#define TFT_WIDTH   320
#define TFT_HEIGHT  480

#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14

#define SPI_FREQUENCY        55000000
#define SPI_READ_FREQUENCY   20000000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_GFXFF
