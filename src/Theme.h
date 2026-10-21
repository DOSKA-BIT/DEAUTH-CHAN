#ifndef THEME_H
#define THEME_H

#include <Arduino.h>

// Paleta de colores del proyecto, todo en un solo lado para no tener
// que ir a cambiar un TFT_YELLOW suelto en cinco archivos distintos
// el dia que alguien (yo, probablemente) quiera retocar el look.
// Los valores estan en RGB565 (lo que pide TFT_eSPI), calculados a
// partir de un rgb(r,g,b) normal de 0 a 255 - si queres probar otro
// color, buscate una calculadora de RGB565 en internet y pegalo aca.

// Paleta monocromatica (estilo terminal retro / Pwnagotchi-Flipper):
// negro puro de fondo y un solo tono celeste para todo lo demas, en
// distintas intensidades. No hay un color separado para avisos o
// errores a proposito - en monocromo esas diferencias se marcan con
// texto invertido (fondo celeste, letra negra) en vez de otro matiz,
// que es como lo resuelven las pantallas monocromaticas de verdad. Los
// nombres de abajo se mantienen igual que antes (COLOR_WARN,
// COLOR_DANGER, etc) para no tener que tocar cada archivo que ya los
// usa - achicamos la paleta aca nomas.
#define COLOR_BG            0x0000   // fondo: negro puro
#define COLOR_PANEL         0x0842   // paneles/barras: casi negro, solo para separar del fondo
#define COLOR_ACCENT        0x9F5F   // celeste brillante: el unico color "vivo" del firmware
#define COLOR_ACCENT_DIM    0x3AED   // mismo celeste, apagado (lo inactivo, lo secundario)
#define COLOR_WARN          COLOR_ACCENT      // sin matiz propio: un aviso se marca invirtiendo texto/fondo, no con otro color
#define COLOR_DANGER        COLOR_ACCENT      // idem para errores (SD no detectada, etc)
#define COLOR_TEXT          0xC7BF   // texto principal: blanco con tinte celeste
#define COLOR_TEXT_DIM      0x4B2D   // texto secundario / placeholders
#define COLOR_SLEEP         0x1167   // fondo de la cara cuando esta durmiendo
#define COLOR_SLEEP_BORDE   COLOR_ACCENT_DIM

#endif
