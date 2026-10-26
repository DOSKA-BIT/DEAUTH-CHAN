#include <Arduino.h>
#include <math.h>
#include <strings.h> // strcasecmp(), usado para ordenar CONOCIDAS por nombre
#include <TFT_eSPI.h>
#include <SD.h>
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

TFT_eSPI tft = TFT_eSPI();

#if TOUCH_IS_CAPACITIVE
    TouchCapacitive touchDriver;
#elif TOUCH_SHARES_TFT_BUS
    TouchResistiveSharedBus touchDriver(&tft);
#else
    TouchResistive touchDriver;
#endif

Mascota mascota;
WiFiScanner scanner;
PCAPWriter pcap;
BLEScanner radar;
GPSModule gps;
Wardriving wardriver;
Learning aprendizaje;
EventLog eventos;

RedInfo redes[MAX_REDES_ESCANEO];
int numRedes = 0;
bool escaneando = false;

// Escaneo automatico de fondo: antes WiFi solo escaneaba cuando
// tocabas el boton en ESCANEAR. Ahora, ademas, cada AUTO_SCAN_MS
// (mientras no haya un escaneo manual ni un conteo de clientes en
// curso, y la mascota no este dormida) se dispara un escaneo solo,
// sin que haga falta estar parado en esa pantalla - asi CANALES se ve
// "vivo" en vez de una foto fija, y "NUEVAS" en el encabezado puede
// subir mientras estas mirando otra herramienta. escaneoEsAutomatico
// le dice al bloque que procesa el resultado (ver loop()) si este
// escaneo puntual vino de este timer o de que tocaste el boton, para
// no guardar en Wardriving un punto de ubicacion que vos no pediste.
const unsigned long AUTO_SCAN_MS = 45000;
unsigned long ultimoAutoScan = 0;
bool escaneoEsAutomatico = false;

// Cuantas veces aparecio cada canal (1-13) sumando TODOS los escaneos
// de la sesion (manuales + automaticos), no solo el ultimo - para
// poder mostrar en CANALES cual es el canal mas saturado "en
// general", no nada mas en la foto de este momento. Vive en RAM nomas
// (como los contadores de anomalias de WiFiScanner): se resetea en
// cada reinicio, y eso esta bien, no es un dato que haga falta
// recordar de una sesion a la otra.
int conteoAcumuladoCanales[13] = {0};
int escaneosAcumulados = 0;

// Orden de la lista de CONOCIDAS: 0=senial, 1=veces vista, 2=nombre.
// Se cambia tocando el renglon de arriba de esa pantalla (ver
// dibujarPantallaConocidas() y manejarToque(), caso SCREEN_CONOCIDAS).
// NO hay un orden por "ultima vez vista": ese dato se guarda con
// millis() de CADA sesion, que arranca de cero en cada reinicio, asi
// que comparar el "ultimaVez" de una red guardada hace rato contra el
// de una vista recien en ESTA sesion no diria nada real.
int ordenConocidas = 0;
const int ALTO_BOTON_ORDEN = 18;

// Filtro de que se guarda en wardriving.csv: 0 = todas las redes
// (comportamiento de siempre), 1 = solo las abiertas, 2 = solo las
// que tienen clave. Wardriving::setFilterByEncryption() ya existia
// en el codigo, pero no habia ningun boton que la llamara - y peor,
// saveNetwork() tampoco consultaba el filtro, asi que aunque alguien
// la hubiera llamado a mano no cambiaba nada. Las dos cosas se
// arreglan juntas (ver Wardriving.cpp).
int filtroWardrive = 0;

// Resultado del ultimo intento de exportar el KML: -2 = todavia no se
// toco el boton en esta sesion, -1 = fallo (sin SD o sin CSV todavia),
// 0 o mas = cantidad de puntos que escribio.
int ultimoExportKML = -2;

// Mismo significado que ultimoExportKML de arriba, pero para el
// resumen de sesion que se exporta desde la pantalla de Eventos (ver
// exportarResumenSesion() mas abajo): -2 = no se toco el boton
// todavia, -1 = fallo (sin SD), 0 o mas = cuantas lineas escribio.
int ultimoExportResumen = -2;

// Indice (dentro de redes[]) de la red cuyo conteo de clientes se
// esta mostrando/mostro ultimo en la pantalla de Escaneo. -1 = no hay
// ningun resultado de conteo para mostrar, se ve la lista de redes
// normal.
int redIndiceConteo = -1;

// Momento del ultimo toque real en la pantalla (no cuenta el toque
// que la despierta, ver mas abajo). Arranca en 0 y se pisa recien en
// setup(), asi el reloj de inactividad no empieza a correr hasta que
// el equipo ya esta andando.
unsigned long ultimoToque = 0;

// Avisa que el contenido de la pantalla activa (lista, stats,
// wardriving, config) quedo desactualizado y hay que redibujarlo.
// Antes esto se disparaba con cada parpadeo de la mascota sin
// importar que pantalla estuviera activa; ahora solo se prende
// cuando de verdad cambio algo que vale la pena mostrar.
bool contenidoSucio = true;

enum MenuScreen {
    SCREEN_MAIN,
    SCREEN_TOOLS,     // menu de categorias (WIFI / WARDRIVING / AJUSTES)
    SCREEN_TOOLS_SUB, // lista de herramientas DENTRO de la categoria elegida
    SCREEN_SCAN,
    SCREEN_STATS,
    SCREEN_WARDRIVE,
    SCREEN_CONFIG,
    SCREEN_CONOCIDAS,
    SCREEN_EVENTOS,
    SCREEN_MONITOR,
    SCREEN_CANALES,
    SCREEN_DUPLICADOS,
    SCREEN_RADAR
};
MenuScreen pantallaActual = SCREEN_MAIN;

// Destello al tocar una fila de la lista de Herramientas: como esto es
// tactil (no hay joystick ni cursor que se mueva solo), la unica forma
// de que la barra de "seleccionado" signifique algo real es mostrarla
// un instante justo donde tocaste, antes de entrar a esa pantalla - en
// vez de fingir un cursor fijo que no se mueve nunca. -1 = no hay
// ningun destello activo.
int filaDestellada = -1;
unsigned long momentoDestello = 0;
const unsigned long DESTELLO_MS = 120; // bien cortito, solo para que se note el toque
MenuScreen destinoPendiente = SCREEN_MAIN;

// Que categoria se esta mostrando en SCREEN_TOOLS_SUB (indice dentro
// de categorias[], ver mas abajo). Se fija apenas se toca una fila en
// SCREEN_TOOLS, antes de que termine el destello - asi la pantalla ya
// sabe que lista filtrada dibujar en cuanto arranca la navegacion.
int categoriaSeleccionada = 0;

struct Boton {
    int x, y, w, h;
    const char* label;
};

// Solo dos botones fijos abajo: uno para volver a la mascota y otro
// para la lista de herramientas. Antes habia un boton por pantalla
// (Escanear/Stats/Wardrive/Config) pero ninguno volvia a la pantalla
// principal, asi que una vez que entrabas a cualquiera de esas la
// mascota quedaba inalcanzable hasta reiniciar el ESP. Con esto la
// mascota siempre esta a un toque de distancia, y agregar una
// herramienta nueva el dia de manana es sumar una fila en la lista,
// no pelear por lugar en la barra de abajo.
Boton menuBotones[2];

void calcularLayoutMenu() {
    int anchoBoton = TFT_PANEL_WIDTH / 2;
    int y = TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT;

    menuBotones[0] = {0, y, anchoBoton - 2, 34, "Mascota"};
    menuBotones[1] = {anchoBoton, y, anchoBoton - 2, 34, "Herramientas"};
}

// Las 3 categorias del submenu de Herramientas (pedido del usuario:
// agrupar por tipo en vez de una lista plana de 7 items). Son pocas y
// fijas, asi que van en su propio array chico en vez de inventarse
// aparte de herramientas[] - cada Herramienta de mas abajo apunta a
// una de estas por indice (campo "categoria").
enum { CAT_WIFI, CAT_WARDRIVING, CAT_RADAR, CAT_AJUSTES, CANTIDAD_CATEGORIAS };

struct Categoria {
    const char* nombre;
    const char* descripcion;
    uint16_t color;
    const uint8_t* icono;
    int y, h; // se calculan en calcularLayoutCategorias()
};

// El icono de cada categoria es el de su primera herramienta (no hay
// un set de iconos "de categoria" aparte - no hacia falta duplicar
// arte para esto).
Categoria categorias[CANTIDAD_CATEGORIAS] = {
    {"WIFI",       "Escaneo, stats, conocidas, monitor y eventos", COLOR_CAT_CYAN,   ICONO_ESCANEAR, 0, 0},
    {"WARDRIVING", "Guardado de ubicacion + redes en la SD",       COLOR_CAT_VIOLET, ICONO_WARDRIVE, 0, 0},
    {"RADAR",      "Dispositivos BLE cerca y red Find My de Apple",COLOR_CAT_GREEN,  ICONO_RADAR,    0, 0},
    {"AJUSTES",    "Datos de la placa y el estado de la SD",       COLOR_CAT_BLUE,   ICONO_CONFIG,   0, 0},
};

// Una fila por herramienta en la pantalla de Herramientas: nombre,
// descripcion corta, a que categoria pertenece (para el submenu de
// arriba), a que pantalla lleva al tocarla, y su color + icono de
// categoria (rediseno "WiFi Sniffers" - cada tipo de funcion se
// distingue de un vistazo por su color e icono, no solo por el
// nombre). Agregar una herramienta nueva es agregar una linea aca (y
// su pantalla propia, y su icono en Iconos.h) - no hay que tocar la
// barra de botones para nada.
struct Herramienta {
    const char* nombre;
    const char* descripcion;
    int categoria;         // indice en categorias[] (CAT_WIFI, etc)
    MenuScreen destino;
    uint16_t color;        // color de categoria (icono + texto)
    const uint8_t* icono;  // bitmap 16x16 de Iconos.h
    int y, h; // se calculan en layoutHerramientasDeCategoria()
};

// Nombres en mayuscula a proposito: es parte de la estetica de lista
// de terminal que reemplaza a las tarjetas redondeadas de antes - ver
// dibujarPantallaHerramientasSub(). El color de cada una es su
// categoria: cian = redes/descubrimiento, amarillo = stats/destacado,
// violeta = analisis/ubicacion, azul = configuracion (neutro), blanco
// = listas simples/informativas, rojo = alertas.
Herramienta herramientas[] = {
    {"ESCANEAR",  "Buscar redes WiFi cerca",         CAT_WIFI,       SCREEN_SCAN,      COLOR_CAT_CYAN,   ICONO_ESCANEAR,  0, 0},
    {"STATS",     "Memoria de redes ya vistas",       CAT_WIFI,       SCREEN_STATS,     COLOR_CAT_YELLOW, ICONO_STATS,     0, 0},
    {"CONOCIDAS", "Redes guardadas, por senial",      CAT_WIFI,       SCREEN_CONOCIDAS, COLOR_CAT_WHITE,  ICONO_CONOCIDAS, 0, 0},
    {"MONITOR",   "Deauth/disassoc detectados",       CAT_WIFI,       SCREEN_MONITOR,   COLOR_CAT_RED,    ICONO_MONITOR,   0, 0},
    {"EVENTOS",   "Historial de esta sesion",         CAT_WIFI,       SCREEN_EVENTOS,   COLOR_CAT_BLUE,   ICONO_EVENTOS,   0, 0},
    {"CANALES",   "Uso del espectro, canal por canal", CAT_WIFI,       SCREEN_CANALES,   COLOR_CAT_YELLOW, ICONO_CANALES,   0, 0},
    {"DUPLICADO", "SSID repetido en el ultimo escaneo",CAT_WIFI,       SCREEN_DUPLICADOS,COLOR_CAT_RED,    ICONO_DUPLICADO, 0, 0},
    {"WARDRIVE",  "Guardar ubicacion + redes en SD",  CAT_WARDRIVING, SCREEN_WARDRIVE,  COLOR_CAT_VIOLET, ICONO_WARDRIVE,  0, 0},
    {"RADAR",     "BLE cerca + red Find My de Apple", CAT_RADAR,      SCREEN_RADAR,     COLOR_CAT_GREEN,  ICONO_RADAR,     0, 0},
    {"CONFIG",    "Datos de la placa y la SD",        CAT_AJUSTES,    SCREEN_CONFIG,    COLOR_CAT_BLUE,   ICONO_CONFIG,    0, 0},
};
const int CANTIDAD_HERRAMIENTAS = sizeof(herramientas) / sizeof(herramientas[0]);

