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

// Avisa que el contenido de la pantalla activa (lista, stats,
// wardriving, config) quedo desactualizado y hay que redibujarlo.
// Antes esto se disparaba con cada parpadeo de la mascota sin
// importar que pantalla estuviera activa; ahora solo se prende
// cuando de verdad cambio algo que vale la pena mostrar.
bool contenidoSucio = true;

enum MenuScreen {
    SCREEN_MAIN,
    SCREEN_SCAN,
    SCREEN_STATS,
    SCREEN_WARDRIVE,
    SCREEN_CONFIG
};
MenuScreen pantallaActual = SCREEN_MAIN;

struct Boton {
    int x, y, w, h;
    const char* label;
};

// El ancho de cada boton se calcula en runtime a partir de
// TFT_PANEL_WIDTH asi que la barra de menu ocupa todo el ancho de la
// pantalla sin importar si son 240px o 480px.
Boton menuBotones[4];

const char* NOMBRES_MENU[4] = {"Escanear", "Stats", "Wardrive", "Config"};

void calcularLayoutMenu() {
    int anchoBoton = TFT_PANEL_WIDTH / 4;
    int y = TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT;
    for (int i = 0; i < 4; i++) {
        menuBotones[i].x = i * anchoBoton;
        menuBotones[i].y = y;
        menuBotones[i].w = anchoBoton - 2;
        menuBotones[i].h = 34;
        menuBotones[i].label = NOMBRES_MENU[i];
    }
}

void dibujarMenuBotones() {
    // Linea fina arriba de la barra, para separarla del contenido en
    // vez de que los botones queden flotando sin mas.
    tft.drawFastHLine(0, TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - 1, TFT_PANEL_WIDTH, COLOR_ACCENT_DIM);

    for (int i = 0; i < 4; i++) {
        // i+1 porque SCREEN_MAIN no tiene boton propio, el indice del
        // menu arranca en SCREEN_SCAN
        bool activo = ((int)pantallaActual == i + 1);
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

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        touchDriver.reclaimBus();
    #endif
}

void manejarToque(int x, int y) {
    for (int i = 0; i < 4; i++) {
        if (x >= menuBotones[i].x && x <= menuBotones[i].x + menuBotones[i].w &&
            y >= menuBotones[i].y && y <= menuBotones[i].y + menuBotones[i].h) {
            pantallaActual = (MenuScreen)(i + 1);
            return;
        }
    }

    switch (pantallaActual) {
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
    mascota.init(&tft);

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
                default:
                    break;
            }
        }
    }

    contenidoSucio = false;
    pantallaAnterior = pantallaActual;

    TouchPoint toque = touchDriver.read();
    if (toque.pressed) {
        if (toque.x >= TFT_PANEL_WIDTH / 3 && toque.y >= CONTENT_TOP && toque.y <= CONTENT_TOP + alturaListado()
            && pantallaActual == SCREEN_MAIN) {
            mascota.tocar(toque.x, toque.y);
        } else {
            manejarToque(toque.x, toque.y);
        }
    }

    delay(10);
}
