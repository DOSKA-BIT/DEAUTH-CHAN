#ifndef BOARD_2432S028_H
#define BOARD_2432S028_H

// ESP32-2432S028R / 2432S028 - la "CYD" original, la que todo el mundo
// tiene tirada en el cajon. Pantalla ILI9341 de 2.8", 240x320, tactil
// resistivo XPT2046 en un SPI aparte del de la pantalla.
//
// El wiring de la pantalla en si (MOSI/MISO/SCLK/CS/DC) esta en
// lib/CustomTFTConfig/User_Setup.h porque TFT_eSPI lo pide como defines
// de compilacion, no lo puede leer de un header normal en runtime.

#define BOARD_NAME "ESP32-2432S028"

#define TFT_PANEL_WIDTH   240
#define TFT_PANEL_HEIGHT  320

// El touch NO comparte bus con la pantalla en esta placa: el XPT2046
// esta cableado a su propio VSPI con estos pines fijos (confirmado
// contra el pinout de referencia de la comunidad CYD). TFT_eSPI usa
// HSPI (12/13/14, ver User_Setup.h) para la pantalla, asi que ambos
// buses son independientes entre si.
#define TOUCH_IS_CAPACITIVE   0
#define TOUCH_SHARES_TFT_BUS  0
#define TOUCH_CS_PIN    33
#define TOUCH_IRQ_PIN   36
#define TOUCH_MOSI_PIN  32
#define TOUCH_MISO_PIN  39
#define TOUCH_SCLK_PIN  25

#define TFT_BL_PIN   21

// La SD usa los pines VSPI "de fabrica" (19/23/18), que son los mismos
// que ocupa el touch de arriba pero remapeados. Como el ESP32 clasico
// solo tiene dos perifericos SPI de uso general y la pantalla ya se
// queda con uno (HSPI), touch y SD comparten el otro (VSPI) por
// necesidad de hardware, no por un descuido del cableado. Por eso
// Learning/PCAPWriter/Wardriving llaman SD.begin(SD_CS_PIN) recien al
// momento de escribir: eso reclama el mapeo de pines para la SD justo
// antes de usarla. Si notas que el touch se queda "pegado" justo
// despues de guardar en la SD, es este reparto de bus, no un bug del
// codigo - la solucion de la comunidad es la misma que uso aca.
#define SD_CS_PIN    5

// El NEO-6M no viene soldado en la placa, va por los pines libres del
// header P3 (CN1). Si tenes otro conexionado cambialo aca, no hace
// falta tocar GPSModule.cpp.
#define GPS_RX_PIN   17
#define GPS_TX_PIN   16

#endif