int alturaListado(); // definida mas abajo, hace falta antes aca

// Alto que ocupa el titulo de seccion (dibujarTituloSeccion, definida
// mas abajo junto con alturaListado) incluyendo su linea separadora -
// las pantallas que lo usan arrancan su propio contenido despues de
// este offset. Declarado aca arriba porque calcularLayoutCategorias
// lo necesita antes de donde vive la funcion que lo dibuja.
const int ALTO_TITULO_SECCION = 30;

// Alto reservado abajo de CUALQUIER pantalla de Herramientas para el
// renglon de ayuda (texto chico tipo "TAP=SELEC | <=VOLVER") y la
// barra solida decorativa, calcado del layout de PORKCHOP/Marauder
// que mando el usuario de referencia. Antes solo lo usaba la lista de
// Herramientas (se llamaba HERR_ALTO_PIE); ahora lo usan las 7
// pantallas via dibujarPieAyuda() (definida mas abajo) para quedar
// todas parejas.
const int ALTO_PIE_AYUDA = 34;

// Alto del boton fijo de "exportar resumen" arriba de la lista de
// Eventos (ver dibujarPantallaEventos() y manejarToque(), caso
// SCREEN_EVENTOS - comparten este mismo numero para que el toque
// coincida siempre con lo que se ve dibujado).
const int ALTO_BOTON_EXPORTAR = 22;

// Alto del bloque fijo de arriba de Monitor (contadores de deauth/
// disassoc + el toggle de captura PCAP), antes de que arranque la
// lista de anomalias - ver dibujarPantallaMonitor() y manejarToque(),
// caso SCREEN_MONITOR.
const int ALTO_BLOQUE_MONITOR = 54;

// Layout del menu de categorias (SCREEN_TOOLS) - son siempre 3, fijas,
// asi que esto se calcula una sola vez en setup() como antes se hacia
// con las 7 herramientas en un solo nivel.
void calcularLayoutCategorias() {
    int espacioDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int gap = 6;
    int alto = (espacioDisponible - gap * (CANTIDAD_CATEGORIAS - 1)) / CANTIDAD_CATEGORIAS;
    if (alto > 48) alto = 48; // son pocas categorias, que se vean bien grandes
    if (alto < 24) alto = 24;

    int y = CONTENT_TOP + ALTO_TITULO_SECCION;
    for (int i = 0; i < CANTIDAD_CATEGORIAS; i++) {
        categorias[i].y = y;
        categorias[i].h = alto;
        y += alto + gap;
    }
}

// Arma, para SCREEN_TOOLS_SUB, la lista de herramientas que
// pertenecen a "categoria" (sus indices dentro de herramientas[], en
// "indicesOut") y calcula la posicion de cada una - mismo criterio que
// antes usaba un calcularLayoutHerramientas() fijo para las 7 juntas, pero
// ahora filtrado y recalculado en el momento en vez de una sola vez en
// setup(), porque la cantidad cambia segun que categoria se eligio (5
// en WIFI, 1 en WARDRIVING, 1 en AJUSTES). Son pocos items como para
// que valga la pena cachear esto aparte; se llama tanto desde el
// dibujado como desde el toque, siempre con los mismos dos pasos, para
// que nunca queden desincronizados entre si (mismo motivo que el
// comentario de SCREEN_SCAN en manejarToque()).
int layoutHerramientasDeCategoria(int categoria, int* indicesOut) {
    int cantidad = 0;
    for (int i = 0; i < CANTIDAD_HERRAMIENTAS; i++) {
        if (herramientas[i].categoria == categoria) {
            indicesOut[cantidad++] = i;
        }
    }
    if (cantidad == 0) return 0; // no deberia pasar nunca, pero evita una division por 0

    int espacioDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int gap = 2;
    int alto = (espacioDisponible - gap * (cantidad - 1)) / cantidad;
    if (alto > 28) alto = 28;
    if (alto < 18) alto = 18;

    int y = CONTENT_TOP + ALTO_TITULO_SECCION;
    for (int k = 0; k < cantidad; k++) {
        int idx = indicesOut[k];
        herramientas[idx].y = y;
        herramientas[idx].h = alto;
        y += alto + gap;
    }
    return cantidad;
}

void dibujarMenuBotones() {
    // Linea fina arriba de la barra, para separarla del contenido en
    // vez de que los botones queden flotando sin mas.
    tft.drawFastHLine(0, TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - 1, TFT_PANEL_WIDTH, COLOR_ACCENT_DIM);

    for (int i = 0; i < 2; i++) {
        // El boton de Mascota se marca activo solo en la pantalla
        // principal; el de Herramientas se marca activo en cualquier
        // otra pantalla (la lista de herramientas y cada herramienta
        // en si), total todas esas viven del lado de "herramientas".
        bool activo = (i == 0) ? (pantallaActual == SCREEN_MAIN) : (pantallaActual != SCREEN_MAIN);
        // Esquinas cuadradas (antes redondeadas) para que combine con
        // la estetica de lista/terminal del resto del firmware.
        uint16_t colorFondo = activo ? COLOR_ACCENT : COLOR_PANEL;
        uint16_t colorTexto = activo ? COLOR_BG : COLOR_TEXT_DIM;
        tft.fillRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, colorFondo);
        if (!activo) {
            tft.drawRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, COLOR_ACCENT_DIM);
        }
        tft.setTextColor(colorTexto, colorFondo);
        tft.setTextFont(1);
        tft.setTextSize(1);
        int anchoTexto = strlen(menuBotones[i].label) * 6;
        int xCentrado = menuBotones[i].x + (menuBotones[i].w - anchoTexto) / 2;
        tft.setCursor(xCentrado, menuBotones[i].y + 13);
        tft.print(menuBotones[i].label);
    }
}

int alturaListado() {
    // Lo que queda del alto de pantalla descontando la franja de
    // arriba (donde va el resumen de la mascota) y la barra de botones
    // de abajo. Es la zona util para listas, stats, etc en cada
    // pantalla que no sea la principal.
    return TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - CONTENT_TOP;
}

// Encabezado de seccion: nombre de la pantalla en grande + una linea
// fina debajo, calcado del titulo bien visible ("PORKCHOP") de la
// referencia que mando el usuario. Antes cada pantalla lo hacia (o no)
// a mano con sus propios numeros sueltos - Stats, Wardrive y Config ya
// tenian este mismo patron escrito a mano cada una; juntarlo en una
// sola funcion evita que una pantalla quede con un estilo distinto a
// las demas el dia de mañana. No toca el encabezado de arriba de todo
// (REDES/NUEVAS/estado, en Mascota::dibujarEncabezado) - ese es
// compartido entre todas las pantallas y muestra el estado del
// dispositivo; este es el titulo de CADA pantalla en particular.
// "color" es el color de categoria de la herramienta (el mismo que
// usa su fila en dibujarPantallaHerramientasSub() - cian, violeta, rojo,
// amarillo, azul o blanco) para que al tocar una fila y entrar a su
// pantalla el titulo de arriba siga mostrando esa misma identidad de
// color, en vez de que todas las pantallas se vean con el mismo
// celeste generico. "icono" es opcional (la propia lista de
// Herramientas no necesita uno junto a su titulo) - si se pasa, se
// dibuja a la izquierda del texto con el mismo bitmap que ya usa esa
// herramienta en la lista.
void dibujarTituloSeccion(const char* titulo, uint16_t color = COLOR_ACCENT, const uint8_t* icono = nullptr) {
    int x = 10;
    if (icono) {
        tft.drawBitmap(x, CONTENT_TOP + 4, icono, ICONO_ANCHO, ICONO_ALTO, color, COLOR_BG);
        x += ICONO_ANCHO + 6;
    }
    tft.setTextColor(color, COLOR_BG);
    tft.setTextFont(2);
    tft.setCursor(x, CONTENT_TOP + 6);
    tft.print(titulo);
    tft.setTextFont(1);
    // La linea separadora se queda neutra (no de categoria) a
    // proposito: es chrome compartido de toda pantalla, no parte de
    // la identidad visual de ESTA herramienta en particular.
    tft.drawFastHLine(10, CONTENT_TOP + 24, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);
}

// Pie de ayuda: un renglon de texto chico (dim) + una barra solida de
// cierre, calcados del "TAP=SELE... <=BACK" + la franja blanca de
// abajo de todo de la foto de referencia. Las pantallas que lo usan
// tienen que restarse ALTO_PIE_AYUDA de su propio alto disponible
// para que el contenido no se les superponga con esto (ver cada
// pantalla para el ejemplo exacto).
void dibujarPieAyuda(const char* texto) {
    int yAyuda = CONTENT_TOP + alturaListado() - ALTO_PIE_AYUDA + 6;
    tft.fillRect(0, yAyuda - 4, TFT_PANEL_WIDTH, 16, COLOR_BG);
    tft.setTextFont(1);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yAyuda);
    tft.print(texto);

    int yBarra = CONTENT_TOP + alturaListado() - 8;
    tft.fillRect(6, yBarra, TFT_PANEL_WIDTH - 12, 5, COLOR_ACCENT);
}

// Pantalla de arriba de todo del submenu: las 3 categorias (WIFI,
// WARDRIVING, AJUSTES). Tocar una lleva a dibujarPantallaHerramientasSub()
// con esa categoria - ver manejarToque(), caso SCREEN_TOOLS.
void dibujarPantallaCategorias() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("HERRAMIENTAS");

    // Mismo estilo de fila (icono + nombre en su color, destello al
    // tocar) que ya se usaba para la lista plana de 7 - ver el
    // comentario de dibujarPantallaHerramientasSub() aca abajo, que
    // repite este mismo patron para las herramientas DENTRO de una
    // categoria.
    for (int i = 0; i < CANTIDAD_CATEGORIAS; i++) {
        Categoria& c = categorias[i];

        bool destellada = (i == filaDestellada);
        uint16_t colorFondoFila = destellada ? c.color : COLOR_BG;
        uint16_t colorFrenteFila = destellada ? COLOR_BG : c.color;

        if (destellada) {
            tft.fillRect(0, c.y, TFT_PANEL_WIDTH, c.h, c.color);
        }

        int yIcono = c.y + (c.h - ICONO_ALTO) / 2;
        tft.drawBitmap(10, yIcono, c.icono, ICONO_ANCHO, ICONO_ALTO, colorFrenteFila, colorFondoFila);

        tft.setTextColor(colorFrenteFila, colorFondoFila);
        tft.setTextFont(2);
        tft.setCursor(10 + ICONO_ANCHO + 8, c.y + (c.h / 2) - 7);
        tft.print(c.nombre);
    }

    if (filaDestellada >= 0 && filaDestellada < CANTIDAD_CATEGORIAS) {
        dibujarPieAyuda(categorias[filaDestellada].descripcion);
    } else {
        dibujarPieAyuda("Toca una categoria para ver sus herramientas");
    }
}

