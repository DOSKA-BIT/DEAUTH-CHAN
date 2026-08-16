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

// Callback cuando se detecta handshake real
void onHandshakeCaptured(const uint8_t* frame, uint32_t len) {
    // Guardar en PCAP con timestamp real
    uint32_t ts = millis();
    pcap.writePacket(frame, len, ts/1000, (ts%1000)*1000);
    
    // Notificar a la mascota
    mascota.incrementarHandshakes();
    
    Serial.printf("Handshake capturado! Tamaño: %d bytes\n", len);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("DEAUTH-CHAN v1.0");
    Serial.println("Iniciando...");
    
    // Inicializar pantalla
    pinMode(TFT_LED_PIN, OUTPUT);
    digitalWrite(TFT_LED_PIN, HIGH);
    
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    // Inicializar touch
    touch.begin();
    
    // Inicializar mascota
    mascota.init(&tft);
    
    // Inicializar SD y PCAP
    if (pcap.begin("deauth")) {
        Serial.println("PCAP iniciado correctamente");
    }
    
    // Inicializar GPS
    gps.begin();
    
    // Inicializar WiFi Hunter con callback
    hunter.begin();
    hunter.setHandshakeCallback(onHandshakeCaptured);
    
    Serial.println("Setup completo");
}

void loop() {
    // Actualizar GPS constantemente (no bloqueante)
    gps.update();
    
    // Actualizar animación de la mascota
    mascota.update();
    mascota.dibujar();
    
    // Verificar touch
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        // Mapear coordenadas según rotación
        int x = map(p.x, 0, 4095, 0, 240);
        int y = map(p.y, 0, 4095, 0, 320);
        
        mascota.tocar(x, y);
    }
    
    // === ACA VA EL CÓDIGO DE ESCANEO ===
    // Este reemplaza el scan simple que tenías antes
    
    if (millis() - lastScanTime > SCAN_INTERVAL) {
        mascota.setEstado(ESTADO_SCANNING);
        
        // Escanear redes WiFi
        hunter.scan(redes, 20, numRedes);
        mascota.setRedesEncontradas(numRedes);
        
        // Guardar en CSV para wardriving
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
        
        // El handshake ahora se detecta por callback, no por random
        // Solo volvemos a IDLE si no hay callback pendiente
        mascota.setEstado(ESTADO_IDLE);
        
        lastScanTime = millis();
    }
    
    delay(50);
}
