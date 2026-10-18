#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "boards/BoardConfig.h"

#define ANIM_FRAME_MS   200
#define SCAN_INTERVAL   5000

// Si pasan 60 segundos sin ningun toque en la pantalla (y no hay un
// escaneo en curso), la mascota se duerme y se apaga el backlight.
// Cualquier toque la despierta. Es la unica parte de la interfaz que
// de verdad ahorra bateria -- todo lo demas sigue prendido igual,
// pero el backlight de estas pantallas es lo que mas consume.
#define SLEEP_TIMEOUT_MS  60000UL

// Medidas del "marco" de la interfaz: la franja de arriba (donde va
// el estado de la mascota) y la barra de botones de abajo. Todo lo
// demas - las listas, los stats, etc - usa el espacio que queda en
// el medio. Estan fijas en un solo lugar porque las necesitan
// tanto main.cpp (para saber donde arrancar a dibujar cada pantalla)
// como Mascota.cpp (para saber cuanto espacio tiene para su cara).
#define CONTENT_TOP      36
#define MENU_BAR_HEIGHT  40

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
