#ifndef IDIOMA_H
#define IDIOMA_H

// Sistema de traducciones del firmware: Fase 1 (infraestructura +
// chrome compartido del menu - ver Menu.cpp y el array herramientas[]/
// categorias[] de main.cpp). El resto de las pantallas (ScreenWifi.cpp,
// ScreenWardriving.cpp, ScreenRadar.cpp, ScreenAjustes.cpp, Mascota.cpp)
// todavia tienen sus textos escritos a mano en español - se van a ir
// pasando a este mismo sistema en fases siguientes, pantalla por
// pantalla.
//
// La idea: cada texto de la interfaz tiene una "clave" (un valor de
// TextoId, de abajo) en vez de escribirse como string literal en el
// medio del codigo de dibujado. Dos tablas (ES[] y EN[] en Idioma.cpp)
// mapean cada clave a su traduccion real, y tr(clave) devuelve la que
// corresponda segun el idioma activo (guardado en Settings, ver
// ajustes.getIdioma()). Agregar un idioma nuevo el dia de mañana es
// agregar UNA tabla nueva en Idioma.cpp (y un nombre a IdiomaId) - no
// hay que tocar Menu.cpp, main.cpp ni ningun Screen*.cpp para eso,
// que es exactamente lo que no pasaba cuando los textos estaban
// sueltos adentro de cada tft.print().
//
// Lo que SI hizo falta (y no hay forma de evitar, es inherente a
// armar este sistema): la primera vez que un texto se convierte de
// string literal a clave, hay que tocar el archivo donde vivia ese
// texto. Esa conversion se hace una sola vez por texto, nunca mas.

// IdiomaId: no es un enum de "layout" como MenuScreen, es la lista de
// idiomas soportados. CANTIDAD_IDIOMAS al final (mismo patron que
// CANTIDAD_CATEGORIAS, ESTADO_COUNT, etc en el resto del firmware)
// sirve para loopear sobre todos los idiomas sin tener que hardcodear
// "2" en ningun lado (ver dibujarPantallaIdioma() en ScreenAjustes.cpp).
enum IdiomaId {
    IDIOMA_ES = 0,
    IDIOMA_EN,
    CANTIDAD_IDIOMAS
};

// Todas las claves de texto traducible de la Fase 1. El orden ACA
// tiene que ser exactamente el mismo orden que usan las tablas ES[] y
// EN[] en Idioma.cpp - cada clave es un indice dentro de esas tablas,
// no un nombre que se busque en tiempo de ejecucion. Si se agrega una
// clave nueva, se agrega en los 3 lugares (aca + ES[] + EN[]) en la
// misma posicion relativa, o las traducciones de TODO lo que viene
// despues quedan corridas.
enum TextoId {
    // Barra de botones de abajo (Menu.cpp)
    TXT_BOTON_MASCOTA,
    TXT_BOTON_HERRAMIENTAS,
    TXT_BOTON_ATRAS,

    // Chrome del menu de 2 niveles (Menu.cpp)
    TXT_TITULO_HERRAMIENTAS,
    TXT_PIE_TOCA_CATEGORIA,
    TXT_PIE_TOCA_HERRAMIENTA,

    // Categorias (main.cpp: categorias[])
    TXT_CAT_WIFI_NOMBRE,
    TXT_CAT_WIFI_DESC,
    TXT_CAT_WARDRIVING_NOMBRE,
    TXT_CAT_WARDRIVING_DESC,
    TXT_CAT_RADAR_NOMBRE,
    TXT_CAT_RADAR_DESC,
    TXT_CAT_AJUSTES_NOMBRE,
    TXT_CAT_AJUSTES_DESC,

    // Herramientas (main.cpp: herramientas[])
    TXT_HERR_ESCANEAR_NOMBRE,
    TXT_HERR_ESCANEAR_DESC,
    TXT_HERR_STATS_NOMBRE,
    TXT_HERR_STATS_DESC,
    TXT_HERR_CONOCIDAS_NOMBRE,
    TXT_HERR_CONOCIDAS_DESC,
    TXT_HERR_MONITOR_NOMBRE,
    TXT_HERR_MONITOR_DESC,
    TXT_HERR_EVENTOS_NOMBRE,
    TXT_HERR_EVENTOS_DESC,
    TXT_HERR_CANALES_NOMBRE,
    TXT_HERR_CANALES_DESC,
    TXT_HERR_ANALIZADOR_NOMBRE,
    TXT_HERR_ANALIZADOR_DESC,
    TXT_HERR_DUPLICADO_NOMBRE,
    TXT_HERR_DUPLICADO_DESC,
    TXT_HERR_GAME_NOMBRE,
    TXT_HERR_GAME_DESC,
    TXT_HERR_WARDRIVE_NOMBRE,
    TXT_HERR_WARDRIVE_DESC,
    TXT_HERR_RADAR_NOMBRE,
    TXT_HERR_RADAR_DESC,
    TXT_HERR_CONFIG_NOMBRE,
    TXT_HERR_CONFIG_DESC,
    TXT_HERR_IDIOMA_NOMBRE,
    TXT_HERR_IDIOMA_DESC,

    // Pantalla IDIOMA (ScreenAjustes.cpp) - los nombres de cada idioma
    // ("ESPANOL"/"ENGLISH") NO son claves de esta lista a proposito:
    // un selector de idioma siempre muestra cada opcion en su propio
    // idioma, nunca traducida, para que el usuario la reconozca aunque
    // el idioma activo hoy no sea el suyo.
    TXT_IDIOMA_TITULO,
    TXT_IDIOMA_PIE,

    CANTIDAD_TEXTOS
};

// Devuelve el texto de "id" en el idioma activo (ajustes.getIdioma()).
// Si "id" fuera invalido (no deberia pasar nunca, todas las claves
// salen del enum de arriba) devuelve "" en vez de leer fuera de la
// tabla.
const char* tr(TextoId id);

#endif
