#ifndef BOARD_3248S035R_H
#define BOARD_3248S035R_H

// ESP32-3248S035R: 3.5" ST7796, 320x480, touch resistivo XPT2046.
// A diferencia de la 2432S028, el touch de esta placa SI comparte el
// mismo bus SPI que la pantalla (mismos MISO/MOSI/SCLK que el TFT,
// nada mas cambia el CS). Por eso main.cpp usa TouchResistiveSharedBus
// en esta placa en vez de TouchResistive.

#define BOARD_NAME "ESP32-3248S035R"

#define TFT_PANEL_WIDTH   320
#define TFT_PANEL_HEIGHT  480

#define TOUCH_IS_CAPACITIVE   0
#define TOUCH_SHARES_TFT_BUS  1
// TOUCH_CS_PIN se usa tambien desde el User_Setup.h de esta placa
// (como TOUCH_CS de TFT_eSPI), tienen que coincidir siempre.
#define TOUCH_CS_PIN    33

#define TFT_BL_PIN   27

#define SD_CS_PIN    5

// Header P3 de esta placa: GND, GPIO35, GPIO22, GPIO21. Igual que en
// la 2432S028, uso el par disponible para el NEO-6M.
#define GPS_RX_PIN   35
#define GPS_TX_PIN   22

#endif
