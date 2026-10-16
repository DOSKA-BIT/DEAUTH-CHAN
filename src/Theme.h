#ifndef THEME_H
#define THEME_H

#include <Arduino.h>

// Paleta de colores del proyecto, todo en un solo lado para no tener
// que ir a cambiar un TFT_YELLOW suelto en cinco archivos distintos
// el dia que alguien (yo, probablemente) quiera retocar el look.
// Los valores estan en RGB565 (lo que pide TFT_eSPI), calculados a
// partir de un rgb(r,g,b) normal de 0 a 255 - si queres probar otro
// color, buscate una calculadora de RGB565 en internet y pegalo aca.

#define COLOR_BG            0x0883   // fondo general, un gris azulado bien oscuro (no negro puro, cansa menos)
#define COLOR_PANEL         0x1905   // paneles y barras, un escalon mas claro que el fondo
#define COLOR_ACCENT        0x0655   // verde azulado, el color "vivo" del proyecto
#define COLOR_ACCENT_DIM    0x036B   // mismo verde pero apagado, para lo que esta inactivo
#define COLOR_WARN          0xFD87   // ambar, redes abiertas y avisos
#define COLOR_DANGER        0xFACB   // rojo suave, errores (SD no detectada y esas cosas)
#define COLOR_TEXT          0xE73D   // texto principal, blanco roto en vez de blanco puro
#define COLOR_TEXT_DIM      0x8452   // texto secundario / placeholders
#define COLOR_SLEEP         0x10C6   // fondo de la cara cuando esta durmiendo
#define COLOR_SLEEP_BORDE   0x3A91   // borde de esa misma cara

#endif
