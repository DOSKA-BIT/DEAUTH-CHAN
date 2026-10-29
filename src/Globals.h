#ifndef GLOBALS_H
#define GLOBALS_H

// Declaraciones compartidas entre main.cpp y los Screen*.cpp/Menu.cpp
// en los que se partio la interfaz (antes todo esto vivia junto, a
// mano, arriba de un main.cpp de mas de 2000 lineas). La idea es
// simple: ESTE archivo solo tiene declaraciones (extern, prototipos,
// enums y structs - que no ocupan memoria propia) y cada variable de
// verdad se sigue definiendo en un solo lugar (main.cpp, salvo que se
// aclare lo contrario en el comentario de esa variable). No cambia
// ningun comportamiento del firmware, es puramente organizativo.

#include <Arduino.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "boards/BoardConfig.h"
#include "TouchDriver.h"
#if TOUCH_IS_CAPACITIVE
    #include "TouchCapacitive.h"
#elif TOUCH_SHARES_TFT_BUS
    #include "TouchResistiveSharedBus.h"
#else
    #include "TouchResistive.h"
#endif
#include "Mascota.h"
#include "WiFiScanner.h"
#include "PCAPWriter.h"
#include "BLEScanner.h"
#include "BLE_DB.h"
#include "GPSModule.h"
#include "Wardriving.h"
#include "Learning.h"
#include "EventLog.h"
#include "Theme.h"
#include "Iconos.h"
#include "Deauther.h"

// ---- Objetos globales (definidos en main.cpp) ----
extern TFT_eSPI tft;

#if TOUCH_IS_CAPACITIVE
    extern TouchCapacitive touchDriver;
#elif TOUCH_SHARES_TFT_BUS
    extern TouchResistiveSharedBus touchDriver;
#else
    extern TouchResistive touchDriver;
#endif

extern Mascota mascota;
extern WiFiScanner scanner;
extern PCAPWriter pcap;
extern BLEScanner radar;
extern GPSModule gps;
extern Wardriving wardriver;
extern Learning aprendizaje;
extern EventLog eventos;

// ---- Estado del ultimo escaneo WiFi ----
extern RedInfo redes[MAX_REDES_ESCANEO];
extern int numRedes;
extern bool escaneando;

// ---- Escaneo automatico de fondo (ver el comentario largo junto a
// la definicion de AUTO_SCAN_MS en main.cpp) ----
extern const unsigned long AUTO_SCAN_MS;
extern unsigned long ultimoAutoScan;
extern bool escaneoEsAutomatico;

// ---- Acumulado historico de canales (pantalla CANALES) ----
extern int conteoAcumuladoCanales[13];
extern int escaneosAcumulados;

// ---- Orden de la lista de CONOCIDAS ----
extern int ordenConocidas;
extern const int ALTO_BOTON_ORDEN;

// ---- Filtro de Wardriving y resultados de export ----
extern int filtroWardrive;
extern int ultimoExportKML;
extern int ultimoExportResumen;

// ---- Conteo de clientes en curso (pantalla ESCANEAR) ----
extern int redIndiceConteo;

// ---- Inactividad / redibujado ----
extern unsigned long ultimoToque;
extern bool contenidoSucio;

// ---- Pantallas ----
enum MenuScreen {
    SCREEN_MAIN,
    SCREEN_TOOLS,     // menu de categorias (WIFI / WARDRIVING / RADAR / AJUSTES)
    SCREEN_TOOLS_SUB, // lista de herramientas DENTRO de la categoria elegida
    SCREEN_SCAN,
    SCREEN_STATS,
    SCREEN_WARDRIVE,
    SCREEN_CONFIG,
    SCREEN_CONOCIDAS,
    SCREEN_EVENTOS,
    SCREEN_MONITOR,
    SCREEN_CANALES,
    SCREEN_ANALIZADOR,
    SCREEN_DUPLICADOS,
    SCREEN_RADAR,
    SCREEN_GAME
};
extern MenuScreen pantallaActual;

// ---- Destello de toque + navegacion pendiente ----
extern int filaDestellada;
extern unsigned long momentoDestello;
extern const unsigned long DESTELLO_MS;
extern MenuScreen destinoPendiente;
extern int categoriaSeleccionada;

// ---- Barra de botones fija de abajo ----
struct Boton {
    int x, y, w, h;
    const char* label;
};
extern Boton menuBotones[2];
void calcularLayoutMenu();

// ---- Categorias del submenu de Herramientas ----
enum { CAT_WIFI, CAT_WARDRIVING, CAT_RADAR, CAT_AJUSTES, CANTIDAD_CATEGORIAS };

struct Categoria {
    const char* nombre;
    const char* descripcion;
    uint16_t color;
    const uint8_t* icono;
    int y, h; // se calculan en calcularLayoutCategorias()
};
extern Categoria categorias[CANTIDAD_CATEGORIAS];