// Segundo nivel del submenu: las herramientas DENTRO de
// categoriaSeleccionada (fijada en manejarToque() al tocar una fila
// de dibujarPantallaCategorias()). Mismo estilo que esa pantalla, solo
// que filtrado - ver layoutHerramientasDeCategoria() mas arriba para
// como se arma la lista filtrada y por que se recalcula en el momento
// en vez de una vez sola en setup().
void dibujarPantallaHerramientasSub() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    Categoria& cat = categorias[categoriaSeleccionada];
    dibujarTituloSeccion(cat.nombre, cat.color, cat.icono);

    int indices[CANTIDAD_HERRAMIENTAS];
    int cantidad = layoutHerramientasDeCategoria(categoriaSeleccionada, indices);

    // Lista de una sola linea por item, icono + nombre, cada uno con
    // el color de su categoria - calcado del menu de iconos "WiFi
    // Sniffers" que mando el usuario de referencia. La descripcion de
    // cada herramienta se muestra aparte, en el renglon de ayuda de
    // mas abajo, solo mientras esa fila esta destellando por el
    // toque - asi la lista en si queda limpia. "filaDestellada" aca
    // es la POSICION dentro de esta lista filtrada (0..cantidad-1),
    // no el indice global en herramientas[] - ver manejarToque().
    for (int k = 0; k < cantidad; k++) {
        Herramienta& h = herramientas[indices[k]];

        bool destellada = (k == filaDestellada);
        uint16_t colorFondoFila = destellada ? h.color : COLOR_BG;
        uint16_t colorFrenteFila = destellada ? COLOR_BG : h.color;

        if (destellada) {
            tft.fillRect(0, h.y, TFT_PANEL_WIDTH, h.h, h.color);
        }

        int yIcono = h.y + (h.h - ICONO_ALTO) / 2;
        tft.drawBitmap(10, yIcono, h.icono, ICONO_ANCHO, ICONO_ALTO, colorFrenteFila, colorFondoFila);

        tft.setTextColor(colorFrenteFila, colorFondoFila);
        tft.setTextFont(2);
        tft.setCursor(10 + ICONO_ANCHO + 8, h.y + (h.h / 2) - 7);
        tft.print(h.nombre);
    }

    if (filaDestellada >= 0 && filaDestellada < cantidad) {
        dibujarPieAyuda(herramientas[indices[filaDestellada]].descripcion);
    } else {
        dibujarPieAyuda("Toca una herramienta para entrar");
    }
}

const char* nombreOrdenConocidas(int orden) {
    switch (orden) {
        case 1: return "VISTAS";
        case 2: return "NOMBRE";
        default: return "SENIAL";
    }
}

void dibujarPantallaConocidas() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CONOCIDAS", COLOR_CAT_WHITE, ICONO_CONOCIDAS);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    // Renglon de "Orden: X" tocable, mismo criterio que el boton de
    // exportar de EVENTOS (ALTO_BOTON_EXPORTAR): un bloque fijo
    // arriba de la lista, con su propio alto reservado, para que el
    // toque en manejarToque() (caso SCREEN_CONOCIDAS) siempre
    // coincida con lo que se ve dibujado aca.
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(8, top + 4);
    tft.print("Orden: ");
    tft.setTextColor(COLOR_CAT_WHITE, COLOR_BG);
    tft.print(nombreOrdenConocidas(ordenConocidas));
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print("  (tocar para cambiar)");
    tft.drawFastHLine(5, top + ALTO_BOTON_ORDEN, TFT_PANEL_WIDTH - 10, COLOR_ACCENT_DIM);
    top += ALTO_BOTON_ORDEN;

    int total = aprendizaje.getTotalRedesConocidas();
    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Todavia no hay redes guardadas");
        dibujarPieAyuda("Escanea para empezar a guardar redes");
        return;
    }

    // NO hay orden por "ultima vez vista" - ver el comentario largo
    // junto a la declaracion de ordenConocidas, mas arriba: ese dato
    // usa millis() de la sesion en que se vio, y comparar eso entre
    // sesiones distintas (reinicio de por medio) no dice nada real.
    // "Veces vista" en cambio es un contador que se acumula en la SD
    // sesion tras sesion sin ese problema.
    RedStats* todas = aprendizaje.getTodas();
    int indices[MAX_REDES_ESCANEO];
    for (int i = 0; i < total; i++) indices[i] = i;
    for (int i = 0; i < total - 1; i++) {
        for (int j = 0; j < total - 1 - i; j++) {
            RedStats& a = todas[indices[j]];
            RedStats& b = todas[indices[j + 1]];
            bool debeSwap;
            switch (ordenConocidas) {
                case 1:
                    debeSwap = a.vecesVista < b.vecesVista;
                    break;
                case 2: {
                    // Las ocultas (sin SSID) siempre van al final,
                    // alfabetico no significa nada para un nombre
                    // vacio.
                    bool aOculta = strlen(a.ssid) == 0;
                    bool bOculta = strlen(b.ssid) == 0;
                    if (aOculta != bOculta) {
                        debeSwap = aOculta;
                    } else {
                        debeSwap = strcasecmp(a.ssid, b.ssid) > 0;
                    }
                    break;
                }
                default:
                    debeSwap = a.mejorRSSI < b.mejorRSSI;
                    break;
            }
            if (debeSwap) {
                int tmp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = tmp;
            }
        }
    }

    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_BOTON_ORDEN - ALTO_PIE_AYUDA) / 20;
    int yOffset = top + 6;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        RedStats& r = todas[indices[i]];

        // Icono chico (la misma estrella de la fila "CONOCIDAS" del
        // menu de Herramientas) a la izquierda de cada red guardada.
        tft.drawBitmap(5, yOffset - 1, ICONO_CONOCIDAS, ICONO_ANCHO, ICONO_ALTO, COLOR_CAT_WHITE, COLOR_BG);

        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(5 + ICONO_ANCHO + 4, yOffset);
        tft.print(strlen(r.ssid) > 0 ? r.ssid : "*OCULTA*");

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(TFT_PANEL_WIDTH - 95, yOffset);
        tft.print(r.mejorRSSI);
        tft.print("dBm x");
        tft.print(r.vecesVista);

        yOffset += 20;
    }

    if (total > mostradas) {
        char masTexto[32];
        snprintf(masTexto, sizeof(masTexto), "+ %d mas, por %s", total - mostradas, nombreOrdenConocidas(ordenConocidas));
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("Toca 'Orden' arriba para cambiar el criterio");
    }
}

void dibujarPantallaEventos() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("EVENTOS", COLOR_CAT_BLUE, ICONO_EVENTOS);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    // Boton fijo arriba de la lista para exportar /resumen.txt con
    // TODA la sesion (no solo los eventos - ver exportarResumenSesion()
    // mas arriba) a la SD. Siempre visible, incluso sin eventos
    // todavia, porque el resumen tambien tiene datos de WiFi/
    // Wardriving que pueden existir igual. manejarToque(), caso
    // SCREEN_EVENTOS, usa estos mismos numeros para que el toque
    // coincida con lo dibujado.
    tft.setTextColor(COLOR_WARN, COLOR_BG);
    tft.setCursor(10, top + 2);
    tft.print("[Toca para exportar resumen .txt]");

    if (ultimoExportResumen < 0 && ultimoExportResumen != -2) {
        tft.setTextColor(COLOR_DANGER, COLOR_BG);
        tft.setCursor(10, top + 14);
        tft.print("Error: la SD no respondio");
    } else if (ultimoExportResumen >= 0) {
        tft.setTextColor(COLOR_CAT_BLUE, COLOR_BG);
        tft.setCursor(10, top + 14);
        tft.print("resumen.txt: ");
        tft.print(ultimoExportResumen);
        tft.print(" lineas");
    }

    tft.drawFastHLine(10, top + ALTO_BOTON_EXPORTAR, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);
    int top2 = top + ALTO_BOTON_EXPORTAR + 4;

    int total = 0;
    EventoLog* lista = eventos.getEventos(total);

    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top2 + 10);
        tft.print("Todavia no paso nada en esta sesion");
        dibujarPieAyuda("El historial empieza al encender");
        return;
    }

    // El mas reciente ya queda primero en el array (ver EventLog::
    // registrar), asi que aca alcanza con recorrerlo en orden.
    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_BOTON_EXPORTAR - ALTO_PIE_AYUDA) / 34;
    int yOffset = top2 + 2;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        EventoLog& e = lista[i];

        // Icono chico (el mismo reloj de la fila "EVENTOS" del menu)
        // a la izquierda de cada entrada del historial.
        tft.drawBitmap(5, yOffset + 2, ICONO_EVENTOS, ICONO_ANCHO, ICONO_ALTO, COLOR_CAT_BLUE, COLOR_BG);
        int xTexto = 5 + ICONO_ANCHO + 4;

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(xTexto, yOffset);
        tft.print(e.timestamp);

        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(xTexto, yOffset + 16);
        tft.print(e.texto);

        yOffset += 34;
    }

    if (total > mostradas) {
        char masTexto[32];
        snprintf(masTexto, sizeof(masTexto), "+ %d mas viejos", total - mostradas);
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("El mas reciente arriba de todo");
    }
}

