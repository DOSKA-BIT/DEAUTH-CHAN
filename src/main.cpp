#include <Arduino.h>
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
#include "GPSModule.h"
#include "Wardriving.h"
#include "Learning.h"
#include "EventLog.h"
#include "Theme.h"

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
GPSModule gps;
Wardriving wardriver;
Learning aprendizaje;
EventLog eventos;

RedInfo redes[MAX_REDES_ESCANEO];
int numRedes = 0;
bool escaneando = false;

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
    SCREEN_TOOLS,
    SCREEN_SCAN,
    SCREEN_STATS,
    SCREEN_WARDRIVE,
    SCREEN_CONFIG,
    SCREEN_CONOCIDAS,
    SCREEN_EVENTOS,
    SCREEN_MONITOR
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

// Una fila por herramienta en la pantalla de Herramientas: nombre,
// descripcion corta y a que pantalla lleva al tocarla. Agregar una
// herramienta nueva es agregar una linea aca (y su pantalla propia),
// nada mas - no hay que tocar la barra de botones para nada.
struct Herramienta {
    const char* nombre;
    const char* descripcion;
    MenuScreen destino;
    int y, h; // se calculan en calcularLayoutHerramientas()
};

// Nombres en mayuscula a proposito: es parte de la estetica de lista
// de terminal (numerada, monocromatica) que reemplaza a las tarjetas
// redondeadas de antes - ver dibujarPantallaHerramientas().
Herramienta herramientas[] = {
    {"ESCANEAR", "Buscar redes WiFi cerca",         SCREEN_SCAN,     0, 0},
    {"STATS",    "Memoria de redes ya vistas",       SCREEN_STATS,    0, 0},
    {"WARDRIVE", "Guardar ubicacion + redes en SD",  SCREEN_WARDRIVE, 0, 0},
    {"CONFIG",   "Datos de la placa y la SD",        SCREEN_CONFIG,   0, 0},
    {"CONOCIDAS", "Redes guardadas, por senial",     SCREEN_CONOCIDAS, 0, 0},
    {"EVENTOS",   "Historial de esta sesion",        SCREEN_EVENTOS,   0, 0},
    {"MONITOR",   "Deauth/disassoc detectados",       SCREEN_MONITOR,   0, 0},
};
const int CANTIDAD_HERRAMIENTAS = sizeof(herramientas) / sizeof(herramientas[0]);

int alturaListado(); // definida mas abajo, hace falta antes aca

// Alto que ocupa el titulo de seccion (dibujarTituloSeccion, definida
// mas abajo junto con alturaListado) incluyendo su linea separadora -
// las pantallas que lo usan arrancan su propio contenido despues de
// este offset. Declarado aca arriba porque calcularLayoutHerramientas
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

