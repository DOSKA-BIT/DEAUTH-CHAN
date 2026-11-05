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
    TXT_HERR_RASTREADOR_NOMBRE,
    TXT_HERR_RASTREADOR_DESC,
    TXT_HERR_CONFIG_NOMBRE,
    TXT_HERR_CONFIG_DESC,
    TXT_HERR_IDIOMA_NOMBRE,
    TXT_HERR_IDIOMA_DESC,

    // Pantalla RADAR (ScreenRadar.cpp) - Fase 2
    TXT_RADAR_PIE_BUSCANDO,
    TXT_RADAR_PIE_FINDMY,
    TXT_RADAR_PIE_CERCA,

    // Pantalla RASTREADOR (ScreenRadar.cpp) - Fase 2
    TXT_RASTREADOR_PIE_BUSCANDO,
    TXT_RASTREADOR_PIE_LISTA,
    TXT_RASTREADOR_PERDIDO,
    TXT_RASTREADOR_PIE_VOLVER,
    TXT_RASTREADOR_SIN_SENIAL,
    TXT_RASTREADOR_ACERCANDOSE,
    TXT_RASTREADOR_ALEJANDOSE,
    TXT_RASTREADOR_SIN_CAMBIOS,

    // Pantalla CONFIG (ScreenAjustes.cpp) - Fase 2
    TXT_CONFIG_BRILLO,
    TXT_CONFIG_AUTOSLEEP,
    TXT_CONFIG_AUTOSLEEP_NUNCA,
    TXT_CONFIG_BORRAR_ARMAR,
    TXT_CONFIG_BORRAR_HECHO,
    TXT_CONFIG_BORRAR_NORMAL,
    TXT_CONFIG_PLACA,
    TXT_CONFIG_PANT,
    TXT_CONFIG_TOUCH,
    TXT_CONFIG_CAPACITIVO,
    TXT_CONFIG_RESISTIVO,
    TXT_CONFIG_SD,
    TXT_CONFIG_SD_OK,
    TXT_CONFIG_SD_NO_DETECTADA,
    TXT_CONFIG_PIE,
    TXT_CONFIG_HEAP,
    TXT_CONFIG_HEAP_SUFIJO,
    TXT_CONFIG_UPTIME,
    TXT_CONFIG_CPU,

    // Pantalla WARDRIVE (ScreenWardriving.cpp) - Fase 2
    TXT_WARDRIVE_GPS,
    TXT_WARDRIVE_FIX,
    TXT_WARDRIVE_SIN_FIX,
    TXT_WARDRIVE_SATELITES,
    TXT_WARDRIVE_REDES_GUARDADAS,
    TXT_WARDRIVE_FILTRO,
    TXT_WARDRIVE_FILTRO_TODAS,
    TXT_WARDRIVE_FILTRO_ABIERTAS,
    TXT_WARDRIVE_FILTRO_CLAVE,
    TXT_WARDRIVE_EXPORTAR,
    TXT_WARDRIVE_EXPORT_NADA,
    TXT_WARDRIVE_EXPORT_ERROR,
    TXT_WARDRIVE_PUNTOS,
    TXT_WARDRIVE_PIE,

    // Pantalla ESCANEAR (ScreenWifi.cpp) - Fase 2
    TXT_SCAN_ESCANEANDO,
    TXT_SCAN_PIE_ESPERANDO,
    TXT_SCAN_ESCUCHANDO_CLIENTES,
    TXT_SCAN_S_RESTANTES,
    TXT_SCAN_PIE_PASIVO_CLIENTES,
    TXT_SCAN_CLIENTES_DE,
    TXT_SCAN_NADIE_HABLANDO,
    TXT_SCAN_MAS_PREFIJO,
    TXT_SCAN_MAS_SUFIJO,
    TXT_SCAN_PIE_VOLVER,
    TXT_SCAN_TOCA_ESCANEAR,
    TXT_SCAN_PIE_PASIVO_VACIO,
    TXT_SCAN_PIE_TOCA_RED,

    // Compartida entre varias pantallas de WIFI (red/SSID sin nombre)
    TXT_OCULTA,

    // Pantalla STATS (ScreenWifi.cpp) - Fase 2
    TXT_STATS_REDES_CONOCIDAS,
    TXT_STATS_VISTAS_ESCANEO,
    TXT_STATS_NUEVAS_DESCUBIERTAS,
    TXT_STATS_ULTIMA_RED_TOP,
    TXT_STATS_VISTA_PREFIJO,
    TXT_STATS_VISTA_SUFIJO,
    TXT_STATS_PIE,

    // Criterios de orden de CONOCIDAS (nombreOrdenConocidas(), ScreenWifi.cpp)
    TXT_ORDEN_SENIAL,
    TXT_ORDEN_VISTAS,
    TXT_ORDEN_NOMBRE,

    // Pantalla CONOCIDAS (ScreenWifi.cpp) - Fase 2
    TXT_CONOCIDAS_ORDEN,
    TXT_CONOCIDAS_TOCAR_CAMBIAR,
    TXT_CONOCIDAS_VACIO,
    TXT_CONOCIDAS_PIE_VACIO,
    TXT_CONOCIDAS_MAS_FORMAT,
    TXT_CONOCIDAS_PIE_CAMBIAR,

    // Pantalla EVENTOS (ScreenWifi.cpp) - Fase 2. El contenido de cada
    // evento guardado (eventos.registrar()) NO pasa por aca - queda
    // igual que estaba, mismo criterio que el resto de los mensajes
    // que se escriben en la SD (ver el comentario largo en
    // ScreenWardriving.cpp sobre nombres de archivo).
    TXT_EVENTOS_EXPORTAR,
    TXT_EVENTOS_EXPORT_ERROR,
    TXT_EVENTOS_LINEAS,
    TXT_EVENTOS_VACIO,
    TXT_EVENTOS_PIE_VACIO,
    TXT_EVENTOS_MAS_FORMAT,
    TXT_EVENTOS_PIE_RECIENTE,

    // Pantalla MONITOR (ScreenWifi.cpp) - Fase 2
    TXT_MONITOR_DEAUTH,
    TXT_MONITOR_DISASSOC,
    TXT_MONITOR_PCAP,
    TXT_MONITOR_PCAP_ON,
    TXT_MONITOR_PCAP_OFF,
    TXT_MONITOR_PAQUETES_GUARDADOS,
    TXT_MONITOR_VACIO,
    TXT_MONITOR_PIE_PASIVO,
    TXT_MONITOR_DE,
    TXT_MONITOR_MAS_FORMAT,

    // Compartida entre CANALES y DUPLICADO (las dos necesitan un
    // escaneo previo para mostrar algo)
    TXT_NECESITA_ESCANEO,
    TXT_PIE_ANDA_ESCANEAR,

    // Pantalla CANALES (ScreenWifi.cpp) - Fase 2
    TXT_CANALES_HISTORICO_FORMAT,
    TXT_CANALES_PIE,

    // Pantalla ANALIZADOR (ScreenWifi.cpp) - Fase 2
    TXT_ANALIZADOR_PIE,

    // Pantalla DUPLICADO (ScreenWifi.cpp) - Fase 2
    TXT_DUPLICADO_NINGUNO,
    TXT_DUPLICADO_PIE_NINGUNO,
    TXT_DUPLICADO_MAS_FORMAT,
    TXT_DUPLICADO_PIE_MESH,

    // Pantalla GAME (ScreenWifi.cpp) - Fase 2
    TXT_GAME_PROXIMAMENTE,
    TXT_GAME_PIE,

    // Mascota.cpp - Fase 2 (ultima pantalla pendiente)
    TXT_MASCOTA_REDES,
    TXT_MASCOTA_NUEVAS,
    TXT_MASCOTA_WII,
    TXT_ESTADO_IDLE,
    TXT_ESTADO_SCANNING,
    TXT_ESTADO_HAPPY,
    TXT_ESTADO_CURIOSA,
    TXT_ESTADO_SLEEP,

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
