#ifndef THEME_H
#define THEME_H

#include <Arduino.h>

// Paleta de colores del proyecto, todo en un solo lado para no tener
// que ir a cambiar un TFT_YELLOW suelto en cinco archivos distintos
// el dia que alguien (yo, probablemente) quiera retocar el look.
// Los valores estan en RGB565 (lo que pide TFT_eSPI), calculados a
// partir de un rgb(r,g,b) normal de 0 a 255 - si queres probar otro
// color, buscate una calculadora de RGB565 en internet y pegalo aca.

// Rediseno "herramienta de ciberseguridad retro" (fase 1: paleta),
// calcado de la referencia que mando el usuario: fondo azul marino
// casi negro, encabezado azul intenso, y colores de categoria
// (cian/violeta/rojo/amarillo) para distinguir de un vistazo que tipo
// de herramienta es cada cosa - a diferencia de la paleta monocroma
// anterior, aca el color SI es informacion (rojo = alerta, amarillo =
// stats/destacado, etc), no solo decoracion.
#define COLOR_BG            0x0843   // fondo: azul marino casi negro
#define COLOR_HEADER        0x1193   // encabezado superior: azul intenso bien visible
#define COLOR_PANEL         0x0886   // paneles/bordes: un paso mas claro que el fondo
#define COLOR_TEXT          0xEF3B   // texto principal: blanco calido
#define COLOR_TEXT_DIM      0x7C35   // texto secundario / placeholders: gris azulado

// Acento general (seleccion, lineas separadoras, lo que antes usaba
// el unico celeste de la paleta monocroma). Coincide a proposito con
// COLOR_CAT_CYAN de aca abajo: las herramientas de red SON el acento
// "por defecto" del firmware.
#define COLOR_ACCENT        0x3F1D   // cian brillante
#define COLOR_ACCENT_DIM     0x2AD0   // mismo cian, apagado

// Colores de categoria: cada herramienta se dibuja con el color de su
// categoria (icono + texto), asi se distingue el tipo de funcion de
// un vistazo sin tener que leer el nombre completo. Ver la asignacion
// herramienta-por-herramienta en main.cpp (el array herramientas[],
// campo "color" de cada una).
#define COLOR_CAT_CYAN      0x3F1D   // redes / escaneo / descubrimiento
#define COLOR_CAT_VIOLET    0x92FD   // analisis / wardriving / datos de ubicacion
#define COLOR_CAT_RED       0xE9E9   // alertas / monitor de deauth-disassoc
#define COLOR_CAT_YELLOW    0xEE27   // estadisticas / acciones destacadas
#define COLOR_CAT_BLUE      0x547F   // configuracion / ajustes, categoria neutra
#define COLOR_CAT_WHITE     COLOR_TEXT // listas simples / informativas (conocidas, eventos)
#define COLOR_CAT_GREEN     0x56CF   // radar / deteccion de dispositivos BLE cercanos

// Avisos y errores: mantienen su propio matiz (ya no comparten color
// con el acento general como en la paleta monocroma), para que un
// error realmente se note distinto de una fila normal.
#define COLOR_WARN          COLOR_CAT_YELLOW
#define COLOR_DANGER         COLOR_CAT_RED

#endif