void dibujarPantallaMonitor() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("MONITOR", COLOR_CAT_RED, ICONO_MONITOR);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    unsigned long totalDeauth = 0, totalDisassoc = 0;
    scanner.getAnomaliasStats(totalDeauth, totalDisassoc);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(5, top);
    tft.print("Deauth: ");
    tft.setTextColor(totalDeauth > 0 ? COLOR_DANGER : COLOR_TEXT, COLOR_BG);
    tft.print(totalDeauth);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(TFT_PANEL_WIDTH / 2 + 5, top);
    tft.print("Disassoc: ");
    tft.setTextColor(totalDisassoc > 0 ? COLOR_WARN : COLOR_TEXT, COLOR_BG);
    tft.print(totalDisassoc);

    tft.drawFastHLine(5, top + 18, TFT_PANEL_WIDTH - 10, COLOR_ACCENT_DIM);

    // Captura cruda a .pcap (beacons, probe req/resp, deauth y
    // disassoc - ver WiFiScanner::activarPCAP()/pcapBuffer). Tocar
    // esta franja prende o apaga la grabacion - manejarToque(), caso
    // SCREEN_MONITOR, usa el mismo ALTO_BLOQUE_MONITOR para que el
    // toque coincida con lo dibujado aca.
    bool pcapOn = scanner.pcapEstaActivo();
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(5, top + 22);
    tft.print("PCAP: ");
    tft.setTextColor(pcapOn ? COLOR_DANGER : COLOR_TEXT_DIM, COLOR_BG);
    tft.print(pcapOn ? "grabando (toca p/parar)" : "apagado (toca p/grabar)");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(5, top + 34);
    tft.print("Paquetes guardados: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(scanner.getPCAPCapturados());

    tft.drawFastHLine(5, top + ALTO_BLOQUE_MONITOR, TFT_PANEL_WIDTH - 10, COLOR_ACCENT_DIM);

    int total = 0;
    AnomaliaWiFi* lista = scanner.getAnomalias(total);

    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + ALTO_BLOQUE_MONITOR + 10);
        tft.print("Nada detectado por ahora");
        dibujarPieAyuda("Pasivo: solo mira, no transmite nada");
        return;
    }

    // El canal del radio en este momento no se controla desde aca (lo
    // dejan donde haya quedado el ultimo scan o conteo de clientes),
    // asi que esta lista solo ve lo que pasa en ESE canal - no es un
    // monitor de toda la banda 2.4GHz a la vez. Igual sirve para
    // notar una rafaga de deauth mientras mirás esa red puntual.
    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_BLOQUE_MONITOR - ALTO_PIE_AYUDA) / 34;
    int yOffset = top + ALTO_BLOQUE_MONITOR + 4;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        AnomaliaWiFi& a = lista[i];
        unsigned long segundos = a.momento / 1000;
        char horaBuf[16];
        sprintf(horaBuf, "T+%02lu:%02lu:%02lu", segundos / 3600, (segundos % 3600) / 60, segundos % 60);

        // Icono de alerta (el mismo triangulo de la fila "MONITOR" del
        // menu) a la izquierda de cada anomalia - en amarillo para
        // disassoc, en rojo para deauth, igual que el texto de abajo.
        uint16_t colorFila = a.esDisassoc ? COLOR_WARN : COLOR_DANGER;
        tft.drawBitmap(5, yOffset + 2, ICONO_MONITOR, ICONO_ANCHO, ICONO_ALTO, colorFila, COLOR_BG);
        int xTexto = 5 + ICONO_ANCHO + 4;

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(xTexto, yOffset);
        tft.print(horaBuf);
        tft.print("  ch");
        tft.print(a.canal);

        tft.setTextColor(colorFila, COLOR_BG);
        tft.setCursor(xTexto, yOffset + 16);
        tft.print(a.esDisassoc ? "DISASSOC" : "DEAUTH");
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.print("  de ..");
        char macBuf[6];
        sprintf(macBuf, "%02X%02X", a.origen[4], a.origen[5]);
        tft.print(macBuf);

        yOffset += 34;
    }

    if (total > mostradas) {
        char masTexto[32];
        snprintf(masTexto, sizeof(masTexto), "+ %d mas", total - mostradas);
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("Pasivo: solo mira, no transmite nada");
    }
}

void dibujarListaRedes() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("ESCANEAR", COLOR_CAT_CYAN, ICONO_ESCANEAR);

    // Todo el contenido de esta pantalla arranca despues del titulo de
    // arriba (antes arrancaba pegado a CONTENT_TOP) - de ahi el "top"
    // en vez de CONTENT_TOP suelto en cada cursor de aca para abajo.
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    // Mientras hay un escaneo en curso no tiene sentido mostrar la
    // lista vieja por debajo del cartel de "Escaneando...": antes se
    // superponian (la lista anterior se quedaba dibujada y el cartel
    // le caia encima a mitad de camino). Se ve una cosa o la otra.
    if (escaneando) {
        tft.setTextColor(COLOR_WARN, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Escaneando...");
        dibujarPieAyuda("Esperando resultados...");
        return;
    }

    // Conteo de clientes de una red puntual en curso: antes
    // scanner.contarClientes() bloqueaba el firmware entero 3
    // segundos enteros con un while+delay(). Ahora es no bloqueante
    // (ver WiFiScanner::update()) y mientras dura se muestra esto en
    // vez de la lista, con la cuenta regresiva real.
    if (scanner.conteoEnProgreso()) {
        tft.setTextColor(COLOR_WARN, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Escuchando clientes de");
        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(10, top + 30);
        if (redIndiceConteo >= 0 && redIndiceConteo < numRedes) {
            tft.print(redes[redIndiceConteo].ssid);
        }
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 50);
        tft.print((scanner.tiempoRestanteConteo() / 1000) + 1);
        tft.print("s restantes");
        dibujarPieAyuda("Pasivo: solo escucha, no se conecta");
        return;
    }

    // Resultado del ultimo conteo de clientes: se queda mostrado
    // hasta que se toque de nuevo (vuelve a la lista) o se pida un
    // escaneo nuevo.
    if (redIndiceConteo >= 0 && redIndiceConteo < numRedes) {
        tft.setTextColor(COLOR_CAT_CYAN, COLOR_BG);
        tft.setCursor(5, top - 6);
        tft.print("Clientes de ");
        tft.print(redes[redIndiceConteo].ssid);

        int total = 0;
        ClienteInfo* lista = scanner.getClientes(total);

        if (total == 0) {
            tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
            tft.setCursor(10, top + 16);
            tft.print("Nadie hablando con esta red ahora");
        } else {
            int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - 36 - ALTO_PIE_AYUDA) / 18;
            int yOffset = top + 14;
            int mostradas = (total < maxVisibles) ? total : maxVisibles;
            for (int i = 0; i < mostradas; i++) {
                ClienteInfo& c = lista[i];
                tft.setTextColor(COLOR_TEXT, COLOR_BG);
                tft.setCursor(5, yOffset);
                tft.print(c.fabricante);

                tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
                tft.setCursor(TFT_PANEL_WIDTH - 60, yOffset);
                tft.print(c.rssi);
                tft.print("dBm");
                yOffset += 18;
            }
            if (total > mostradas) {
                tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
                tft.setCursor(5, yOffset);
                tft.print("+ ");
                tft.print(total - mostradas);
                tft.print(" mas");
            }
        }

        dibujarPieAyuda("Toca para volver a la lista");
        return;
    }

    int yOffset = top + 6;
    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / 20;

    for (int i = 0; i < numRedes && i < maxVisibles; i++) {
        // Icono de señal (el mismo de la fila "ESCANEAR" del menu) a
        // la izquierda de cada red - en amarillo si es abierta (sin
        // clave), en cian si tiene clave, calcado del resto de listas
        // de esta pantalla. El numero suelto de antes ("1.", "2.") ya
        // no hace falta: el icono cumple el mismo rol de marcador.
        uint16_t colorFila = redes[i].tieneClave ? COLOR_CAT_CYAN : COLOR_WARN;
        tft.drawBitmap(5, yOffset - 1, ICONO_ESCANEAR, ICONO_ANCHO, ICONO_ALTO, colorFila, COLOR_BG);
        int xTexto = 5 + ICONO_ANCHO + 4;

        tft.setTextColor(colorFila, COLOR_BG);
        tft.setCursor(xTexto, yOffset);
        tft.print(strlen(redes[i].ssid) == 0 ? "*OCULTA*" : redes[i].ssid);

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(TFT_PANEL_WIDTH - 70, yOffset);
        tft.print(redes[i].rssi);
        tft.print("dBm");
        yOffset += 20;
    }

    if (numRedes == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Toca aca para escanear");
        dibujarPieAyuda("Pasivo: no se conecta a ninguna red");
    } else {
        dibujarPieAyuda("Toca una red para ver sus clientes");
    }
}

// Cuenta, canal por canal (1 a 13 - el rango de 2.4GHz en casi todo el
// mundo), cuantas redes del ULTIMO escaneo (redes[]/numRedes, los
// mismos que llena dibujarListaRedes()) hay en cada uno, y lo muestra
// como un grafico de barras horizontales. Util para elegir un canal
// poco usado al configurar un router propio - no hace falta ningun
// dato nuevo, son los mismos que ya guarda cada escaneo.
void dibujarPantallaCanales() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CANALES", COLOR_CAT_YELLOW, ICONO_CANALES);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    if (numRedes == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Hace falta un escaneo primero");
        dibujarPieAyuda("Anda a ESCANEAR y volve aca");
        return;
    }

    int conteo[13] = {0};
    int maxConteo = 1; // nunca 0, para no dividir por 0 mas abajo
    for (int i = 0; i < numRedes; i++) {
        int ch = redes[i].canal;
        if (ch >= 1 && ch <= 13) {
            conteo[ch - 1]++;
            if (conteo[ch - 1] > maxConteo) maxConteo = conteo[ch - 1];
        }
    }

    const int FILAS = 13;
    int altoFila = (alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / FILAS;
    if (altoFila < 10) altoFila = 10; // las 13 filas entran apretadas igual en pantallas chicas
    int xBarra = 38;
    int anchoMaxBarra = TFT_PANEL_WIDTH - xBarra - 35; // deja lugar al numero de la derecha

    int y = top + 2;
    for (int ch = 1; ch <= 13; ch++) {
        int cant = conteo[ch - 1];
        int anchoBarra = (cant * anchoMaxBarra) / maxConteo;
        if (cant > 0 && anchoBarra < 2) anchoBarra = 2; // que se note aunque sea una sola red

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(2, y);
        tft.print("CH");
        tft.print(ch);

        tft.fillRect(xBarra, y, anchoMaxBarra, 8, COLOR_PANEL);
        if (anchoBarra > 0) {
            tft.fillRect(xBarra, y, anchoBarra, 8, COLOR_CAT_YELLOW);
        }

        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(xBarra + anchoMaxBarra + 4, y);
        tft.print(cant);

        y += altoFila;
    }

    // Estas barras son la foto del ULTIMO escaneo (manual o
    // automatico, ver AUTO_SCAN_MS en el bloque de loop() - por eso
    // esta pantalla ahora se actualiza sola cada 45s sin que haga
    // falta ir a ESCANEAR). El pie de abajo en cambio es el
    // ACUMULADO de todos los escaneos de la sesion, para contestar
    // "que canal esta mas saturado en general" y no solo en este
    // instante puntual.
    if (escaneosAcumulados > 1) {
        int chMasUsado = 1;
        for (int ch = 2; ch <= 13; ch++) {
            if (conteoAcumuladoCanales[ch - 1] > conteoAcumuladoCanales[chMasUsado - 1]) {
                chMasUsado = ch;
            }
        }
        char pie[40];
        snprintf(pie, sizeof(pie), "Historico (%d escaneos): CH%d el mas usado", escaneosAcumulados, chMasUsado);
        dibujarPieAyuda(pie);
    } else {
        dibujarPieAyuda("Barra mas corta = canal menos saturado");
    }
}

