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

RedInfo redes[MAX_REDES_ESCANEO];
int numRedes = 0;
bool escaneando = false;

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
    SCREEN_CONOCIDAS
};
MenuScreen pantallaActual = SCREEN_MAIN;

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

Herramienta herramientas[] = {
    {"Escanear", "Buscar redes WiFi cerca",         SCREEN_SCAN,     0, 0},
    {"Stats",    "Memoria de redes ya vistas",       SCREEN_STATS,    0, 0},
    {"Wardrive", "Guardar ubicacion + redes en SD",  SCREEN_WARDRIVE, 0, 0},
    {"Config",   "Datos de la placa y la SD",        SCREEN_CONFIG,   0, 0},
    {"Conocidas", "Redes guardadas, por senial",     SCREEN_CONOCIDAS, 0, 0},
};
const int CANTIDAD_HERRAMIENTAS = sizeof(herramientas) / sizeof(herramientas[0]);

int alturaListado(); // definida mas abajo, hace falta antes aca

void calcularLayoutHerramientas() {
    // El alto de cada fila se calcula segun cuantas herramientas haya
    // para que entren todas sin superponerse, en vez de tener un
    // numero fijo que se rompe el dia que se agrega una quinta o
    // sexta herramienta (como paso al sumar "Conocidas").
    int espacioDisponible = alturaListado() - 12;
    int gap = 4;
    int alto = (espacioDisponible - gap * (CANTIDAD_HERRAMIENTAS - 1)) / CANTIDAD_HERRAMIENTAS;
    if (alto > 50) alto = 50; // no hace falta que crezcan de mas si hay pocas

    int y = CONTENT_TOP + 8;
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
        uint16_t colorFondo = activo ? COLOR_ACCENT : COLOR_PANEL;
        uint16_t colorTexto = activo ? COLOR_BG : COLOR_TEXT_DIM;
        tft.fillRoundRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, 6, colorFondo);
        if (!activo) {
            tft.drawRoundRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, 6, COLOR_ACCENT_DIM);
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

void dibujarPantallaHerramientas() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);

    for (int i = 0; i < CANTIDAD_HERRAMIENTAS; i++) {
        Herramienta& h = herramientas[i];
        tft.fillRoundRect(8, h.y, TFT_PANEL_WIDTH - 16, h.h, 8, COLOR_PANEL);
        tft.drawRoundRect(8, h.y, TFT_PANEL_WIDTH - 16, h.h, 8, COLOR_ACCENT_DIM);

        tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
        tft.setTextFont(2);
        tft.setCursor(18, h.y + 6);
        tft.print(h.nombre);
        tft.setTextFont(1);

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
        tft.setCursor(18, h.y + 28);
        tft.print(h.descripcion);
    }

    // Espacio de sobra abajo de la lista para cuando se agreguen mas
    // herramientas - por ahora solo un cartelito avisando que hay
    // lugar, para que no quede una franja vacia sin explicacion.
    if (CANTIDAD_HERRAMIENTAS > 0) {
        int yHint = herramientas[CANTIDAD_HERRAMIENTAS - 1].y + herramientas[CANTIDAD_HERRAMIENTAS - 1].h + 10;
        if (yHint < CONTENT_TOP + alturaListado() - 10) {
            tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
            tft.setCursor(14, yHint);
            tft.print("+ mas herramientas pronto");
        }
    }
}

void dibujarPantallaConocidas() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);

    int total = aprendizaje.getTotalRedesConocidas();
    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, CONTENT_TOP + 20);
        tft.print("Todavia no hay redes guardadas");
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

    int maxVisibles = alturaListado() / 20;
    int yOffset = CONTENT_TOP + 10;
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
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, yOffset + 4);
        tft.print("+ ");
        tft.print(total - mostradas);
        tft.print(" mas, ordenadas por senial");
    }
}

void dibujarListaRedes() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);

    // Mientras hay un escaneo en curso no tiene sentido mostrar la
    // lista vieja por debajo del cartel de "Escaneando...": antes se
    // superponian (la lista anterior se quedaba dibujada y el cartel
    // le caia encima a mitad de camino). Se ve una cosa o la otra.
    if (escaneando) {
        tft.setTextColor(COLOR_WARN, COLOR_BG);
        tft.setCursor(10, CONTENT_TOP + 20);
        tft.print("Escaneando...");
        return;
    }

    int yOffset = CONTENT_TOP + 10;
    int maxVisibles = alturaListado() / 20;

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
        tft.setCursor(10, CONTENT_TOP + 20);
        tft.print("Toca aca para escanear");
    }
}

