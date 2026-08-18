#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>
#include "config.h"
#include "Mascota.h"
#include "WiFiHunter.h"
#include "PCAPWriter.h"
#include "GPSModule.h"

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

Mascota mascota;
WiFiHunter hunter;
PCAPWriter pcap;
GPSModule gps;

RedInfo redes[20];
int numRedes = 0;
unsigned long lastScanTime = 0;

bool scanning = false;
int redSeleccionada = -1;

ClienteInfo* clientes = nullptr;
int numClientes = 0;
bool escaneandoClientes = false;

// Modos de visualización
enum ModoPantalla {
    MODO_REDES,
    MODO_CLIENTES
};
ModoPantalla modoActual = MODO_REDES;

void onHandshakeCaptured(const uint8_t* frame, uint32_t len) {
    uint32_t ts = millis();
    pcap.writePacket(frame, len, ts/1000, (ts%1000)*1000);
    mascota.incrementarHandshakes();
    Serial.printf("Handshake capturado! %d bytes\n", len);
}

void mostrarRedes() {
    tft.fillRect(0, 200, 240, 120, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    int y = 200;
    for (int i = 0; i < numRedes && i < 6; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s (%d)", redes[i].ssid, redes[i].rssi);
        tft.setCursor(5, y);
        tft.print(buf);
        y += 20;
    }
}

void mostrarClientes() {
    tft.fillRect(0, 200, 240, 120, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    int y = 200;
    int maxMostrar = (numClientes < 6) ? numClientes : 6;
    for (int i = 0; i < maxMostrar; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s (%d dBm)", clientes[i].fabricante, clientes[i].rssi);
        tft.setCursor(5, y);
        tft.print(buf);
        y += 20;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("DEAUTH-CHAN v1.0");
    
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
    
    Serial.println("Setup completado");
}

void loop() {
    gps.update();
    hunter.processPendingHandshakes();
    mascota.update();
    mascota.dibujar();
    
    // Dibujar lista según modo
    if (modoActual == MODO_REDES) {
        mostrarRedes();
    } else if (modoActual == MODO_CLIENTES) {
        mostrarClientes();
    }
    
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 0, 4095, 0, 240);
        int y = map(p.y, 0, 4095, 0, 320);
        
        if (mascota.getEstado() == ESTADO_IDLE) {
            if (modoActual == MODO_REDES && y > 200) {
                int indice = (y - 200) / 20;
                if (indice < numRedes) {
                    redSeleccionada = indice;
                    // Cambiar a modo clientes y escanear
                    modoActual = MODO_CLIENTES;
                    mascota.setEstado(ESTADO_SCANNING);
                    hunter.scanClients(redes[redSeleccionada].bssid, redes[redSeleccionada].canal);
                    clientes = hunter.getClientes(numClientes);
                    mascota.setEstado(ESTADO_IDLE);
                }
            } else if (modoActual == MODO_CLIENTES && y > 200) {
                int indice = (y - 200) / 20;
                if (indice < numClientes) {
                    // Atacar a ese cliente específico
                    mascota.setEstado(ESTADO_ATTACK);
                    hunter.deauth(redes[redSeleccionada], clientes[indice].mac, 30);
                    delay(100);
                    mascota.setEstado(ESTADO_IDLE);
                }
            } else if (y < 200) {
                // Tocar la mascota o volver al modo redes
                if (modoActual == MODO_CLIENTES) {
                    modoActual = MODO_REDES;
                } else {
                    mascota.tocar(x, y);
                }
            }
        }
    }
    
    // Escaneo automático de redes
    if (!scanning && millis() - lastScanTime > SCAN_INTERVAL && modoActual == MODO_REDES) {
        mascota.setEstado(ESTADO_SCANNING);
        hunter.startScan();
        scanning = true;
    }
    
    if (scanning && hunter.isScanDone()) {
        hunter.getScanResults(redes, 20, numRedes);
        mascota.setRedesEncontradas(numRedes);
        
        GPSData pos = gps.getData();
        File csv = SD.open("/wardriving.csv", FILE_APPEND);
        if (csv) {
            for (int i = 0; i < numRedes; i++) {
                csv.printf("%s,%s,%02X:%02X:%02X:%02X:%02X:%02X,%d,%d,%.6f,%.6f,%.1f\n",
                    pos.valid ? pos.timestamp : "NO_FIX",
                    redes[i].ssid,
                    redes[i].bssid[0], redes[i].bssid[1], redes[i].bssid[2],
                    redes[i].bssid[3], redes[i].bssid[4], redes[i].bssid[5],
                    redes[i].rssi,
                    redes[i].canal,
                    pos.lat,
                    pos.lng,
                    pos.altitude
                );
            }
            csv.close();
        }
        
        mascota.setEstado(ESTADO_IDLE);
        scanning = false;
        lastScanTime = millis();
        Serial.printf("Escaneo completado: %d redes\n", numRedes);
    }
    
    delay(10);
}