// Compara las redes del ULTIMO escaneo entre si (no contra el
// historial de Conocidas - un router nuevo o un mesh agregado despues
// darian falsos positivos ahi) y marca que SSID aparece con mas de un
// BSSID distinto EN EL MISMO escaneo. Esa es la señal pasiva clasica
// de un posible "gemelo malvado": algo mas transmitiendo el mismo
// nombre de red que una real. No es una certeza (una red mesh real de
// varios routers con el mismo nombre da el mismo patron) - por eso el
// pie de esta pantalla aclara que es una pista, no una acusacion.
void dibujarPantallaDuplicados() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("DUPLICADO", COLOR_CAT_RED, ICONO_DUPLICADO);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    if (numRedes == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Hace falta un escaneo primero");
        dibujarPieAyuda("Anda a ESCANEAR y volve aca");
        return;
    }

    // Las redes ocultas (SSID vacio) se ignoran a proposito: entre
    // ellas serian "duplicados" sin decir nada real, total ninguna
    // tiene nombre que comparar.
    bool sospechosa[MAX_REDES_ESCANEO] = {false};
    int totalSospechosas = 0;
    for (int i = 0; i < numRedes; i++) {
        if (strlen(redes[i].ssid) == 0) continue;
        for (int j = i + 1; j < numRedes; j++) {
            if (strlen(redes[j].ssid) == 0) continue;
            if (strcmp(redes[i].ssid, redes[j].ssid) == 0 &&
                memcmp(redes[i].bssid, redes[j].bssid, 6) != 0) {
                if (!sospechosa[i]) { sospechosa[i] = true; totalSospechosas++; }
                if (!sospechosa[j]) { sospechosa[j] = true; totalSospechosas++; }
            }
        }
    }

    if (totalSospechosas == 0) {
        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Ningun SSID repetido en este escaneo");
        dibujarPieAyuda("Puede cambiar en el proximo escaneo");
        return;
    }

    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / 34;
    int yOffset = top + 2;
    int mostradas = 0;

    for (int i = 0; i < numRedes && mostradas < maxVisibles; i++) {
        if (!sospechosa[i]) continue;

        tft.drawBitmap(5, yOffset + 2, ICONO_DUPLICADO, ICONO_ANCHO, ICONO_ALTO, COLOR_DANGER, COLOR_BG);
        int xTexto = 5 + ICONO_ANCHO + 4;

        tft.setTextColor(COLOR_DANGER, COLOR_BG);
        tft.setCursor(xTexto, yOffset);
        tft.print(redes[i].ssid);

        char macBuf[20];
        sprintf(macBuf, "..%02X%02X  ch%d", redes[i].bssid[4], redes[i].bssid[5], redes[i].canal);
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(xTexto, yOffset + 16);
        tft.print(macBuf);

        yOffset += 34;
        mostradas++;
    }

    if (totalSospechosas > mostradas) {
        char masTexto[36];
        snprintf(masTexto, sizeof(masTexto), "+ %d mas con el mismo problema", totalSospechosas - mostradas);
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("Puede ser mesh real - no es 100% certeza");
    }
}

// RADAR: lista en vivo de dispositivos BLE escuchados (ver el
// comentario largo en BLEScanner.h de que detecta esto de verdad y
// que no). Se repinta sola cada 1s mientras estamos parados en esta
// pantalla (igual que SCREEN_MONITOR en loop()), porque a diferencia
// de ESCANEAR esto no es algo que se "dispare" con un boton - el
// radar ya esta escuchando de fondo desde que arranco el firmware.
// Cuanto tarda el barrido en dar una vuelta completa (ver
// dibujarPantallaRadar) y cada cuanto se repinta la pantalla mientras
// estamos parados en ella (ver el bloque de refresco en loop(), caso
// SCREEN_RADAR) - mas chico el segundo numero = barrido mas fluido,
// a costa de redibujar mas seguido.
const unsigned long RADAR_BARRIDO_MS = 4000;
const unsigned long RADAR_REFRESCO_MS = 150;

// Angulo "de pantalla" fijo para un dispositivo, sacado de su MAC.
// OJO, esto es puramente visual, NO es la direccion real hacia donde
// esta el dispositivo: ni el ESP32 ni una antena BLE comun como la de
// esta placa pueden saber de que lado viene una señal, solo que tan
// fuerte llega. Sin esto cada dispositivo aparecería siempre pegado
// al centro en el mismo punto (0 grados), lo que se superpondría con
// cualquier otro y además no se vería nada parecido a un radar. Lo
// unico que SI es un dato real es la distancia al centro (mas abajo),
// que sale de la señal (RSSI).
int anguloVisualBLE(const uint8_t* mac) {
    return (mac[2] * 7 + mac[3] * 13 + mac[4] * 19 + mac[5] * 29) % 360;
}

// RADAR: barrido circular en vivo (ver el comentario largo en
// BLEScanner.h de que detecta esto de verdad y que no - sigue
// aplicando igual aca, esto solo cambio como se MUESTRA el mismo
// dato). Centro = señal fuerte (dispositivo "cerca"), borde = señal
// debil ("lejos") - la distancia al centro sale del RSSI real de
// cada dispositivo. El angulo alrededor del centro NO es la
// direccion real (ver anguloVisualBLE) - es nada mas para que cada
// dispositivo tenga su propio lugar fijo en pantalla y no se pisen
// todos en el mismo punto.
void dibujarPantallaRadar() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("RADAR", COLOR_CAT_GREEN, ICONO_RADAR);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    int altoDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int cx = TFT_PANEL_WIDTH / 2;
    int cy = top + altoDisponible / 2;
    int radio = min(TFT_PANEL_WIDTH / 2 - 12, altoDisponible / 2 - 6);

    // Anillos de referencia + cruz del medio - la "grilla" clasica de
    // un radar, puramente decorativa.
    tft.drawCircle(cx, cy, radio, COLOR_PANEL);
    tft.drawCircle(cx, cy, (radio * 2) / 3, COLOR_PANEL);
    tft.drawCircle(cx, cy, radio / 3, COLOR_PANEL);
    tft.drawFastHLine(cx - radio, cy, radio * 2, COLOR_PANEL);
    tft.drawFastVLine(cx, cy - radio, radio * 2, COLOR_PANEL);

    // Barrido giratorio: una linea principal + dos mas atras (mas
    // oscuras) para que se note que "gira" en vez de ser una aguja
    // fija saltando de a tirones.
    float anguloBarrido = (millis() % RADAR_BARRIDO_MS) * 360.0 / RADAR_BARRIDO_MS;
    for (int i = 0; i < 3; i++) {
        float ang = radians(anguloBarrido - i * 10);
        uint16_t colorLinea = (i == 0) ? COLOR_CAT_GREEN : COLOR_ACCENT_DIM;
        int x2 = cx + (int)(cos(ang) * radio);
        int y2 = cy + (int)(sin(ang) * radio);
        tft.drawLine(cx, cy, x2, y2, colorLinea);
    }

    int numDisp = 0;
    DispositivoBLE* dispositivos = radar.getDispositivos(numDisp);
    unsigned long ahora = millis();
    int rssiMasFuerte = -999;

    for (int i = 0; i < numDisp; i++) {
        DispositivoBLE& d = dispositivos[i];
        if (ahora - d.ultimaVez > 30000) continue; // mas de 30s sin escucharlo: no lo pintamos mas

        // RSSI tipico de BLE anda entre -30 (pegado) y -95 (al limite
        // del alcance) - fuera de ese rango lo recortamos para que un
        // pico raro no mande el punto afuera del circulo.
        int rssi = d.rssi;
        if (rssi > -30) rssi = -30;
        if (rssi < -95) rssi = -95;
        float t = (float)(rssi - (-30)) / ((-95) - (-30)); // 0 = cerca, 1 = lejos
        int distancia = (int)(t * radio);

        float ang = radians(anguloVisualBLE(d.mac));
        int x = cx + (int)(cos(ang) * distancia);
        int y = cy + (int)(sin(ang) * distancia);

        uint16_t color = d.esFindMyApple ? COLOR_DANGER : COLOR_CAT_GREEN;
        tft.fillCircle(x, y, 3, color);
        if (d.esFindMyApple) {
            tft.drawCircle(x, y, 6, COLOR_DANGER); // aro extra para que resalte entre los demas
        }

        if (rssi > rssiMasFuerte) rssiMasFuerte = rssi;
    }

    unsigned long totalVistos = 0, totalFindMy = 0;
    radar.getStats(totalVistos, totalFindMy);

    char pie[44];
    if (numDisp == 0) {
        snprintf(pie, sizeof(pie), "Buscando...");
    } else if (totalFindMy > 0) {
        snprintf(pie, sizeof(pie), "Rojo = red Find My (no es 100%% un AirTag)");
    } else {
        snprintf(pie, sizeof(pie), "%d cerca - centro = senial mas fuerte", numDisp);
    }
    dibujarPieAyuda(pie);
}

void dibujarPantallaStats() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("STATS", COLOR_CAT_YELLOW, ICONO_STATS);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 35);
    tft.print("Redes conocidas: ");
    tft.print(aprendizaje.getTotalRedesConocidas());

    tft.setCursor(10, CONTENT_TOP + 55);
    tft.print("Vistas este escaneo: ");
    tft.print(numRedes);

    tft.setCursor(10, CONTENT_TOP + 75);
    tft.print("Nuevas descubiertas: ");
    tft.print(mascota.getRedesNuevas());

    if (numRedes > 0) {
        RedStats* s = aprendizaje.getStats(redes[0].bssid);
        if (s) {
            tft.setCursor(10, CONTENT_TOP + 105);
            tft.setTextColor(COLOR_WARN, COLOR_BG);
            tft.print("Ultima red top: ");
            tft.print(s->ssid);
            tft.setCursor(10, CONTENT_TOP + 125);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.print("Vista ");
            tft.print(s->vecesVista);
            tft.print(" veces");
        }
    }

    dibujarPieAyuda("Se actualiza con cada escaneo");
}

void dibujarPantallaWardrive() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("WARDRIVE", COLOR_CAT_VIOLET, ICONO_WARDRIVE);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 35);
    tft.print("GPS: ");
    GPSData pos = gps.getData();
    tft.setTextColor(pos.valid ? COLOR_CAT_VIOLET : COLOR_WARN, COLOR_BG);
    tft.print(pos.valid ? "FIX" : "SIN FIX");

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 55);
    tft.print("Satelites: ");
    tft.print(pos.satellites);

    tft.setCursor(10, CONTENT_TOP + 75);
    tft.print("Redes guardadas: ");
    // Antes esto releia wardriving.csv entero byte por byte Y
    // remontaba la SD cada vez que se dibujaba esta pantalla - con un
    // CSV grande de varias sesiones se iba a poner lento, y de paso
    // invalidaba cualquier File que otro modulo tuviera abierto de
    // antes (ver el arreglo en Wardriving.cpp/EventLog.cpp). Ahora
    // Wardriving ya lleva la cuenta el solo, sin releer nada.
    tft.print(wardriver.getContadorRedes());

    // Filtro de que se guarda: se cicla tocando esta linea. Se guarda
    // en RAM nomas (no en la SD), asi que vuelve a "Todas" en cada
    // reinicio - es una decision de la sesion actual, no una
    // preferencia permanente del dispositivo.
    tft.setCursor(10, CONTENT_TOP + 95);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print("Filtro [toca]: ");
    tft.setTextColor(COLOR_CAT_VIOLET, COLOR_BG);
    if (filtroWardrive == 0) {
        tft.print("Todas");
    } else if (filtroWardrive == 1) {
        tft.print("Solo abiertas");
    } else {
        tft.print("Solo con clave");
    }

    tft.setCursor(10, CONTENT_TOP + 120);
    tft.setTextColor(COLOR_WARN, COLOR_BG);
    tft.print("[Toca para exportar KML]");

    // Resultado del ultimo export, para que tocar el boton de arriba
    // no se sienta como que no paso nada - antes exportKML() no hacia
    // nada real, asi que tampoco habia nada que mostrar aca.
    tft.setCursor(10, CONTENT_TOP + 145);
    if (ultimoExportKML == -2) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.print("(todavia no exportaste nada)");
    } else if (ultimoExportKML < 0) {
        tft.setTextColor(COLOR_DANGER, COLOR_BG);
        tft.print("Error: sin SD o sin datos");
    } else {
        tft.setTextColor(ultimoExportKML > 0 ? COLOR_CAT_VIOLET : COLOR_WARN, COLOR_BG);
        tft.print("wardriving.kml: ");
        tft.print(ultimoExportKML);
        tft.print(" puntos");
    }

    dibujarPieAyuda("Guarda cada red vista, con GPS si hay fix");

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        touchDriver.reclaimBus();
    #endif
}