void calcularLayoutHerramientas() {
    // El alto de cada fila se calcula segun cuantas herramientas haya
    // para que entren todas sin superponerse, en vez de tener un
    // numero fijo que se rompe el dia que se agrega una quinta o
    // sexta herramienta (como paso al sumar "Conocidas"). Filas de una
    // sola linea (antes tenian nombre + descripcion + separador, ahora
    // la descripcion se muestra aparte, en el renglon de ayuda de
    // abajo) asi que son mas bajas y entran mas sin apretarse. Arrancan
    // despues de ALTO_TITULO_SECCION porque ahora esta pantalla tiene
    // su propio titulo "HERRAMIENTAS" arriba de la lista.
    int espacioDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int gap = 2;
    int alto = (espacioDisponible - gap * (CANTIDAD_HERRAMIENTAS - 1)) / CANTIDAD_HERRAMIENTAS;
    if (alto > 28) alto = 28; // no hace falta que crezcan de mas si hay pocas
    if (alto < 18) alto = 18; // tampoco que se aplasten si hay muchas

    int y = CONTENT_TOP + ALTO_TITULO_SECCION;
    for (int i = 0; i < CANTIDAD_HERRAMIENTAS; i++) {
        herramientas[i].y = y;
        herramientas[i].h = alto;
        y += alto + gap;
    }
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
void dibujarTituloSeccion(const char* titulo) {
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextFont(2);
    tft.setCursor(10, CONTENT_TOP + 6);
    tft.print(titulo);
    tft.setTextFont(1);
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

void dibujarPantallaHerramientas() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("HERRAMIENTAS");

    // Lista de una sola linea por item (numero + nombre, nada de
    // descripcion ni separador en cada fila) calcada del menu de
    // PORKCHOP/Marauder que mando el usuario de referencia. La
    // descripcion de cada herramienta se muestra aparte, en el
    // renglon de ayuda de mas abajo, solo mientras esa fila esta
    // destellando por el toque - asi la lista en si queda tan limpia
    // como la de la foto.
    for (int i = 0; i < CANTIDAD_HERRAMIENTAS; i++) {
        Herramienta& h = herramientas[i];

        char numero[4];
        snprintf(numero, sizeof(numero), "%02d", i + 1);

        // La fila recien tocada se dibuja invertida (fondo celeste,
        // texto negro) como un destello breve de feedback tactil - ver
        // el comentario de filaDestellada arriba, junto a su
        // declaracion. El resto de las filas van con los colores
        // normales.
        bool destellada = (i == filaDestellada);
        uint16_t colorFondoFila = destellada ? COLOR_ACCENT : COLOR_BG;
        uint16_t colorTextoFila = destellada ? COLOR_BG : COLOR_TEXT;

        if (destellada) {
            tft.fillRect(0, h.y, TFT_PANEL_WIDTH, h.h, COLOR_ACCENT);
        }

        tft.setTextColor(colorTextoFila, colorFondoFila);
        tft.setTextFont(2);
        tft.setCursor(10, h.y + (h.h / 2) - 7);
        tft.print(numero);
        tft.print(' ');
        tft.print(h.nombre);
    }

    // Pie de ayuda: mientras una fila esta destellando muestra SU
    // descripcion (asi no hace falta repetirla en cada item, como en
    // la referencia), y el resto del tiempo un recordatorio generico.
    if (filaDestellada >= 0 && filaDestellada < CANTIDAD_HERRAMIENTAS) {
        dibujarPieAyuda(herramientas[filaDestellada].descripcion);
    } else {
        dibujarPieAyuda("Toca una herramienta para entrar");
    }
}

void dibujarPantallaConocidas() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CONOCIDAS");
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    int total = aprendizaje.getTotalRedesConocidas();
    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Todavia no hay redes guardadas");
        dibujarPieAyuda("Escanea para empezar a guardar redes");
        return;
    }

    // Ordenadas por la mejor señal vista, de mas fuerte a mas debil.
    // Podria ordenarse por "ultima vez vista" en cambio, pero eso usa
    // millis() y esos valores quedan grabados en la SD de una sesion
    // anterior - comparados contra el millis() de esta sesion (que
    // arranca de cero en cada reset) no dicen nada real. La señal en
    // cambio es un numero que sigue significando lo mismo siempre.
    RedStats* todas = aprendizaje.getTodas();
    int indices[MAX_REDES_ESCANEO];
    for (int i = 0; i < total; i++) indices[i] = i;
    for (int i = 0; i < total - 1; i++) {
        for (int j = 0; j < total - 1 - i; j++) {
            if (todas[indices[j]].mejorRSSI < todas[indices[j + 1]].mejorRSSI) {
                int tmp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = tmp;
            }
        }
    }

    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / 20;
    int yOffset = top + 6;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        RedStats& r = todas[indices[i]];

        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(5, yOffset);
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
        snprintf(masTexto, sizeof(masTexto), "+ %d mas, por senial", total - mostradas);
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("Ordenadas por senial, fuerte a debil");
    }
}