void dibujarPantallaStats() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextFont(2);
    tft.setCursor(10, CONTENT_TOP + 6);
    tft.print("Memoria de redes");
    tft.setTextFont(1);
    tft.drawFastHLine(10, CONTENT_TOP + 24, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);

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
}

void dibujarPantallaWardrive() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextFont(2);
    tft.setCursor(10, CONTENT_TOP + 6);
    tft.print("Wardriving");
    tft.setTextFont(1);
    tft.drawFastHLine(10, CONTENT_TOP + 24, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);

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
    int lineas = 0;
    SD.end();
    if (SD.begin(SD_CS_PIN)) {
        File f = SD.open("/wardriving.csv", FILE_READ);
        if (f) {
            while (f.available()) { if (f.read() == '\n') lineas++; }
            f.close();
            lineas -= 1;
        }
    }
    tft.print(lineas > 0 ? lineas : 0);

    tft.setCursor(10, CONTENT_TOP + 105);
    tft.setTextColor(COLOR_WARN, COLOR_BG);
    tft.print("[Toca para exportar KML]");

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
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextFont(2);
    tft.setCursor(10, CONTENT_TOP + 6);
    tft.print("Config");
    tft.setTextFont(1);
    tft.drawFastHLine(10, CONTENT_TOP + 24, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);

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
                    pantallaActual = h.destino;
                    return;
                }
            }
            break;
        case SCREEN_SCAN:
            if (y > CONTENT_TOP && y < CONTENT_TOP + alturaListado() && !escaneando) {
                mascota.setEstado(ESTADO_SCANNING);
                scanner.startScan();
                escaneando = true;
                contenidoSucio = true; // para que se vea "Escaneando..." ya mismo
            }
            break;
        case SCREEN_WARDRIVE:
            if (y > CONTENT_TOP + 95 && y < CONTENT_TOP + 120) {
                wardriver.exportKML();
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

    // Modo sleep: si nadie toco la pantalla en SLEEP_TIMEOUT_MS y no
    // hay un escaneo activo, apagamos el backlight. No tocamos nada
    // de WiFi/SD/GPS -- el resto de los modulos sigue funcionando
    // igual, lo unico que cambia es que la pantalla deja de consumir.
    if (!escaneando && mascota.getEstado() != ESTADO_SLEEP &&
        millis() - ultimoToque > SLEEP_TIMEOUT_MS) {
        mascota.setEstado(ESTADO_SLEEP);
        digitalWrite(TFT_BL_PIN, LOW);
    }

    if (pantallaActual == SCREEN_SCAN && escaneando && scanner.isScanDone()) {
        scanner.getScanResults(redes, MAX_REDES_ESCANEO, numRedes);
        mascota.setRedesEncontradas(numRedes);
        escaneando = false;
        mascota.setEstado(ESTADO_IDLE);

        GPSData pos = gps.getData();
        for (int i = 0; i < numRedes; i++) {
            wardriver.saveNetwork(redes[i], pos);
            bool esNueva = aprendizaje.registrarAvistamiento(redes[i]);
            if (esNueva) {
                mascota.incrementarRedesNuevas();
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
    bool mascotaCambio = mascota.necesitaRedibujar();

    if (cambioPantalla) {
        dibujarMenuBotones();
    }

    if (enPantallaPrincipal) {
        // Esta pantalla ES la mascota, no hay texto de bienvenida
        // compitiendo por el mismo espacio: antes se dibujaban los
        // dos en la misma franja, uno tapando al otro.
        if (cambioPantalla || mascotaCambio) {
            mascota.dibujar();
        }
    } else {
        // En el resto de las pantallas la mascota no tiene su cara
        // grande (ese lugar lo usa el contenido de la pantalla), pero
        // la franja de arriba con su estado se mantiene igual en
        // todos lados para no perderle la pista mientras mirás otra
        // cosa.
        if (cambioPantalla || mascotaCambio) {
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
        } else if (toque.x >= TFT_PANEL_WIDTH / 3 && toque.y >= CONTENT_TOP && toque.y <= CONTENT_TOP + alturaListado()
            && pantallaActual == SCREEN_MAIN) {
            mascota.tocar(toque.x, toque.y);
        } else {
            manejarToque(toque.x, toque.y);
        }
    }

    delay(10);
}