// Repinta solo los numeros de diagnostico que cambian con el tiempo
// (heap libre, heap minimo, uptime). A proposito NO toca la SD ni
// nada del resto de la pantalla: esto se llama una vez por segundo
// mientras estamos parados en Config, y si remontara la SD en cada
// llamada volveriamos al mismo problema de lentitud que ya arreglamos
// en Wardrive/Stats (remount de SD disparado por un redibujado que
// en realidad no necesitaba tocarla).
void actualizarDiagnosticoHardware() {
    const int yBloque = CONTENT_TOP + 136;
    // Alto fijo para las 3 lineas (heap libre, heap minimo, uptime);
    // se limpia entero antes de reescribir por si un numero se achica
    // de 3 a 2 digitos y deja basura del dibujo anterior.
    tft.fillRect(0, yBloque - 6, TFT_PANEL_WIDTH, 66, COLOR_BG);

    uint32_t heapLibre = ESP.getFreeHeap() / 1024;
    uint32_t heapMinimo = ESP.getMinFreeHeap() / 1024;

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yBloque);
    tft.print("Heap libre: ");
    // Por debajo de 20KB el ESP32 empieza a fallar cosas como el WiFi
    // scan o el TLS, asi que marco en rojo cuando se acerca a eso en
    // vez de esperar a que algo explote para enterarme.
    tft.setTextColor(heapLibre > 20 ? COLOR_TEXT : COLOR_DANGER, COLOR_BG);
    tft.print(heapLibre);
    tft.print(" KB");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yBloque + 20);
    tft.print("Heap minimo: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(heapMinimo);
    tft.print(" KB");

    unsigned long segundos = millis() / 1000;
    int horas = segundos / 3600;
    int minutos = (segundos % 3600) / 60;
    int segs = segundos % 60;

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yBloque + 40);
    tft.print("Encendido: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    char uptimeBuf[16];
    sprintf(uptimeBuf, "%02d:%02d:%02d", horas, minutos, segs);
    tft.print(uptimeBuf);
}

// Vuelca a /resumen.txt en la SD un reporte legible de TODA la sesion
// (no solo lo que entra en una pantalla puntual): cuanto hace que esta
// encendido, cuantas redes vio y aprendio, cuantas anomalias de
// deauth/disassoc detecto, cuantas anoto Wardriving, y el historial
// de eventos completo - todo en texto plano para abrir en cualquier
// lado sin tener que parsear wardriving.csv ni eventos.csv a mano.
// Mismo patron de SD.end()/SD.begin() que ya usa Wardriving::
// exportKML() (ver el comentario largo ahi): abre, escribe y cierra
// en esta misma llamada, nunca deja un File colgado para la proxima.
// Devuelve cuantas lineas escribio, o -1 si la SD no estaba
// disponible.
int exportarResumenSesion() {
    SD.end();
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("No se pudo montar la SD para el resumen");
        return -1;
    }

    File f = SD.open("/resumen.txt", FILE_WRITE);
    if (!f) {
        Serial.println("No se pudo crear resumen.txt");
        return -1;
    }

    int lineas = 0;
    unsigned long seg = millis() / 1000;
    f.println("=== Resumen de sesion - deauth-chan ===");
    f.printf("Tiempo encendido: %02lu:%02lu:%02lu\n", seg / 3600, (seg % 3600) / 60, seg % 60);
    lineas += 2;

    f.println();
    f.println("-- WiFi --");
    f.printf("Redes vistas en el ultimo escaneo: %d\n", numRedes);
    f.printf("Redes distintas aprendidas (esta sesion y anteriores): %d\n", aprendizaje.getTotalRedesConocidas());
    f.printf("Redes nuevas descubiertas en esta sesion: %d\n", mascota.getRedesNuevas());
    lineas += 4;

    unsigned long deauth = 0, disassoc = 0;
    scanner.getAnomaliasStats(deauth, disassoc);
    f.println();
    f.println("-- Monitor pasivo --");
    f.printf("Paquetes deauth detectados: %lu\n", deauth);
    f.printf("Paquetes disassoc detectados: %lu\n", disassoc);
    lineas += 3;

    f.println();
    f.println("-- Wardriving --");
    f.printf("Redes anotadas en wardriving.csv: %d\n", wardriver.getContadorRedes());
    lineas += 2;

    f.println();
    f.println("-- Historial de eventos de esta sesion --");
    int totalEventos = 0;
    EventoLog* listaEventos = eventos.getEventos(totalEventos);
    if (totalEventos == 0) {
        f.println("(sin eventos)");
        lineas++;
    } else {
        // El mas reciente queda primero en el array (ver EventLog::
        // registrar) - lo volcamos en ese mismo orden.
        for (int i = 0; i < totalEventos; i++) {
            f.printf("%s  %s\n", listaEventos[i].timestamp, listaEventos[i].texto);
            lineas++;
        }
    }

    f.close();
    Serial.printf("resumen.txt generado (%d lineas)\n", lineas);
    return lineas;
}