void dibujarPantallaEventos() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("EVENTOS");
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    int total = 0;
    EventoLog* lista = eventos.getEventos(total);

    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Todavia no paso nada en esta sesion");
        dibujarPieAyuda("El historial empieza al encender");
        return;
    }

    // El mas reciente ya queda primero en el array (ver EventLog::
    // registrar), asi que aca alcanza con recorrerlo en orden.
    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / 34;
    int yOffset = top + 2;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        EventoLog& e = lista[i];

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(5, yOffset);
        tft.print(e.timestamp);

        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(5, yOffset + 16);
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
    dibujarTituloSeccion("MONITOR");
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

    int total = 0;
    AnomaliaWiFi* lista = scanner.getAnomalias(total);

    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 36);
        tft.print("Nada detectado por ahora");
        dibujarPieAyuda("Pasivo: solo mira, no transmite nada");
        return;
    }

    // El canal del radio en este momento no se controla desde aca (lo
    // dejan donde haya quedado el ultimo scan o conteo de clientes),
    // asi que esta lista solo ve lo que pasa en ESE canal - no es un
    // monitor de toda la banda 2.4GHz a la vez. Igual sirve para
    // notar una rafaga de deauth mientras mirás esa red puntual.
    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - 30 - ALTO_PIE_AYUDA) / 34;
    int yOffset = top + 28;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        AnomaliaWiFi& a = lista[i];
        unsigned long segundos = a.momento / 1000;
        char horaBuf[16];
        sprintf(horaBuf, "T+%02lu:%02lu:%02lu", segundos / 3600, (segundos % 3600) / 60, segundos % 60);

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(5, yOffset);
        tft.print(horaBuf);
        tft.print("  ch");
        tft.print(a.canal);

        tft.setTextColor(a.esDisassoc ? COLOR_WARN : COLOR_DANGER, COLOR_BG);
        tft.setCursor(5, yOffset + 16);
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
    dibujarTituloSeccion("ESCANEAR");

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
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
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
        char buf[40];
        if (strlen(redes[i].ssid) == 0) {
            sprintf(buf, "%d.*OCULTA*", i + 1);
        } else {
            sprintf(buf, "%d.%s", i + 1, redes[i].ssid);
        }
        tft.setTextColor(redes[i].tieneClave ? COLOR_TEXT : COLOR_WARN, COLOR_BG);
        tft.setCursor(5, yOffset);
        tft.print(buf);

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

void dibujarPantallaStats() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("STATS");

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
    dibujarTituloSeccion("WARDRIVE");

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 35);
    tft.print("GPS: ");
    GPSData pos = gps.getData();
    tft.setTextColor(pos.valid ? COLOR_ACCENT : COLOR_WARN, COLOR_BG);
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
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
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
        tft.setTextColor(ultimoExportKML > 0 ? COLOR_ACCENT : COLOR_WARN, COLOR_BG);
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

void dibujarPantallaConfig() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CONFIG");

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
    tft.setTextColor(sdOk ? COLOR_ACCENT : COLOR_DANGER, COLOR_BG);
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
            for (int i = 0; i < CANTIDAD_HERRAMIENTAS; i++) {
                Herramienta& h = herramientas[i];
                if (y >= h.y && y <= h.y + h.h) {
                    // No navegamos todavia: primero se ve el destello
                    // (fila invertida) como feedback de que el toque se
                    // registro, y recien cuando pasa DESTELLO_MS (ver
                    // loop()) se entra de verdad a la pantalla destino.
                    filaDestellada = i;
                    momentoDestello = millis();
                    destinoPendiente = h.destino;
                    contenidoSucio = true;
                    return;
                }
            }
            break;
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
    calcularLayoutHerramientas();
    mascota.init(&tft);
    ultimoToque = millis();

    if (pcap.begin("wardrive")) {
        Serial.println("PCAP listo");
    }

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

    if (pantallaActual == SCREEN_SCAN && escaneando && scanner.isScanDone()) {
        scanner.getScanResults(redes, MAX_REDES_ESCANEO, numRedes);
        mascota.setRedesEncontradas(numRedes);
        escaneando = false;
        mascota.setEstado(ESTADO_IDLE);

        GPSData pos = gps.getData();
        int nuevasEnEsteScan = 0;
        for (int i = 0; i < numRedes; i++) {
            wardriver.saveNetwork(redes[i], pos);
            bool esNueva = aprendizaje.registrarAvistamiento(redes[i]);
            if (esNueva) {
                mascota.incrementarRedesNuevas();
                nuevasEnEsteScan++;
            }
        }
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
        // volver a escribir la SD veinte veces seguidas.
        char resumenEvento[40];
        snprintf(resumenEvento, sizeof(resumenEvento), "Escaneo: %d redes (%d nuevas)", numRedes, nuevasEnEsteScan);
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

        contenidoSucio = true; // hay resultados nuevos para mostrar en la lista
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
                    dibujarPantallaHerramientas();
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