// ---- Herramientas (una fila por pantalla dentro de una categoria) ----
struct Herramienta {
    const char* nombre;
    const char* descripcion;
    int categoria;         // indice en categorias[] (CAT_WIFI, etc)
    MenuScreen destino;
    uint16_t color;        // color de categoria (icono + texto)
    const uint8_t* icono;  // bitmap 16x16 de Iconos.h
    int y, h; // se calculan en layoutHerramientasDeCategoria()
};
extern Herramienta herramientas[];
extern const int CANTIDAD_HERRAMIENTAS;

// ---- Medidas de chrome compartido (titulo de seccion, pie de ayuda,
// botones fijos de algunas pantallas) ----
extern const int ALTO_TITULO_SECCION;
extern const int ALTO_PIE_AYUDA;
extern const int ALTO_BOTON_EXPORTAR;
extern const int ALTO_BLOQUE_MONITOR;
extern const int ALTO_CONTROLES_ANALIZADOR;

// ---- Analizador de canales: controles de pantalla (ScreenWifi.cpp) ----
// Puramente visuales (que tan grande se dibuja el grafico y cuantas
// muestras entran en pantalla) - no tocan el radio para nada, a
// diferencia del canal elegido (ese vive en WiFiScanner, ver
// setCanalAnalizador()).
extern int analizadorEscalaX;
extern int analizadorEscalaY;

// ---- RADAR: timing del barrido (definidos en ScreenRadar.cpp) ----
extern const unsigned long RADAR_BARRIDO_MS;
extern const unsigned long RADAR_REFRESCO_MS;

// ---- Chrome compartido / menu de dos niveles (Menu.cpp) ----
int alturaListado();
void dibujarTituloSeccion(const char* titulo, uint16_t color = COLOR_ACCENT, const uint8_t* icono = nullptr);
void dibujarPieAyuda(const char* texto);
void dibujarMenuBotones();
void calcularLayoutCategorias();
int layoutHerramientasDeCategoria(int categoria, int* indicesOut);
void dibujarPantallaCategorias();
void dibujarPantallaHerramientasSub();
void manejarToqueCategorias(int y);
void manejarToqueHerramientasSub(int y);
bool esPantallaHoja(MenuScreen pantalla);

// ---- Helpers de dibujo estilo Fennec (Menu.cpp) - vocabulario visual
// compartido por TODAS las pantallas desde la fase 3 del rediseño
// (paleta monocromatica ambar + trazos calcados de cifertech/fennec:
// texto con letras separadas, lineas punteadas en vez de solidas,
// marcos de solo esquinas en vez de rectangulos rellenos/con borde
// completo, y barras tipo "medidor" segmentadas en vez de una barra
// de progreso lisa). Ninguno de estos cambia layout, hitboxes de
// toque ni logica - solo COMO se pinta cada cosa.
int anchoTextoEspaciado(const char* texto, int espacio = 2);
void dibujarTextoEspaciado(int x, int y, const char* texto, uint16_t color, uint16_t fondo, int espacio = 2);
void dibujarLineaPunteada(int x, int y, int w, uint16_t color);
void dibujarEsquinas(int x, int y, int w, int h, uint16_t color, int largo = 4);
void dibujarBarraSegmentada(int x, int y, int w, int h, int segmentos, int llenos, uint16_t colorLleno, uint16_t colorVacio);
void dibujarBarraSegmentadaV(int x, int y, int w, int h, int segmentos, int llenos, uint16_t colorLleno, uint16_t colorVacio);

// ---- Pantallas de la categoria WIFI (ScreenWifi.cpp) ----
void dibujarListaRedes();
void dibujarPantallaStats();
const char* nombreOrdenConocidas(int orden);
void dibujarPantallaConocidas();
void dibujarPantallaEventos();
void dibujarPantallaMonitor();
void dibujarPantallaCanales();
void dibujarPantallaAnalizador();
void dibujarPantallaDuplicados();
void dibujarPantallaGame();
int exportarResumenSesion();
void manejarToqueWifi(MenuScreen pantalla, int x, int y);

// ---- Pantalla de la categoria WARDRIVING (ScreenWardriving.cpp) ----
void dibujarPantallaWardrive();
void manejarToqueWardriving(int x, int y);

// ---- Pantalla de la categoria RADAR (ScreenRadar.cpp) ----
int anguloVisualBLE(const uint8_t* mac);
void dibujarPantallaRadar();

// ---- Pantalla de la categoria AJUSTES (ScreenAjustes.cpp) ----
void actualizarDiagnosticoHardware();
void dibujarPantallaConfig();

// ---- Dispatcher de toque (definido en main.cpp) ----
void manejarToque(int x, int y);

#endif
