#ifndef BOARD_2432S024_H
#define BOARD_2432S024_H

// ESP32-2432S024 (2.4" resistivo). Misma familia de referencia de
// Sunton que la 2432S028: mismo controlador ILI9341, misma resolucion
// logica 240x320 (la pantalla es fisicamente mas chica pero tiene la
// misma cantidad de pixeles), y el mismo bus dedicado para el touch.
//
// OJO: a diferencia de la 2432S028 (donde el pinout esta muy
// verificado por la comunidad), en la 2432S024 hay reportes cruzados
// de usuarios con el backlight en GPIO21 y otros en GPIO27 segun la
// revision de la placa. Dejo GPIO21 por ser el mas repetido, pero si
// al flashear la pantalla queda sin luz de fondo, ese es el primer
// sospechoso: probá GPIO27 antes de asumir que algo mas esta roto.

#define BOARD_NAME "ESP32-2432S024"

#define TFT_PANEL_WIDTH   240
#define TFT_PANEL_HEIGHT  320

#define TOUCH_IS_CAPACITIVE   0
#define TOUCH_SHARES_TFT_BUS  0
#define TOUCH_CS_PIN    33
#define TOUCH_IRQ_PIN   36
#define TOUCH_MOSI_PIN  32
#define TOUCH_MISO_PIN  39
#define TOUCH_SCLK_PIN  25

#define TFT_BL_PIN   21

#define SD_CS_PIN    5

#define GPS_RX_PIN   17
#define GPS_TX_PIN   16

#endif
