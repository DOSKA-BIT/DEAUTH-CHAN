// Configuración personalizada para el Sparkle IoT XH-32S

// Controlador de la pantalla
#define ILI9341_DRIVER

// Definición de los pines
#define TFT_CS   15  // Chip select
#define TFT_DC   2   // Data/Command
#define TFT_RST  -1  // Reset (no conectado, se maneja por software)

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14

// Configuración del touch (XPT2046)
#define TOUCH_CS 33

// Configuración general
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Carga de fuentes
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_GFXFF
#define SMOOTH_FONT
