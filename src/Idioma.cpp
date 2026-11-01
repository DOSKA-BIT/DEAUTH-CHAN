#include "Idioma.h"
#include "Globals.h" // para ajustes.getIdioma()

// Tabla en español. El orden tiene que calzar EXACTO con el enum
// TextoId de Idioma.h - cada linea lleva su clave como comentario
// para poder auditar visualmente que no se corrio ninguna.
static const char* ES[CANTIDAD_TEXTOS] = {
    "Mascota",                                     // TXT_BOTON_MASCOTA
    "Herramientas",                                // TXT_BOTON_HERRAMIENTAS
    "Atras",                                       // TXT_BOTON_ATRAS

    "HERRAMIENTAS",                                // TXT_TITULO_HERRAMIENTAS
    "Toca una categoria para ver sus herramientas",// TXT_PIE_TOCA_CATEGORIA
    "Toca una herramienta para entrar",            // TXT_PIE_TOCA_HERRAMIENTA

    "WIFI",                                        // TXT_CAT_WIFI_NOMBRE
    "Escaneo, stats, conocidas, monitor y eventos",// TXT_CAT_WIFI_DESC
    "WARDRIVING",                                  // TXT_CAT_WARDRIVING_NOMBRE
    "Guardado de ubicacion + redes en la SD",      // TXT_CAT_WARDRIVING_DESC
    "RADAR",                                       // TXT_CAT_RADAR_NOMBRE
    "Dispositivos BLE cerca y red Find My de Apple",// TXT_CAT_RADAR_DESC
    "AJUSTES",                                     // TXT_CAT_AJUSTES_NOMBRE
    "Datos de la placa y el estado de la SD",      // TXT_CAT_AJUSTES_DESC

    "ESCANEAR",                                    // TXT_HERR_ESCANEAR_NOMBRE
    "Buscar redes WiFi cerca",                     // TXT_HERR_ESCANEAR_DESC
    "STATS",                                       // TXT_HERR_STATS_NOMBRE
    "Memoria de redes ya vistas",                  // TXT_HERR_STATS_DESC
    "CONOCIDAS",                                   // TXT_HERR_CONOCIDAS_NOMBRE
    "Redes guardadas, por senial",                 // TXT_HERR_CONOCIDAS_DESC
    "MONITOR",                                     // TXT_HERR_MONITOR_NOMBRE
    "Deauth/disassoc detectados",                  // TXT_HERR_MONITOR_DESC
    "EVENTOS",                                     // TXT_HERR_EVENTOS_NOMBRE
    "Historial de esta sesion",                    // TXT_HERR_EVENTOS_DESC
    "CANALES",                                     // TXT_HERR_CANALES_NOMBRE
    "Uso del espectro, canal por canal",           // TXT_HERR_CANALES_DESC
    "ANALIZADOR",                                  // TXT_HERR_ANALIZADOR_NOMBRE
    "Trafico en vivo, canal por canal",            // TXT_HERR_ANALIZADOR_DESC
    "DUPLICADO",                                   // TXT_HERR_DUPLICADO_NOMBRE
    "SSID repetido en el ultimo escaneo",          // TXT_HERR_DUPLICADO_DESC
    "GAME",                                        // TXT_HERR_GAME_NOMBRE
    "Proximamente",                                // TXT_HERR_GAME_DESC
    "WARDRIVE",                                    // TXT_HERR_WARDRIVE_NOMBRE
    "Guardar ubicacion + redes en SD",             // TXT_HERR_WARDRIVE_DESC
    "RADAR",                                       // TXT_HERR_RADAR_NOMBRE
    "BLE cerca + red Find My de Apple",            // TXT_HERR_RADAR_DESC
    "RASTREADOR",                                  // TXT_HERR_RASTREADOR_NOMBRE
    "Ubicar un dispositivo BLE por senial",        // TXT_HERR_RASTREADOR_DESC
    "CONFIG",                                      // TXT_HERR_CONFIG_NOMBRE
    "Datos de la placa y la SD",                   // TXT_HERR_CONFIG_DESC
    "IDIOMA",                                      // TXT_HERR_IDIOMA_NOMBRE
    "Cambiar el idioma de la interfaz",            // TXT_HERR_IDIOMA_DESC

    "Buscando...",                                 // TXT_RADAR_PIE_BUSCANDO
    "Rojo = red Find My (no es 100% un AirTag)",   // TXT_RADAR_PIE_FINDMY
    "%d cerca - centro = senial mas fuerte",       // TXT_RADAR_PIE_CERCA

    "Buscando dispositivos BLE...",                // TXT_RASTREADOR_PIE_BUSCANDO
    "Toca un dispositivo para rastrearlo",         // TXT_RASTREADOR_PIE_LISTA
    "DISPOSITIVO PERDIDO",                         // TXT_RASTREADOR_PERDIDO
    "Toca la pantalla para volver a la lista",     // TXT_RASTREADOR_PIE_VOLVER
    "SIN SENIAL RECIENTE",                         // TXT_RASTREADOR_SIN_SENIAL
    "ACERCANDOSE",                                 // TXT_RASTREADOR_ACERCANDOSE
    "ALEJANDOSE",                                  // TXT_RASTREADOR_ALEJANDOSE
    "SIN CAMBIOS",                                 // TXT_RASTREADOR_SIN_CAMBIOS

    "BRILLO",                                      // TXT_CONFIG_BRILLO
    "AUTOSLEEP",                                   // TXT_CONFIG_AUTOSLEEP
    "NUNCA",                                       // TXT_CONFIG_AUTOSLEEP_NUNCA
    "SEGURO? TOCA DE NUEVO",                       // TXT_CONFIG_BORRAR_ARMAR
    "LISTO, SE BORRARON",                          // TXT_CONFIG_BORRAR_HECHO
    "BORRAR REDES GUARDADAS",                      // TXT_CONFIG_BORRAR_NORMAL
    "Placa: ",                                     // TXT_CONFIG_PLACA
    "Pant: ",                                      // TXT_CONFIG_PANT
    "  Touch: ",                                   // TXT_CONFIG_TOUCH
    "Capacitivo",                                  // TXT_CONFIG_CAPACITIVO
    "Resistivo",                                   // TXT_CONFIG_RESISTIVO
    "SD: ",                                        // TXT_CONFIG_SD
    "OK",                                          // TXT_CONFIG_SD_OK
    "No detectada",                                // TXT_CONFIG_SD_NO_DETECTADA
    "Toca - / + para ajustar, o Borrar redes abajo", // TXT_CONFIG_PIE
    "Heap: ",                                      // TXT_CONFIG_HEAP
    " KB libre/minimo",                            // TXT_CONFIG_HEAP_SUFIJO
    "Uptime: ",                                    // TXT_CONFIG_UPTIME
    "  CPU ",                                      // TXT_CONFIG_CPU

    "IDIOMA",                                      // TXT_IDIOMA_TITULO
    "Toca un idioma para activarlo",               // TXT_IDIOMA_PIE
};

