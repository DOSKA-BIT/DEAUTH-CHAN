#ifndef THEME_H
#define THEME_H

#include <Arduino.h>

// Paleta de colores del proyecto, todo en un solo lado para no tener
// que ir a cambiar un TFT_YELLOW suelto en cinco archivos distintos
// el dia que alguien (yo, probablemente) quiera retocar el look.
// Los valores estan en RGB565 (lo que pide TFT_eSPI), calculados a
// partir de un rgb(r,g,b) normal de 0 a 255.

// Rediseno "fase 3": estetica tipo Fennec (cifertech/fennec) -
// monitor CRT monocromatico ambar, que el usuario pidio copiar tal
// cual ("quiero copiar de punta a punta ... la estetica de dibujo y
// el estilo de los colores los trazados y demas"). Solo 5 tonos de un
// mismo color base, nada de colores por categoria como en la fase
// anterior - la diferenciacion visual ahora es por BRILLO (que tan
// claro es cada tono), no por matiz, calcado del esquema bg/sand/
// line/muted/dim de Fennec/DisplayUI.cpp.
#define COLOR_BG            0x0841   // bg:    fondo, casi negro
#define COLOR_SAND          0xE651   // sand:  el tono mas claro - texto principal, acento activo
#define COLOR_LINE          0xCD2D   // line:  segundo mas claro - bordes/acento secundario
#define COLOR_MUTED         0x8BCA   // muted: texto/elementos secundarios
#define COLOR_DIM           0x3984   // dim:   el mas oscuro de los 4 "visibles" - tracks, paneles apagados

// --- Mapeo de los nombres que ya usaba el resto del firmware, para
// no tener que tocar Menu.cpp/Mascota.cpp/Screen*.cpp mas de lo
// necesario. Ya no hay COLOR_HEADER como bloque de color aparte: en
// Fennec el encabezado no es una barra de otro color, es el mismo
// fondo con texto y una linea fina separandolo (ver Mascota::
// dibujarEncabezado()).
#define COLOR_HEADER        COLOR_BG
#define COLOR_PANEL         COLOR_DIM    // tracks de barras, fondos de paneles apagados
#define COLOR_TEXT          COLOR_SAND   // texto principal
#define COLOR_TEXT_DIM      COLOR_MUTED  // texto secundario / placeholders

// Acento general (seleccion, lineas separadoras activas, texto
// resaltado). COLOR_ACCENT_DIM es el acento "visible pero no
// protagonista" - lo que antes era un cian apagado ahora es el
// segundo tono de la escala ambar.
#define COLOR_ACCENT        COLOR_SAND
#define COLOR_ACCENT_DIM    COLOR_LINE

// Categorias: colapsadas a proposito en un solo tono (eleccion
// explicita del usuario: "Monocromatica ambar, como Fennec" - se
// pierde el color por categoria a cambio de que el look sea identico
// a la referencia). El icono/texto de cada herramienta ya no dice de
// un vistazo "esto es wardriving" vs "esto es radar" por el color -
// lo sigue diciendo el nombre y el icono en si.
#define COLOR_CAT_CYAN      COLOR_ACCENT
#define COLOR_CAT_VIOLET    COLOR_ACCENT
#define COLOR_CAT_YELLOW    COLOR_ACCENT
#define COLOR_CAT_BLUE      COLOR_ACCENT
#define COLOR_CAT_WHITE     COLOR_TEXT
#define COLOR_CAT_GREEN     COLOR_ACCENT

// COLOR_WARN SI se mantiene dentro de la familia ambar (es el mismo
// tono que el acento, el mas claro de la escala) - una fila en WARN
// ya se distingue por brillo contra el resto, calcado de como Fennec
// usa "sand vs muted/dim" para marcar lo que esta activo/presente.
#define COLOR_WARN           COLOR_SAND

// COLOR_DANGER es la UNICA excepcion a la paleta monocroma, y es
// deliberada: el Monitor pasivo usa este color para marcar paquetes
// deauth/disassoc reales y SSIDs duplicados (posible gemelo malvado)
// - señales de seguridad, no decoracion. Fennec no tiene un caso
// parecido (es un receptor de radio, no detecta ataques), asi que acá
// no hay una referencia que copiar; mantener un rojo bien distinto
// del ambar es para que esa alerta siga notandose a simple vista en
// vez de mezclarse con el resto de la interfaz. Si se prefiere 100%
// monocromatico avisar y se reemplaza por COLOR_SAND tambien.
#define COLOR_DANGER         0xFA25   // rojo-naranja calido, bien distinto del ambar

#endif
