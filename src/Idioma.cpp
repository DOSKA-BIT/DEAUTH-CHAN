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

    "GPS: ",                                       // TXT_WARDRIVE_GPS
    "FIX",                                         // TXT_WARDRIVE_FIX
    "SIN FIX",                                     // TXT_WARDRIVE_SIN_FIX
    "Satelites: ",                                 // TXT_WARDRIVE_SATELITES
    "Redes guardadas: ",                           // TXT_WARDRIVE_REDES_GUARDADAS
    "Filtro [toca]: ",                             // TXT_WARDRIVE_FILTRO
    "Todas",                                       // TXT_WARDRIVE_FILTRO_TODAS
    "Solo abiertas",                               // TXT_WARDRIVE_FILTRO_ABIERTAS
    "Solo con clave",                              // TXT_WARDRIVE_FILTRO_CLAVE
    "[Toca para exportar KML]",                    // TXT_WARDRIVE_EXPORTAR
    "(todavia no exportaste nada)",                // TXT_WARDRIVE_EXPORT_NADA
    "Error: sin SD o sin datos",                   // TXT_WARDRIVE_EXPORT_ERROR
    " puntos",                                     // TXT_WARDRIVE_PUNTOS
    "Guarda cada red vista, con GPS si hay fix",   // TXT_WARDRIVE_PIE

    "Escaneando...",                               // TXT_SCAN_ESCANEANDO
    "Esperando resultados...",                     // TXT_SCAN_PIE_ESPERANDO
    "Escuchando clientes de",                      // TXT_SCAN_ESCUCHANDO_CLIENTES
    "s restantes",                                 // TXT_SCAN_S_RESTANTES
    "Pasivo: solo escucha, no se conecta",         // TXT_SCAN_PIE_PASIVO_CLIENTES
    "Clientes de ",                                // TXT_SCAN_CLIENTES_DE
    "Nadie hablando con esta red ahora",           // TXT_SCAN_NADIE_HABLANDO
    "+ ",                                          // TXT_SCAN_MAS_PREFIJO
    " mas",                                        // TXT_SCAN_MAS_SUFIJO
    "Toca para volver a la lista",                 // TXT_SCAN_PIE_VOLVER
    "Toca aca para escanear",                      // TXT_SCAN_TOCA_ESCANEAR
    "Pasivo: no se conecta a ninguna red",         // TXT_SCAN_PIE_PASIVO_VACIO
    "Toca una red para ver sus clientes",          // TXT_SCAN_PIE_TOCA_RED

    "*OCULTA*",                                    // TXT_OCULTA

    "Redes conocidas: ",                           // TXT_STATS_REDES_CONOCIDAS
    "Vistas este escaneo: ",                       // TXT_STATS_VISTAS_ESCANEO
    "Nuevas descubiertas: ",                       // TXT_STATS_NUEVAS_DESCUBIERTAS
    "Ultima red top: ",                            // TXT_STATS_ULTIMA_RED_TOP
    "Vista ",                                      // TXT_STATS_VISTA_PREFIJO
    " veces",                                      // TXT_STATS_VISTA_SUFIJO
    "Se actualiza con cada escaneo",               // TXT_STATS_PIE

    "SENIAL",                                      // TXT_ORDEN_SENIAL
    "VISTAS",                                      // TXT_ORDEN_VISTAS
    "NOMBRE",                                      // TXT_ORDEN_NOMBRE

    "Orden: ",                                     // TXT_CONOCIDAS_ORDEN
    "  (tocar para cambiar)",                      // TXT_CONOCIDAS_TOCAR_CAMBIAR
    "Todavia no hay redes guardadas",              // TXT_CONOCIDAS_VACIO
    "Escanea para empezar a guardar redes",        // TXT_CONOCIDAS_PIE_VACIO
    "+ %d mas, por %s",                            // TXT_CONOCIDAS_MAS_FORMAT
    "Toca 'Orden' arriba para cambiar el criterio",// TXT_CONOCIDAS_PIE_CAMBIAR

    "[Toca para exportar resumen .txt]",           // TXT_EVENTOS_EXPORTAR
    "Error: la SD no respondio",                   // TXT_EVENTOS_EXPORT_ERROR
    " lineas",                                     // TXT_EVENTOS_LINEAS
    "Todavia no paso nada en esta sesion",         // TXT_EVENTOS_VACIO
    "El historial empieza al encender",            // TXT_EVENTOS_PIE_VACIO
    "+ %d mas viejos",                             // TXT_EVENTOS_MAS_FORMAT
    "El mas reciente arriba de todo",              // TXT_EVENTOS_PIE_RECIENTE

    "Deauth: ",                                    // TXT_MONITOR_DEAUTH
    "Disassoc: ",                                  // TXT_MONITOR_DISASSOC
    "PCAP: ",                                      // TXT_MONITOR_PCAP
    "grabando (toca p/parar)",                     // TXT_MONITOR_PCAP_ON
    "apagado (toca p/grabar)",                     // TXT_MONITOR_PCAP_OFF
    "Paquetes guardados: ",                        // TXT_MONITOR_PAQUETES_GUARDADOS
    "Nada detectado por ahora",                    // TXT_MONITOR_VACIO
    "Pasivo: solo mira, no transmite nada",        // TXT_MONITOR_PIE_PASIVO
    "  de ..",                                     // TXT_MONITOR_DE
    "+ %d mas",                                    // TXT_MONITOR_MAS_FORMAT

    "Hace falta un escaneo primero",               // TXT_NECESITA_ESCANEO
    "Anda a ESCANEAR y volve aca",                 // TXT_PIE_ANDA_ESCANEAR

    "Historico (%d escaneos): CH%d el mas usado",  // TXT_CANALES_HISTORICO_FORMAT
    "Barra mas corta = canal menos saturado",      // TXT_CANALES_PIE

    "Toca - o + para ajustar Channel / escala Y / X", // TXT_ANALIZADOR_PIE

    "Ningun SSID repetido en este escaneo",        // TXT_DUPLICADO_NINGUNO
    "Puede cambiar en el proximo escaneo",         // TXT_DUPLICADO_PIE_NINGUNO
    "+ %d mas con el mismo problema",              // TXT_DUPLICADO_MAS_FORMAT
    "Puede ser mesh real - no es 100% certeza",    // TXT_DUPLICADO_PIE_MESH

    "Proximamente...",                             // TXT_GAME_PROXIMAMENTE
    "Todavia no hay nada aca",                     // TXT_GAME_PIE

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

    "GPS: ",                                       // TXT_WARDRIVE_GPS
    "FIX",                                         // TXT_WARDRIVE_FIX
    "NO FIX",                                      // TXT_WARDRIVE_SIN_FIX
    "Satellites: ",                                // TXT_WARDRIVE_SATELITES
    "Saved networks: ",                            // TXT_WARDRIVE_REDES_GUARDADAS
    "Filter [tap]: ",                               // TXT_WARDRIVE_FILTRO
    "All",                                         // TXT_WARDRIVE_FILTRO_TODAS
    "Open only",                                   // TXT_WARDRIVE_FILTRO_ABIERTAS
    "Encrypted only",                              // TXT_WARDRIVE_FILTRO_CLAVE
    "[Tap to export KML]",                         // TXT_WARDRIVE_EXPORTAR
    "(nothing exported yet)",                      // TXT_WARDRIVE_EXPORT_NADA
    "Error: no SD or no data",                     // TXT_WARDRIVE_EXPORT_ERROR
    " points",                                     // TXT_WARDRIVE_PUNTOS
    "Saves every network seen, with GPS if there's a fix", // TXT_WARDRIVE_PIE

    "Scanning...",                                 // TXT_SCAN_ESCANEANDO
    "Waiting for results...",                      // TXT_SCAN_PIE_ESPERANDO
    "Listening for clients of",                    // TXT_SCAN_ESCUCHANDO_CLIENTES
    "s left",                                      // TXT_SCAN_S_RESTANTES
    "Passive: only listens, never connects",       // TXT_SCAN_PIE_PASIVO_CLIENTES
    "Clients of ",                                 // TXT_SCAN_CLIENTES_DE
    "No one talking to this network right now",    // TXT_SCAN_NADIE_HABLANDO
    "+ ",                                          // TXT_SCAN_MAS_PREFIJO
    " more",                                       // TXT_SCAN_MAS_SUFIJO
    "Tap to go back to the list",                  // TXT_SCAN_PIE_VOLVER
    "Tap here to scan",                            // TXT_SCAN_TOCA_ESCANEAR
    "Passive: never connects to any network",      // TXT_SCAN_PIE_PASIVO_VACIO
    "Tap a network to see its clients",            // TXT_SCAN_PIE_TOCA_RED

    "*HIDDEN*",                                    // TXT_OCULTA

    "Known networks: ",                            // TXT_STATS_REDES_CONOCIDAS
    "Seen this scan: ",                            // TXT_STATS_VISTAS_ESCANEO
    "Newly discovered: ",                          // TXT_STATS_NUEVAS_DESCUBIERTAS
    "Latest top network: ",                        // TXT_STATS_ULTIMA_RED_TOP
    "Seen ",                                       // TXT_STATS_VISTA_PREFIJO
    " times",                                      // TXT_STATS_VISTA_SUFIJO
    "Updates with every scan",                     // TXT_STATS_PIE

    "SIGNAL",                                      // TXT_ORDEN_SENIAL
    "SEEN",                                        // TXT_ORDEN_VISTAS
    "NAME",                                        // TXT_ORDEN_NOMBRE

    "Sort: ",                                      // TXT_CONOCIDAS_ORDEN
    "  (tap to change)",                           // TXT_CONOCIDAS_TOCAR_CAMBIAR
    "No saved networks yet",                       // TXT_CONOCIDAS_VACIO
    "Scan to start saving networks",               // TXT_CONOCIDAS_PIE_VACIO
    "+ %d more, by %s",                            // TXT_CONOCIDAS_MAS_FORMAT
    "Tap 'Sort' above to change the criteria",     // TXT_CONOCIDAS_PIE_CAMBIAR

    "[Tap to export summary .txt]",                // TXT_EVENTOS_EXPORTAR
    "Error: the SD card didn't respond",           // TXT_EVENTOS_EXPORT_ERROR
    " lines",                                      // TXT_EVENTOS_LINEAS
    "Nothing has happened in this session yet",    // TXT_EVENTOS_VACIO
    "History starts at power-on",                  // TXT_EVENTOS_PIE_VACIO
    "+ %d older",                                  // TXT_EVENTOS_MAS_FORMAT
    "Most recent at the top",                       // TXT_EVENTOS_PIE_RECIENTE

    "Deauth: ",                                    // TXT_MONITOR_DEAUTH
    "Disassoc: ",                                  // TXT_MONITOR_DISASSOC
    "PCAP: ",                                      // TXT_MONITOR_PCAP
    "recording (tap to stop)",                     // TXT_MONITOR_PCAP_ON
    "off (tap to record)",                         // TXT_MONITOR_PCAP_OFF
    "Packets saved: ",                             // TXT_MONITOR_PAQUETES_GUARDADOS
    "Nothing detected so far",                     // TXT_MONITOR_VACIO
    "Passive: only watches, never transmits",      // TXT_MONITOR_PIE_PASIVO
    "  from ..",                                   // TXT_MONITOR_DE
    "+ %d more",                                   // TXT_MONITOR_MAS_FORMAT

    "Needs a scan first",                          // TXT_NECESITA_ESCANEO
    "Go to SCAN and come back here",               // TXT_PIE_ANDA_ESCANEAR

    "History (%d scans): CH%d most used",          // TXT_CANALES_HISTORICO_FORMAT
    "Shorter bar = less congested channel",        // TXT_CANALES_PIE

    "Tap - or + to adjust Channel / scale Y / X",  // TXT_ANALIZADOR_PIE

    "No repeated SSID in this scan",               // TXT_DUPLICADO_NINGUNO
    "May change on the next scan",                 // TXT_DUPLICADO_PIE_NINGUNO
    "+ %d more with the same issue",               // TXT_DUPLICADO_MAS_FORMAT
    "Could be a real mesh network - not 100% certain", // TXT_DUPLICADO_PIE_MESH

    "Coming soon...",                              // TXT_GAME_PROXIMAMENTE
    "Nothing here yet",                            // TXT_GAME_PIE

    "LANGUAGE",                                    // TXT_IDIOMA_TITULO
    "Tap a language to switch to it",              // TXT_IDIOMA_PIE
};

const char* tr(TextoId id) {
    if (id < 0 || id >= CANTIDAD_TEXTOS) return "";
    return (ajustes.getIdioma() == IDIOMA_EN) ? EN[id] : ES[id];
}
