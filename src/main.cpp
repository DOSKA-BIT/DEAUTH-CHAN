#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>
#include "config.h"
#include "Mascota.h"
#include "WiFiHunter.h"
#include "PCAPWriter.h"
#include "GPSModule.h"
#include "Wardriving.h"
#include "Learning.h"

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

Mascota mascota;
WiFiHunter hunter;
PCAPWriter pcap;
GPSModule gps;
Wardriving wardriver;
Learning learner;

RedInfo redes[20];
int numRedes = 0;
bool scanning = false;
unsigned long lastScanTime = 0;

ClienteInfo* clientes = nullptr;
int numClientes = 0;

enum MenuScreen {
    SCREEN_MAIN,
    SCREEN_SCAN,
    SCREEN_ATTACK,
    SCREEN_CONFIG,
    SCREEN_WARDRIVE
};
MenuScreen currentScreen = SCREEN_MAIN;

struct Button {
    int x, y, w, h;
    char label[12];
};
Button menuButtons[4] = {
    {10, 280, 55, 30, "Escanear"},
    {70, 280, 55, 30, "Atacar"},
    {130, 280, 55, 30, "Config"},
    {190, 280, 55, 30, "Wardrive"}
};

void onHandshakeCaptured(const uint8_t* frame, uint32_t len) {
    uint32_t ts = millis();
    pcap.writePacket(frame, len, ts/1000, (ts%1000)*1000);
    mascota.incrementarHandshakes();
    Serial.printf("Handshake capturado! %d bytes\n", len);
}

void drawMenuButtons() {
    for (int i = 0; i < 4; i++) {
        int color = (currentScreen == i) ? TFT_GREEN : TFT_DARKGREY;
        tft.fillRoundRect(menuButtons[i].x, menuButtons[i].y, menuButtons[i].w, menuButtons[i].h, 4, color);
        tft.drawRoundRect(menuButtons[i].x, menuButtons[i].y, menuButtons[i].w, menuButtons[i].h, 4, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        tft.setCursor(menuButtons[i].x + 4, menuButtons[i].y + 10);
        tft.print(menuButtons[i].label);
    }
}

void drawNetworkList() {
    tft.fillRect(0, 100, 240, 180, TFT_BLACK);
    int yOffset = 110;
    for (int i = 0; i < numRedes && i < 8; i++) {
        char buf[32];
        sprintf(buf, "%d.%s", i+1, redes[i].ssid);
        if (strlen(redes[i].ssid) == 0) {
            sprintf(buf, "%d.*OCULTA*", i+1);
        }
        tft.setTextColor(TFT_YELLOW);
        tft.setCursor(5, yOffset);
        tft.print(buf);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(160, yOffset);
        tft.print(redes[i].rssi);
        tft.print("dBm");
        yOffset += 20;
    }
}

void drawConfigScreen() {
    tft.fillRect(0, 100, 240, 180, TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 110);
    tft.print("MODO SIGILOSO: ");
    tft.setTextColor(hunter.isSilentMode() ? TFT_GREEN : TFT_RED);
    tft.print(hunter.isSilentMode() ? "ON" : "OFF");
    tft.setCursor(10, 140);
    tft.setTextColor(TFT_WHITE);
    tft.print("[Toca para cambiar]");
    
    tft.setCursor(10, 170);
    tft.setTextColor(TFT_YELLOW);
    tft.print("BEACON FLOOD:");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 185);
    tft.print("SSID: DEAUTH_TEST");
    tft.setCursor(10, 210);
    tft.print("[Toca para enviar]");
    
    tft.setCursor(10, 240);
    tft.setTextColor(TFT_PURPLE);
    tft.print("RANDOM MAC:");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 255);
    tft.print("[Toca para cambiar]");
}

