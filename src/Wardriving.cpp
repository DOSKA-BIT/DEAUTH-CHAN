#include "Wardriving.h"
#include "boards/BoardConfig.h"

void Wardriving::begin() {
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD no disponible para wardriving");
        return;
    }

    csvFile = SD.open("/wardriving.csv", FILE_APPEND);
    if (!csvFile) {
        csvFile = SD.open("/wardriving.csv", FILE_WRITE);
        ensureHeader();
        csvFile.close();
        csvFile = SD.open("/wardriving.csv", FILE_APPEND);
    }
    Serial.println("Wardriving iniciado");
}

void Wardriving::ensureHeader() {
    if (csvFile) {
        csvFile.println("timestamp,ssid,bssid,rssi,channel,encryption,lat,lng,altitude");
    }
}

void Wardriving::setFilterByEncryption(bool enabled, int authMode) {
    filterEnabled = enabled;
    filterAuthMode = authMode;
}

void Wardriving::saveNetwork(const RedInfo& red, const GPSData& pos) {
    if (!csvFile) return;

    csvFile.printf("%s,%s,%02X:%02X:%02X:%02X:%02X:%02X,%d,%d,%d,%.6f,%.6f,%.1f\n",
        pos.valid ? pos.timestamp : "NO_FIX",
        red.ssid,
        red.bssid[0], red.bssid[1], red.bssid[2],
        red.bssid[3], red.bssid[4], red.bssid[5],
        red.rssi,
        red.canal,
        red.tieneClave ? 1 : 0,
        pos.lat,
        pos.lng,
        pos.altitude
    );
    csvFile.flush();
}

void Wardriving::exportKML() {
    // Igual que en la version original: el CSV ya tiene todo lo que
    // hace falta (lat/lng/ssid/rssi), pero armar el XML del KML bien
    // formado con estilos por intensidad de señal es tarea para
    // hacerla con calma en la fase 2, no algo que valga la pena
    // apurar en un dispositivo con 320KB de RAM.
    Serial.println("Exportacion a KML pendiente, usa wardriving.csv por ahora");
}
