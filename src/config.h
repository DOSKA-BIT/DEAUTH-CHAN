#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define TFT_LED_PIN 21
#define TOUCH_CS 14
#define TOUCH_IRQ 13
#define SD_CS 5

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define ANIM_FRAME_MS 200
#define SCAN_INTERVAL 5000

enum EstadoMascota {
    ESTADO_IDLE = 0,
    ESTADO_SCANNING,
    ESTADO_HAPPY,
    ESTADO_ATTACK,
    ESTADO_SLEEP,
    ESTADO_COUNT
};

struct RedInfo {
    char ssid[33];
    uint8_t bssid[6];
    int rssi;
    int canal;
    bool tieneClave;
    bool handshakeCapturado;
};

#endif
