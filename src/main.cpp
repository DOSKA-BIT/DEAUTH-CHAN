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
    int y = TFT_PANEL_HEIGHT - 40;
    for (int i = 0; i < 4; i++) {
        menuBotones[i].x = i * anchoBoton;
        menuBotones[i].y = y;
        menuBotones[i].w = anchoBoton - 2;
        menuBotones[i].h = 34;
        menuBotones[i].label = NOMBRES_MENU[i];
    }
}

void dibujarMenuBotones() {
    for (int i = 0; i < 4; i++) {
        // i+1 porque SCREEN_MAIN no tiene boton propio, el indice del
        // menu arranca en SCREEN_SCAN
        bool activo = ((int)pantallaActual == i + 1);
        uint16_t color = activo ? TFT_GREEN : TFT_DARKGREY;
        tft.fillRoundRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, 4, color);
        tft.drawRoundRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, 4, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        tft.setCursor(menuBotones[i].x + 4, menuBotones[i].y + 12);
        tft.print(menuBotones[i].label);
    }
}

int alturaListado() {
    // Deja lugar para la cabecera de la mascota arriba y la barra de
    // menu abajo, el resto es zona util para listas de texto.
    return TFT_PANEL_HEIGHT - 40 - 100;
}

void dibujarListaRedes() {
    tft.fillRect(0, 100, TFT_PANEL_WIDTH, alturaListado(), TFT_BLACK);
    int yOffset = 110;
    int maxVisibles = alturaListado() / 20;

    for (int i = 0; i < numRedes && i < maxVisibles; i++) {
        char buf[40];
        if (strlen(redes[i].ssid) == 0) {
            sprintf(buf, "%d.*OCULTA*", i + 1);
        } else {
            sprintf(buf, "%d.%s", i + 1, redes[i].ssid);
        }
        tft.setTextColor(redes[i].tieneClave ? TFT_YELLOW : TFT_RED);
        tft.setCursor(5, yOffset);
        tft.print(buf);

        tft.setTextColor(TFT_WHITE);
        tft.setCursor(TFT_PANEL_WIDTH - 70, yOffset);
        tft.print(redes[i].rssi);
        tft.print("dBm");
        yOffset += 20;
    }

    if (numRedes == 0 && !escaneando) {
        tft.setTextColor(TFT_DARKGREY);
        tft.setCursor(10, 130);
        tft.print("Toca aca para escanear");
    }
}

void dibujarPantallaStats() {
    tft.fillRect(0, 100, TFT_PANEL_WIDTH, alturaListado(), TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 110);
    tft.print("MEMORIA DE REDES");

    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 135);
    tft.print("Redes conocidas: ");
    tft.print(aprendizaje.getTotalRedesConocidas());

    tft.setCursor(10, 155);
    tft.print("Vistas este escaneo: ");
    tft.print(numRedes);

    tft.setCursor(10, 175);
    tft.print("Nuevas descubiertas: ");
    tft.print(mascota.getRedesNuevas());

    if (numRedes > 0) {
        RedStats* s = aprendizaje.getStats(redes[0].bssid);
        if (s) {
            tft.setCursor(10, 205);
            tft.setTextColor(TFT_YELLOW);
            tft.print("Ultima red top: ");
            tft.print(s->ssid);
            tft.setCursor(10, 225);
            tft.setTextColor(TFT_WHITE);
            tft.print("Vista ");
            tft.print(s->vecesVista);
            tft.print(" veces");
        }
    }
}

void dibujarPantallaWardrive() {
    tft.fillRect(0, 100, TFT_PANEL_WIDTH, alturaListado(), TFT_BLACK);
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(10, 110);
    tft.print("WARDRIVING");

    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 130);
    tft.print("GPS: ");
    GPSData pos = gps.getData();
    tft.print(pos.valid ? "FIX" : "SIN FIX");

    tft.setCursor(10, 150);
    tft.print("Satelites: ");
    tft.print(pos.satellites);

    tft.setCursor(10, 170);
    tft.print("Redes guardadas: ");
    int lineas = 0;
    if (SD.begin(SD_CS_PIN)) {
        File f = SD.open("/wardriving.csv", FILE_READ);
        if (f) {
            while (f.available()) { if (f.read() == '\n') lineas++; }
            f.close();
            lineas -= 1;
        }
    }
    tft.print(lineas > 0 ? lineas : 0);

    tft.setCursor(10, 200);
    tft.setTextColor(TFT_YELLOW);
    tft.print("[Toca para exportar KML]");

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        touchDriver.reclaimBus();
    #endif
}

void dibujarPantallaConfig() {
    tft.fillRect(0, 100, TFT_PANEL_WIDTH, alturaListado(), TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 110);
    tft.print("Placa: ");
    tft.setTextColor(TFT_WHITE);
    tft.print(BOARD_NAME);

    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 130);
    tft.print("Pantalla: ");
    tft.setTextColor(TFT_WHITE);
    tft.print(TFT_PANEL_WIDTH);
    tft.print("x");
    tft.print(TFT_PANEL_HEIGHT);

    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 150);
    tft.print("Touch: ");
    tft.setTextColor(TFT_WHITE);
    tft.print(TOUCH_IS_CAPACITIVE ? "Capacitivo" : "Resistivo");

    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 170);
    tft.print("SD: ");
    bool sdOk = SD.begin(SD_CS_PIN);
    tft.setTextColor(sdOk ? TFT_GREEN : TFT_RED);
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
            if (y > 100 && y < 100 + alturaListado() && !escaneando) {
                mascota.setEstado(ESTADO_SCANNING);
                scanner.startScan();
                escaneando = true;
            }
            break;
        case SCREEN_WARDRIVE:
            if (y > 195 && y < 220) {
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
    tft.fillScreen(TFT_BLACK);

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

    Serial.println("Setup completado");
}

void loop() {
    gps.update();
    mascota.update();

    mascota.dibujar();
    dibujarMenuBotones();

    switch (pantallaActual) {
        case SCREEN_MAIN:
            tft.fillRect(0, 100, TFT_PANEL_WIDTH, alturaListado(), TFT_BLACK);
            tft.setTextColor(TFT_WHITE);
            tft.setCursor(20, 150);
            tft.print("Bienvenido a bordo");
            tft.setCursor(20, 175);
            tft.print("Toca un boton de abajo");
            break;

        case SCREEN_SCAN:
            dibujarListaRedes();
            if (escaneando && scanner.isScanDone()) {
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
                    // La SD y el touch comparten el periferico VSPI en
                    // esta placa (ver el comentario en Board_2432S028.h).
                    // Cada escritura a SD de arriba remapeo el bus hacia
                    // sus propios pines, asi que hay que recuperarlo para
                    // el touch antes de seguir escuchando toques.
                    touchDriver.reclaimBus();
                #endif
            }
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
    }

    TouchPoint toque = touchDriver.read();
    if (toque.pressed) {
        if (toque.x >= TFT_PANEL_WIDTH / 3 && toque.y >= 100 && toque.y <= 100 + alturaListado()
            && pantallaActual == SCREEN_MAIN) {
            mascota.tocar(toque.x, toque.y);
        } else {
            manejarToque(toque.x, toque.y);
        }
    }

    delay(10);
}