// Tabla en ingles - mismo orden exacto que ES[] de arriba.
static const char* EN[CANTIDAD_TEXTOS] = {
    "Pet",                                         // TXT_BOTON_MASCOTA
    "Tools",                                       // TXT_BOTON_HERRAMIENTAS
    "Back",                                        // TXT_BOTON_ATRAS

    "TOOLS",                                       // TXT_TITULO_HERRAMIENTAS
    "Tap a category to see its tools",             // TXT_PIE_TOCA_CATEGORIA
    "Tap a tool to open it",                       // TXT_PIE_TOCA_HERRAMIENTA

    "WIFI",                                        // TXT_CAT_WIFI_NOMBRE
    "Scan, stats, known, monitor and events",      // TXT_CAT_WIFI_DESC
    "WARDRIVING",                                  // TXT_CAT_WARDRIVING_NOMBRE
    "Save location + networks to the SD card",     // TXT_CAT_WARDRIVING_DESC
    "RADAR",                                       // TXT_CAT_RADAR_NOMBRE
    "Nearby BLE devices and Apple's Find My network",// TXT_CAT_RADAR_DESC
    "SETTINGS",                                    // TXT_CAT_AJUSTES_NOMBRE
    "Board info and SD card status",               // TXT_CAT_AJUSTES_DESC

    "SCAN",                                        // TXT_HERR_ESCANEAR_NOMBRE
    "Look for nearby WiFi networks",               // TXT_HERR_ESCANEAR_DESC
    "STATS",                                       // TXT_HERR_STATS_NOMBRE
    "Memory of networks seen so far",              // TXT_HERR_STATS_DESC
    "KNOWN",                                       // TXT_HERR_CONOCIDAS_NOMBRE
    "Saved networks, sorted by signal",            // TXT_HERR_CONOCIDAS_DESC
    "MONITOR",                                     // TXT_HERR_MONITOR_NOMBRE
    "Deauth/disassoc frames detected",             // TXT_HERR_MONITOR_DESC
    "EVENTS",                                      // TXT_HERR_EVENTOS_NOMBRE
    "This session's history",                      // TXT_HERR_EVENTOS_DESC
    "CHANNELS",                                    // TXT_HERR_CANALES_NOMBRE
    "Spectrum usage, channel by channel",          // TXT_HERR_CANALES_DESC
    "ANALYZER",                                    // TXT_HERR_ANALIZADOR_NOMBRE
    "Live traffic on one channel",                 // TXT_HERR_ANALIZADOR_DESC
    "DUPLICATE",                                   // TXT_HERR_DUPLICADO_NOMBRE
    "Same SSID seen twice in the last scan",       // TXT_HERR_DUPLICADO_DESC
    "GAME",                                        // TXT_HERR_GAME_NOMBRE
    "Coming soon",                                 // TXT_HERR_GAME_DESC
    "WARDRIVE",                                    // TXT_HERR_WARDRIVE_NOMBRE
    "Save location + networks to the SD card",     // TXT_HERR_WARDRIVE_DESC
    "RADAR",                                       // TXT_HERR_RADAR_NOMBRE
    "Nearby BLE + Apple's Find My network",         // TXT_HERR_RADAR_DESC
    "TRACKER FINDER",                              // TXT_HERR_RASTREADOR_NOMBRE
    "Locate a BLE device by signal strength",      // TXT_HERR_RASTREADOR_DESC
    "CONFIG",                                      // TXT_HERR_CONFIG_NOMBRE
    "Board info and SD card",                      // TXT_HERR_CONFIG_DESC
    "LANGUAGE",                                    // TXT_HERR_IDIOMA_NOMBRE
    "Change the interface language",               // TXT_HERR_IDIOMA_DESC

    "Searching...",                                // TXT_RADAR_PIE_BUSCANDO
    "Red = Find My network (not necessarily an AirTag)", // TXT_RADAR_PIE_FINDMY
    "%d nearby - center = strongest signal",       // TXT_RADAR_PIE_CERCA

    "Searching for BLE devices...",                // TXT_RASTREADOR_PIE_BUSCANDO
    "Tap a device to track it",                    // TXT_RASTREADOR_PIE_LISTA
    "DEVICE LOST",                                 // TXT_RASTREADOR_PERDIDO
    "Tap the screen to go back to the list",       // TXT_RASTREADOR_PIE_VOLVER
    "NO RECENT SIGNAL",                            // TXT_RASTREADOR_SIN_SENIAL
    "GETTING CLOSER",                               // TXT_RASTREADOR_ACERCANDOSE
    "GETTING FARTHER",                              // TXT_RASTREADOR_ALEJANDOSE
    "NO CHANGE",                                   // TXT_RASTREADOR_SIN_CAMBIOS

    "BRIGHTNESS",                                  // TXT_CONFIG_BRILLO
    "AUTOSLEEP",                                   // TXT_CONFIG_AUTOSLEEP
    "NEVER",                                       // TXT_CONFIG_AUTOSLEEP_NUNCA
    "SURE? TAP AGAIN",                             // TXT_CONFIG_BORRAR_ARMAR
    "DONE, ERASED",                                // TXT_CONFIG_BORRAR_HECHO
    "ERASE SAVED NETWORKS",                        // TXT_CONFIG_BORRAR_NORMAL
    "Board: ",                                     // TXT_CONFIG_PLACA
    "Screen: ",                                    // TXT_CONFIG_PANT
    "  Touch: ",                                   // TXT_CONFIG_TOUCH
    "Capacitive",                                  // TXT_CONFIG_CAPACITIVO
    "Resistive",                                   // TXT_CONFIG_RESISTIVO
    "SD: ",                                        // TXT_CONFIG_SD
    "OK",                                          // TXT_CONFIG_SD_OK
    "Not detected",                                // TXT_CONFIG_SD_NO_DETECTADA
    "Tap - / + to adjust, or Erase networks below", // TXT_CONFIG_PIE
    "Heap: ",                                      // TXT_CONFIG_HEAP
    " KB free/min",                                // TXT_CONFIG_HEAP_SUFIJO
    "Uptime: ",                                    // TXT_CONFIG_UPTIME
    "  CPU ",                                      // TXT_CONFIG_CPU

    "LANGUAGE",                                    // TXT_IDIOMA_TITULO
    "Tap a language to switch to it",              // TXT_IDIOMA_PIE
};

const char* tr(TextoId id) {
    if (id < 0 || id >= CANTIDAD_TEXTOS) return "";
    return (ajustes.getIdioma() == IDIOMA_EN) ? EN[id] : ES[id];
}
