#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "boards/BoardConfig.h"

#define ANIM_FRAME_MS   200
#define SCAN_INTERVAL   5000

// Cuantas redes distintas podemos tener en memoria en un mismo escaneo.
// El ESP32 normalmente devuelve entre 10 y 40 en un entorno urbano, con
// 20 alcanza para no comerse RAM de mas en placas con poco heap libre.
#define MAX_REDES_ESCANEO  20

enum EstadoMascota {
    ESTADO_IDLE = 0,
    ESTADO_SCANNING,
    ESTADO_HAPPY,
    ESTADO_CURIOSA,   // reemplaza al viejo estado "attack": red nueva o poco vista
    ESTADO_SLEEP,
    ESTADO_COUNT
};

struct RedInfo {
    char ssid[33];
    uint8_t bssid[6];
    int rssi;
    int canal;
    bool tieneClave;
};

#endif
