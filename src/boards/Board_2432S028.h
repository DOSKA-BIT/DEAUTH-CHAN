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

#define TOUCH_IS_CAPACITIVE 0
#define TOUCH_CS_PIN   33
#define TOUCH_IRQ_PIN  36

#define TFT_BL_PIN   21

#define SD_CS_PIN    5

// El NEO-6M no viene soldado en la placa, va por los pines libres del
// header P3 (CN1). Si tenes otro conexionado cambialo aca, no hace
// falta tocar GPSModule.cpp.
#define GPS_RX_PIN   17
#define GPS_TX_PIN   16

#endif
