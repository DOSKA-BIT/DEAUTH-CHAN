#ifndef BOARD_3248S035C_H
#define BOARD_3248S035C_H

// ESP32-3248S035C: misma placa fisica que la 3248S035R pero con touch
// capacitivo GT911 por I2C en vez de resistivo XPT2046.
//
// Aviso de fabrica: en esta placa el pin INT del GT911 esta mal
// ruteado (va a GND en vez de pasar por el jumper R25 hacia el GPIO
// de interrupcion), asi que el touch funciona por polling y no por
// interrupcion salvo que le hagas la modificacion de hardware que
// documenta la comunidad (cortar la pista, puentear R25, sacar R18).
// El codigo ya asume polling, no hace falta tocar nada para que ande,
// simplemente vas a tener algo mas de uso de CPU que si la
// interrupcion funcionara.

#define BOARD_NAME "ESP32-3248S035C"

#define TFT_PANEL_WIDTH   320
#define TFT_PANEL_HEIGHT  480

#define TOUCH_IS_CAPACITIVE  1
#define TOUCH_SDA_PIN   33
#define TOUCH_SCL_PIN   32
#define TOUCH_INT_PIN   21
#define TOUCH_RST_PIN   25

#define TFT_BL_PIN   27

#define SD_CS_PIN    5

#define GPS_RX_PIN   35
#define GPS_TX_PIN   22

#endif
