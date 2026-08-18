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

// Variables para el escaneo asíncrono
bool scanning = false;
unsigned long scanStartTime = 0;

// Callback cuando se captura un handshake (se ejecuta desde el loop)
void onHandshakeCaptured(const uint8_t* frame, uint32_t len) {
    // Aquí guardamos en PCAP y notificamos a la mascota
    uint32_t ts = millis();
    pcap.writePacket(frame, len, ts/1000, (ts%1000)*1000);
    
    mascota.incrementarHandshakes();
    
    Serial.printf("¡Handshake capturado! Tamaño: %d bytes\n", len);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("DEAUTH-CHAN v1.0 - ¡Hola mundo!");
    
    // Encender retroiluminación de la pantalla
    pinMode(TFT_LED_PIN, OUTPUT);
    digitalWrite(TFT_LED_PIN, HIGH);
    
    // Inicializar pantalla
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    // Inicializar táctil
    touch.begin();
    
    // Inicializar mascota
    mascota.init(&tft);
    
    // Inicializar SD y PCAP
    if (pcap.begin("deauth")) {
        Serial.println("PCAP iniciado correctamente");
    } else {
        Serial.println("Error con la SD, wardriving desactivado");
    }
    
    // Inicializar GPS
    gps.begin();
    
    // Inicializar WiFi Hunter y asignar callback
    hunter.begin();
    hunter.setHandshakeCallback(onHandshakeCaptured);
    
    Serial.println("Setup completado. ¡A divertirnos!");
}

void loop() {
    // Actualizar GPS (no bloqueante)
    gps.update();
    
    // Procesar handshakes pendientes (desde la cola)
    hunter.processPendingHandshakes();
    
    // Actualizar y dibujar la mascota (con sprite)
    mascota.update();
    mascota.dibujar();
    
    // Leer el táctil
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 0, 4095, 0, 240);
        int y = map(p.y, 0, 4095, 0, 320);
        mascota.tocar(x, y);
    }
    
    // --- Escaneo no bloqueante ---
    if (!scanning && millis() - lastScanTime > SCAN_INTERVAL) {
        // Iniciamos escaneo
        mascota.setEstado(ESTADO_SCANNING);
        hunter.startScan();
        scanning = true;
        scanStartTime = millis();
        Serial.println("Iniciando escaneo...");
    }
    
    if (scanning && hunter.isScanDone()) {
        // El escaneo ha terminado, recogemos resultados
        hunter.getScanResults(redes, 20, numRedes);
        mascota.setRedesEncontradas(numRedes);
        
        // Guardar en CSV con datos GPS (wardriving)
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
        
        // Volvemos a IDLE
        mascota.setEstado(ESTADO_IDLE);
        scanning = false;
        lastScanTime = millis();
        Serial.printf("Escaneo completado: %d redes encontradas\n", numRedes);
    }
    
    // Pequeña pausa para no saturar el CPU
    delay(10);
}