void dibujarPantallaConfig() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CONFIG", COLOR_CAT_BLUE, ICONO_CONFIG);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 35);
    tft.print("Placa: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(BOARD_NAME);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 55);
    tft.print("Pantalla: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(TFT_PANEL_WIDTH);
    tft.print("x");
    tft.print(TFT_PANEL_HEIGHT);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 75);
    tft.print("Touch: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(TOUCH_IS_CAPACITIVE ? "Capacitivo" : "Resistivo");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 95);
    tft.print("SD: ");
    SD.end();
    bool sdOk = SD.begin(SD_CS_PIN);
    tft.setTextColor(sdOk ? COLOR_CAT_BLUE : COLOR_DANGER, COLOR_BG);
    tft.print(sdOk ? "OK" : "No detectada");

    // La frecuencia de CPU no cambia sola en tiempo de ejecucion (no
    // llamamos a setCpuFrequencyMhz en ningun lado), asi que va junto
    // con los datos "de una vez" de arriba y no en el bloque que se
    // refresca cada segundo.
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 115);
    tft.print("CPU: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(ESP.getCpuFreqMHz());
    tft.print(" MHz");

    // Diagnostico de hardware: nada de esto pide libreria nueva, son
    // funciones que ya trae el core de Arduino-ESP32. Sirve sobre
    // todo para notar si alguna pantalla nueva se come memoria de mas
    // y no la devuelve (heap minimo bajando con el tiempo sin fondo).
    // Los numeros en si se pintan aparte (actualizarDiagnosticoHardware)
    // porque ese bloque se repinta solo, una vez por segundo, mientras
    // el resto de esta pantalla (lo de arriba) se queda quieto.
    tft.drawFastHLine(10, CONTENT_TOP + 132, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);
    actualizarDiagnosticoHardware();

    // Esto se dibuja una sola vez, al entrar a la pantalla - no hace
    // falta repetirlo en actualizarDiagnosticoHardware() (que se llama
    // aparte cada 1s) porque el texto no cambia nunca.
    dibujarPieAyuda("Info de la placa y el estado de la SD");

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        touchDriver.reclaimBus();
    #endif
}

void manejarToque(int x, int y) {
    for (int i = 0; i < 2; i++) {
        if (x >= menuBotones[i].x && x <= menuBotones[i].x + menuBotones[i].w &&
            y >= menuBotones[i].y && y <= menuBotones[i].y + menuBotones[i].h) {
            // Boton 0 = volver a la mascota, boton 1 = ir a la lista
            // de herramientas (no a la ultima herramienta que estaba
            // abierta - siempre arranca de la lista, asi es predecible).
            pantallaActual = (i == 0) ? SCREEN_MAIN : SCREEN_TOOLS;
            return;
        }
    }

    switch (pantallaActual) {
        case SCREEN_TOOLS:
            for (int i = 0; i < CANTIDAD_CATEGORIAS; i++) {
                Categoria& c = categorias[i];
                if (y >= c.y && y <= c.y + c.h) {
                    // No navegamos todavia: primero se ve el destello
                    // (fila invertida) como feedback de que el toque se
                    // registro, y recien cuando pasa DESTELLO_MS (ver
                    // loop()) se entra de verdad al submenu de esta
                    // categoria. categoriaSeleccionada se fija ya
                    // mismo (no cuando termina el destello) porque
                    // dibujarPantallaHerramientasSub() lo necesita
                    // desde el primer frame en que se dibuja.
                    filaDestellada = i;
                    momentoDestello = millis();
                    destinoPendiente = SCREEN_TOOLS_SUB;
                    categoriaSeleccionada = i;
                    contenidoSucio = true;
                    return;
                }
            }
            break;
        case SCREEN_TOOLS_SUB: {
            // Misma logica de destello que SCREEN_TOOLS, pero sobre la
            // lista FILTRADA de esta categoria - ver el comentario de
            // layoutHerramientasDeCategoria() para por que se arma de
            // nuevo aca en vez de leer una cacheada. filaDestellada
            // guarda la POSICION dentro de esta lista filtrada (0..k),
            // no el indice global en herramientas[] - coincide con lo
            // que espera dibujarPantallaHerramientasSub().
            int indices[CANTIDAD_HERRAMIENTAS];
            int cantidad = layoutHerramientasDeCategoria(categoriaSeleccionada, indices);
            for (int k = 0; k < cantidad; k++) {
                Herramienta& h = herramientas[indices[k]];
                if (y >= h.y && y <= h.y + h.h) {
                    filaDestellada = k;
                    momentoDestello = millis();
                    destinoPendiente = h.destino;
                    contenidoSucio = true;
                    return;
                }
            }
            break;
        }
        case SCREEN_SCAN: {
            // Mientras hay un escaneo o un conteo de clientes en
            // curso, un toque no hace nada - hay que esperar a que
            // termine (el conteo ya no bloquea el firmware mientras
            // tanto, asi que el resto de la interfaz sigue andando).
            if (escaneando || scanner.conteoEnProgreso()) break;

            if (redIndiceConteo >= 0) {
                // Se esta mostrando el resultado de un conteo: un
                // toque en cualquier lado vuelve a la lista de redes,
                // en vez de interpretar la posicion como si fuera una
                // fila de esa lista (son datos distintos en el mismo
                // lugar de la pantalla).
                redIndiceConteo = -1;
                contenidoSucio = true;
                break;
            }

            // Mismos numeros que usa dibujarListaRedes() para pintar la
            // lista (top + 6, filas de 20px) - si alguno de los dos
            // cambia sin el otro, el toque deja de coincidir con lo que
            // se ve en pantalla.
            int primeraFilaY = CONTENT_TOP + ALTO_TITULO_SECCION + 6;
            int filaAlto = 20;
            int indiceFila = (y - primeraFilaY) / filaAlto;
            bool filaValida = (y >= primeraFilaY) && indiceFila >= 0 &&
                indiceFila < numRedes &&
                indiceFila < ((alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / filaAlto);

            if (filaValida) {
                // Toco una red puntual: arranca el conteo pasivo de
                // clientes para esa red (no bloqueante).
                scanner.iniciarConteoClientes(redes[indiceFila].bssid, redes[indiceFila].canal);
                redIndiceConteo = indiceFila;
                contenidoSucio = true;
            } else if (y > CONTENT_TOP && y < CONTENT_TOP + alturaListado()) {
                // Toco el espacio vacio debajo de la lista (o no hay
                // lista todavia): pide un escaneo nuevo. redIndiceConteo
                // ya esta en -1 en esta rama (si no, hubiera entrado en
                // el "if" de arriba), pero un escaneo nuevo tambien
                // deja vieja cualquier referencia a redes[] anterior.
                mascota.setEstado(ESTADO_SCANNING);
                scanner.startScan();
                escaneando = true;
                contenidoSucio = true; // para que se vea "Escaneando..." ya mismo
            }
            break;
        }
        case SCREEN_WARDRIVE:
            if (y > CONTENT_TOP + 90 && y < CONTENT_TOP + 110) {
                // Cicla entre los 3 modos. El filtro solo distingue
                // "tiene clave si o no" porque es lo unico que guarda
                // RedInfo (tieneClave es un bool) - no hay forma de
                // pedir "solo WPA2" sin guardar tambien el modo exacto
                // de cada red, que hoy no se guarda en ningun lado.
                filtroWardrive = (filtroWardrive + 1) % 3;
                if (filtroWardrive == 0) {
                    wardriver.setFilterByEncryption(false, WIFI_AUTH_OPEN);
                } else if (filtroWardrive == 1) {
                    wardriver.setFilterByEncryption(true, WIFI_AUTH_OPEN);
                } else {
                    wardriver.setFilterByEncryption(true, WIFI_AUTH_WPA2_PSK);
                }
                contenidoSucio = true;
            } else if (y > CONTENT_TOP + 118 && y < CONTENT_TOP + 140) {
                ultimoExportKML = wardriver.exportKML();
                #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
                    // exportKML() remonta la SD (lee el csv y escribe
                    // el kml), asi que hay que reclamar el bus para el
                    // touch antes de que se lea el proximo toque.
                    touchDriver.reclaimBus();
                #endif
                contenidoSucio = true; // para mostrar el resultado ya mismo
            }
            break;
        case SCREEN_CONOCIDAS: {
            // Mismos numeros que dibujarPantallaConocidas() para su
            // renglon fijo de "Orden: X" - ver ALTO_BOTON_ORDEN mas
            // arriba. Tocar en cualquier lado de esa franja pasa al
            // siguiente criterio (ciclico: senial -> vistas -> nombre
            // -> de nuevo senial).
            int top = CONTENT_TOP + ALTO_TITULO_SECCION;
            if (y >= top && y <= top + ALTO_BOTON_ORDEN) {
                ordenConocidas = (ordenConocidas + 1) % 3;
                contenidoSucio = true;
            }
            break;
        }
        case SCREEN_EVENTOS: {
            // Mismos numeros que dibujarPantallaEventos() para su
            // boton fijo de "exportar resumen" - ver el comentario de
            // ALTO_BOTON_EXPORTAR mas arriba.
            int top = CONTENT_TOP + ALTO_TITULO_SECCION;
            if (y >= top && y <= top + ALTO_BOTON_EXPORTAR) {
                ultimoExportResumen = exportarResumenSesion();
                #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
                    // exportarResumenSesion() remonta la SD, igual que
                    // exportKML() de Wardrive - mismo motivo para
                    // reclamar el bus aca.
                    touchDriver.reclaimBus();
                #endif
                contenidoSucio = true; // para mostrar el resultado ya mismo
            }
            break;
        }
        case SCREEN_MONITOR: {
            // Mismos numeros que dibujarPantallaMonitor() para su
            // franja de PCAP - ver el comentario de ALTO_BLOQUE_MONITOR
            // mas arriba. Arranca en top+18 (despues del primer
            // divisor, el de los contadores) para no confundirse con
            // un toque en la zona de Deauth/Disassoc, que no hace nada.
            int top = CONTENT_TOP + ALTO_TITULO_SECCION;
            if (y >= top + 18 && y <= top + ALTO_BLOQUE_MONITOR) {
                bool nuevoEstado = !scanner.pcapEstaActivo();
                scanner.activarPCAP(nuevoEstado);
                eventos.registrar(nuevoEstado ? "PCAP: grabacion iniciada" : "PCAP: grabacion detenida");
                #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
                    // registrar() escribe a la SD - mismo motivo de
                    // siempre para reclamar el bus del touch despues.
                    touchDriver.reclaimBus();
                #endif
                contenidoSucio = true;
            }
            break;
        }
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("CYD Wardriver iniciando en %s\n", BOARD_NAME);

    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(COLOR_BG);

    touchDriver.begin();
    calcularLayoutMenu();
    calcularLayoutCategorias();
    mascota.init(&tft);
    ultimoToque = millis();

    if (pcap.begin("wardrive")) {
        Serial.println("PCAP listo");
    }

    // radar.begin() (Bluetooth) va ANTES que scanner.begin() (WiFi) a
    // proposito: los dos comparten el mismo radio 2.4GHz del ESP32, y
    // si WiFi ya lo prendio en modo promiscuo cuando Bluetooth intenta
    // inicializarse, esp_bt_controller_init() lo encuentra en un
    // estado que no puede manejar y aborta (ESP_ERR_INVALID_STATE,
    // visto en el log serie: "abort() was called" justo en
    // NimBLEDevice::init). Inicializar Bluetooth primero, con el radio
    // todavia "limpio", evita ese choque.
    radar.begin(); // RADAR: escaneo BLE pasivo, corre de fondo desde el arranque (ver BLEScanner.h)
    gps.begin();
    scanner.begin();
    wardriver.begin();
    aprendizaje.begin();
    eventos.begin();
    eventos.registrar("Inicio del sistema");

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        // pcap.begin(), wardriver.begin() y aprendizaje.begin() tocan la
        // SD, que comparte el periferico VSPI con el touch en esta
        // placa (ver el comentario largo en Board_2432S028.h). Eso deja
        // el bus apuntando a los pines de la SD en vez de los del
        // touch, asi que hay que reclamarlo antes de que arranque el
        // loop() o el touch queda leyendo basura desde el primer toque.
        touchDriver.reclaimBus();
    #endif

    Serial.println("Setup completado");
}

void loop() {
    gps.update();
    mascota.update();
    scanner.update(); // corta solo el conteo de clientes a los 3s (ver WiFiScanner::update)

    // Drena el buffer de paquetes crudos de PCAP (si hay algo activado
    // en Monitor) y recien ACA los escribe a la SD, via
    // PCAPWriter::writePacket() - nunca desde la tarea de WiFi, que es
    // donde se llenan (ver el comentario largo de pcapBuffer en
    // WiFiScanner.h sobre por que). Un paquete por vuelta de loop()
    // alcanza de sobra para no acumular atraso en uso normal.
    PaqueteCrudoWiFi paquetePCAP;
    if (scanner.sacarPaquetePCAP(paquetePCAP)) {
        pcap.writePacket(paquetePCAP.datos, paquetePCAP.len, paquetePCAP.ts_sec, paquetePCAP.ts_usec);
    }

    // Modo sleep: si nadie toco la pantalla en SLEEP_TIMEOUT_MS y no
    // hay un escaneo ni un conteo de clientes activo, apagamos el
    // backlight. No tocamos nada de WiFi/SD/GPS -- el resto de los
    // modulos sigue funcionando igual, lo unico que cambia es que la
    // pantalla deja de consumir.
    if (!escaneando && !scanner.conteoEnProgreso() && mascota.getEstado() != ESTADO_SLEEP &&
        millis() - ultimoToque > SLEEP_TIMEOUT_MS) {
        mascota.setEstado(ESTADO_SLEEP);
        digitalWrite(TFT_BL_PIN, LOW);
        eventos.registrar("Durmiendo por inactividad", nullptr);
        #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
            // registrar() acaba de escribir en la SD, que en esta
            // placa remapea el bus VSPI hacia sus propios pines (igual
            // que cualquier otra escritura a SD). Si no se reclama aca
            // el proximo touchDriver.read() de este mismo loop() lee
            // basura del touch.
            touchDriver.reclaimBus();
        #endif
    }

    // ANTES esto pedia ademas "pantallaActual == SCREEN_SCAN" - bug
    // real que encontre repasando el codigo: si tocabas "Herramientas"
    // (o cualquier otra pantalla) MIENTRAS un escaneo estaba en curso,
    // esta linea nunca se ejecutaba (porque ya no estabas en
    // SCREEN_SCAN), asi que "escaneando" se quedaba en true PARA
    // SIEMPRE - y el toque de "Anda a ESCANEAR y volve aca" quedaba
    // mudo de ahi en mas, porque manejarToque() no dispara un escaneo
    // nuevo mientras escaneando ya esta en true. Sacar la condicion de
    // pantalla de aca tambien es lo que hace falta para que el
    // escaneo automatico de mas abajo funcione estando en CUALQUIER
    // pantalla, no solo en ESCANEAR.
    if (escaneando && scanner.isScanDone()) {
        scanner.getScanResults(redes, MAX_REDES_ESCANEO, numRedes);
        mascota.setRedesEncontradas(numRedes);
        escaneando = false;
        // El escaneo automatico no muestra el estado "ESCANEANDO" (el
        // usuario no lo pidio, no hace falta avisarle que esta pasando
        // salvo que encuentre algo nuevo - eso ya lo hace
        // incrementarRedesNuevas() mas abajo, poniendo CURIOSA).
        if (!escaneoEsAutomatico) {
            mascota.setEstado(ESTADO_IDLE);
        }

        GPSData pos = gps.getData();
        int nuevasEnEsteScan = 0;
        for (int i = 0; i < numRedes; i++) {
            // Wardriving (guardar ubicacion+red en la SD) sigue siendo
            // una accion deliberada del usuario - un escaneo de fondo
            // cada 45s no deberia llenarle la SD de puntos que nunca
            // pidio. Aprendizaje (Conocidas/redes nuevas) SI se
            // actualiza siempre: ese historial tiene que reflejar todo
            // lo que el radio escucho, sea de un escaneo manual o
            // automatico.
            if (!escaneoEsAutomatico) {
                wardriver.saveNetwork(redes[i], pos);
            }
            bool esNueva = aprendizaje.registrarAvistamiento(redes[i]);
            if (esNueva) {
                mascota.incrementarRedesNuevas();
                nuevasEnEsteScan++;
            }

            int ch = redes[i].canal;
            if (ch >= 1 && ch <= 13) conteoAcumuladoCanales[ch - 1]++;
        }
        escaneosAcumulados++;

        // Una sola escritura a la SD aca afuera del for: antes
        // registrarAvistamiento() guardaba en SD por cada red
        // individual, asi que un escaneo con 15 redes eran 15
        // remontajes de SD seguidos. Guardando una vez al final se
        // llega al mismo resultado sin la espera.
        if (numRedes > 0) {
            aprendizaje.guardarEnSD();
        }

        // Un solo registro de evento por escaneo (no uno por red, por
        // la misma razon de arriba): deja el resumen que de verdad
        // importa - cuantas redes vio y cuantas eran nuevas - sin
        // volver a escribir la SD veinte veces seguidas. Se marca
        // "Auto" para distinguirlo de un escaneo que de verdad tocaste
        // vos, al mirar el historial despues.
        char resumenEvento[48];
        snprintf(resumenEvento, sizeof(resumenEvento), "%s: %d redes (%d nuevas)",
                 escaneoEsAutomatico ? "Auto-escaneo" : "Escaneo", numRedes, nuevasEnEsteScan);
        eventos.registrar(resumenEvento, &pos);

        Serial.printf("Escaneo completado: %d redes\n", numRedes);

        #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
            // La SD y el touch comparten el periferico VSPI en esta
            // placa (ver el comentario largo en Board_2432S028.h). Cada
            // escritura a SD de arriba remapea el bus hacia sus propios
            // pines, asi que hay que recuperarlo para el touch antes de
            // seguir escuchando toques.
            touchDriver.reclaimBus();
        #endif

        escaneoEsAutomatico = false;
        contenidoSucio = true; // hay resultados nuevos para mostrar (en SCAN, CANALES o DUPLICADO)
    }

    // Dispara el escaneo de fondo (ver comentario largo arriba de
    // AUTO_SCAN_MS): solo si no hay ya un escaneo o un conteo de
    // clientes usando el radio, no estamos parados en ESCANEAR (ahi
    // el usuario tiene el control manual) y la mascota no esta
    // dormida (no tiene sentido gastar radio con la pantalla apagada
    // por inactividad).
    if (!escaneando && !scanner.conteoEnProgreso() && pantallaActual != SCREEN_SCAN &&
        mascota.getEstado() != ESTADO_SLEEP &&
        millis() - ultimoAutoScan >= AUTO_SCAN_MS) {
        ultimoAutoScan = millis();
        scanner.startScan();
        escaneando = true;
        escaneoEsAutomatico = true;
    }

    // Conteo de clientes no bloqueante: mientras dura, se repinta la
    // cuenta regresiva cada medio segundo; apenas termina (deja de
    // estar en progreso de una vuelta del loop a la otra) se marca
    // contenidoSucio una sola vez para que se vea el resultado final.
    {
        static bool conteoEnProgresoAnterior = false;
        bool conteoAhora = scanner.conteoEnProgreso();
        if (conteoEnProgresoAnterior && !conteoAhora) {
            contenidoSucio = true;
        }
        conteoEnProgresoAnterior = conteoAhora;

        if (pantallaActual == SCREEN_SCAN && conteoAhora) {
            static unsigned long ultimoRefrescoConteo = 0;
            unsigned long ahoraConteo = millis();
            if (ahoraConteo - ultimoRefrescoConteo >= 500) {
                ultimoRefrescoConteo = ahoraConteo;
                dibujarListaRedes();
            }
        }
    }

    // Si hay un destello de toque pendiente en Herramientas, se espera
    // sin bloquear (nada de delay()) a que pasen los DESTELLO_MS antes
    // de entrar de verdad a la pantalla elegida - asi da tiempo a que
    // se vea la fila invertida en vez de saltar directo.
    if (filaDestellada >= 0 && millis() - momentoDestello >= DESTELLO_MS) {
        filaDestellada = -1;
        pantallaActual = destinoPendiente;
        contenidoSucio = true;
    }

    static MenuScreen pantallaAnterior = (MenuScreen)-1;
    bool cambioPantalla = (pantallaActual != pantallaAnterior);
    if (cambioPantalla) {
        contenidoSucio = true;
    }

    // La mascota se dibuja en la misma franja central que usan las
    // demas pantallas para su contenido (lista de redes, stats,
    // wardriving, config): no son dos capas separadas, es el mismo
    // pedazo de pantalla. Por eso antes, cuando a la mascota le tocaba
    // parpadear, se volvia a disparar TODO el bloque de abajo sin
    // importar en que pantalla estuvieramos - y en Wardrive/Config eso
    // significaba remontar la SD en cada parpadeo (de ahi la lentitud
    // y el parpadeo en esas pantallas, no solo en la principal). Ahora
    // la mascota solo se dibuja a si misma, y solo mientras estamos
    // parados en la pantalla principal; el resto de las pantallas se
    // redibuja una sola vez, cuando entras a ellas o cuando cambia
    // algo que de verdad hay que mostrar (como un escaneo terminado).
    bool enPantallaPrincipal = (pantallaActual == SCREEN_MAIN);
    // Dos chequeos separados a proposito: la cara grande anima sola
    // (bobbing, cambio de frame) y necesita redibujarse cada ~200ms
    // mientras estamos en la pantalla principal, pero el encabezado NO
    // tiene nada que se mueva solo - antes usaba el mismo chequeo que
    // la cara grande, y por eso la franja de arriba parpadeaba en
    // TODAS las pantallas cada vez que a la mascota le tocaba animar,
    // no solo cuando el texto que muestra cambiaba de verdad.
    bool mascotaCambio = mascota.necesitaRedibujar();
    bool encabezadoCambio = mascota.necesitaRedibujarEncabezado();

    if (cambioPantalla) {
        dibujarMenuBotones();
    }

    if (enPantallaPrincipal) {
        // Esta pantalla ES la mascota, no hay texto de bienvenida
        // compitiendo por el mismo espacio: antes se dibujaban los
        // dos en la misma franja, uno tapando al otro.
        if (cambioPantalla || mascotaCambio || encabezadoCambio) {
            // El borrado completo de la franja solo hace falta al
            // entrar a esta pantalla; en los redibujados de animacion
            // (mascotaCambio/encabezadoCambio solos) alcanza con que
            // la mascota limpie su propia zona, si no la pantalla
            // parpadea de mas.
            mascota.dibujar(cambioPantalla);
        }
    } else {
        // En el resto de las pantallas la mascota no tiene su cara
        // grande (ese lugar lo usa el contenido de la pantalla), pero
        // la franja de arriba con su estado se mantiene igual en
        // todos lados para no perderle la pista mientras mirás otra
        // cosa. Ojo: acá se chequea SOLO encabezadoCambio, no
        // mascotaCambio - si no, esta franja parpadearia en Escaneo,
        // Stats, Wardrive, etc cada vez que la cara grande (que ni se
        // ve en estas pantallas) tiene un tick de animacion.
        if (cambioPantalla || encabezadoCambio) {
            mascota.dibujarEncabezado();
        }
        if (contenidoSucio) {
            switch (pantallaActual) {
                case SCREEN_TOOLS:
                    dibujarPantallaCategorias();
                    break;
                case SCREEN_TOOLS_SUB:
                    dibujarPantallaHerramientasSub();
                    break;
                case SCREEN_SCAN:
                    dibujarListaRedes();
                    break;
                case SCREEN_STATS:
                    dibujarPantallaStats();
                    break;
                case SCREEN_WARDRIVE:
                    dibujarPantallaWardrive();
                    break;
                case SCREEN_CONFIG:
                    dibujarPantallaConfig();
                    break;
                case SCREEN_CONOCIDAS:
                    dibujarPantallaConocidas();
                    break;
                case SCREEN_EVENTOS:
                    dibujarPantallaEventos();
                    break;
                case SCREEN_MONITOR:
                    dibujarPantallaMonitor();
                    break;
                case SCREEN_CANALES:
                    dibujarPantallaCanales();
                    break;
                case SCREEN_DUPLICADOS:
                    dibujarPantallaDuplicados();
                    break;
                case SCREEN_RADAR:
                    dibujarPantallaRadar();
                    break;
                default:
                    break;
            }
        }

        // Config muestra heap/uptime en vivo: se refresca solo ese
        // bloque de numeros cada 1s, sin pasar por contenidoSucio (que
        // volveria a dibujar todo el texto fijo de arriba) y sin tocar
        // la SD para nada - por eso es una funcion aparte y no un
        // vuelta a dibujarPantallaConfig() completa.
        if (pantallaActual == SCREEN_CONFIG) {
            static unsigned long ultimoRefrescoConfig = 0;
            unsigned long ahoraConfig = millis();
            if (cambioPantalla || ahoraConfig - ultimoRefrescoConfig >= 1000) {
                ultimoRefrescoConfig = ahoraConfig;
                if (!cambioPantalla) {
                    // Si acabamos de entrar a la pantalla ya se pinto
                    // completa arriba (contenidoSucio); aca solo evito
                    // un segundo redibujado inmediato del mismo cuadro.
                    actualizarDiagnosticoHardware();
                }
            }
        }

        // Igual que en Config: el Monitor de deauth/disassoc se
        // repinta solo cada 1s mientras estamos parados en el, para
        // que una rafaga se note sin tener que salir y volver a
        // entrar a la pantalla. Esta pantalla no toca la SD para
        // nada (los contadores viven en RAM, dentro de WiFiScanner),
        // asi que ademas es mucho mas liviano que el refresco de
        // Config.
        if (pantallaActual == SCREEN_MONITOR) {
            static unsigned long ultimoRefrescoMonitor = 0;
            unsigned long ahoraMonitor = millis();
            if (cambioPantalla || ahoraMonitor - ultimoRefrescoMonitor >= 1000) {
                ultimoRefrescoMonitor = ahoraMonitor;
                if (!cambioPantalla) {
                    dibujarPantallaMonitor();
                }
            }
        }

        // Mismo patron que Monitor: RADAR tambien vive de datos que se
        // acumulan solos de fondo (el callback de BLE), asi que sin
        // esto la lista se quedaria congelada en lo que habia apenas
        // entraste a la pantalla hasta que la tocaras o salieras y
        // volvieras a entrar.
        if (pantallaActual == SCREEN_RADAR) {
            static unsigned long ultimoRefrescoRadar = 0;
            unsigned long ahoraRadar = millis();
            if (cambioPantalla || ahoraRadar - ultimoRefrescoRadar >= RADAR_REFRESCO_MS) {
                ultimoRefrescoRadar = ahoraRadar;
                if (!cambioPantalla) {
                    dibujarPantallaRadar();
                }
            }
        }
    }

    contenidoSucio = false;
    pantallaAnterior = pantallaActual;

    TouchPoint toque = touchDriver.read();
    if (toque.pressed) {
        ultimoToque = millis();

        if (mascota.getEstado() == ESTADO_SLEEP) {
            // El primer toque despues de dormir solo despierta: no
            // dispara ademas el boton o la zona de la pantalla que
            // haya debajo del dedo, para no activar algo sin querer
            // apenas se prende de nuevo la pantalla.
            digitalWrite(TFT_BL_PIN, HIGH);
            mascota.setEstado(ESTADO_IDLE);
            contenidoSucio = true;
            eventos.registrar("Se desperto", nullptr);
            #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
                // Mismo motivo que en el bloque de "Durmiendo": esto
                // acaba de escribir en la SD, hay que reclamar el bus
                // antes de que se lea el touch de vuelta en la proxima
                // vuelta del loop().
                touchDriver.reclaimBus();
            #endif
        } else if (toque.x >= TFT_PANEL_WIDTH / 3 && toque.y >= CONTENT_TOP && toque.y <= CONTENT_TOP + alturaListado()
            && pantallaActual == SCREEN_MAIN) {
            mascota.tocar(toque.x, toque.y);
        } else {
            manejarToque(toque.x, toque.y);
        }
    }

    delay(10);
}