void drawWardriveScreen() {
    tft.fillRect(0, 100, 240, 180, TFT_BLACK);
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(10, 110);
    tft.print("WARDROVING");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 130);
    tft.print("GPS: ");
    GPSData pos = gps.getData();
    tft.print(pos.valid ? "FIX" : "NO FIX");
    tft.setCursor(10, 150);
    tft.print("SAT: ");
    tft.print(pos.satellites);
    tft.setCursor(10, 170);
    tft.print("REDES GUARDADAS: ");
    File f = SD.open("/wardriving.csv", FILE_READ);
    int lines = 0;
    if (f) {
        while (f.available()) { if (f.read() == '\n') lines++; }
        f.close();
    }
    tft.print(lines-1);
    tft.setCursor(10, 200);
    tft.setTextColor(TFT_YELLOW);
    tft.print("[Toca para exportar KML]");
}

void handleTouch(int x, int y) {
    for (int i = 0; i < 4; i++) {
        if (x >= menuButtons[i].x && x <= menuButtons[i].x + menuButtons[i].w &&
            y >= menuButtons[i].y && y <= menuButtons[i].y + menuButtons[i].h) {
            currentScreen = (MenuScreen)i;
            return;
        }
    }
    
    switch (currentScreen) {
        case SCREEN_SCAN:
            if (y > 100 && y < 200) {
                mascota.setEstado(ESTADO_SCANNING);
                hunter.startScan();
                scanning = true;
            }
            break;
        case SCREEN_ATTACK:
            if (y > 100 && y < 280 && numRedes > 0) {
                int index = (y - 110) / 20;
                if (index < numRedes) {
                    mascota.setEstado(ESTADO_ATTACK);
                    hunter.deauth(redes[index], nullptr, 30);
                    learner.registerAttack(redes[index]);
                    delay(200);
                    mascota.setEstado(ESTADO_IDLE);
                }
            }
            break;
        case SCREEN_CONFIG:
            if (x > 10 && x < 150) {
                if (y > 110 && y < 130) {
                    hunter.setSilentMode(!hunter.isSilentMode());
                }
                if (y > 170 && y < 230) {
                    hunter.beaconFlood("DEAUTH_TEST", 50);
                }
                if (y > 240 && y < 270) {
                    hunter.randomizeMAC();
                }
            }
            break;
        case SCREEN_WARDRIVE:
            if (y > 200 && y < 230) {
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
    Serial.println("DEAUTH-CHAN v2.0 - INTERFAZ PROFESIONAL");
    
    pinMode(TFT_LED_PIN, OUTPUT);
    digitalWrite(TFT_LED_PIN, HIGH);
    
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    touch.begin();
    mascota.init(&tft);
    
    if (pcap.begin("deauth")) {
        Serial.println("PCAP iniciado");
    }
    
    gps.begin();
    hunter.begin();
    hunter.setHandshakeCallback(onHandshakeCaptured);
    wardriver.begin();
    learner.begin();
    
    Serial.println("Setup completado");
}

void loop() {
    gps.update();
    hunter.processPendingHandshakes();
    mascota.update();
    
    mascota.dibujar();
    drawMenuButtons();
    
    switch (currentScreen) {
        case SCREEN_MAIN:
            tft.fillRect(0, 100, 240, 180, TFT_BLACK);
            tft.setTextColor(TFT_WHITE);
            tft.setCursor(30, 150);
            tft.print("Bienvenido");
            tft.setCursor(50, 180);
            tft.print("Toca un boton");
            break;
        case SCREEN_SCAN:
            drawNetworkList();
            if (scanning && hunter.isScanDone()) {
                hunter.getScanResults(redes, 20, numRedes);
                mascota.setRedesEncontradas(numRedes);
                scanning = false;
                mascota.setEstado(ESTADO_IDLE);
                GPSData pos = gps.getData();
                for (int i = 0; i < numRedes; i++) {
                    wardriver.saveNetwork(redes[i], pos);
                }
                Serial.printf("Escaneo completado: %d redes\n", numRedes);
            }
            break;
        case SCREEN_ATTACK:
            drawNetworkList();
            break;
        case SCREEN_CONFIG:
            drawConfigScreen();
            break;
        case SCREEN_WARDRIVE:
            drawWardriveScreen();
            break;
    }
    
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 0, 4095, 0, 240);
        int y = map(p.y, 0, 4095, 0, 320);
        
        if (x >= 80 && x <= 240 && y >= 100 && y <= 280) {
            mascota.tocar(x, y);
        } else {
            handleTouch(x, y);
        }
    }
    
    delay(10);
}
