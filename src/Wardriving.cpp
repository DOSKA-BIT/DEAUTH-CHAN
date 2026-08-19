#include "Wardriving.h"

void Wardriving::begin() {
    if (!SD.begin(SD_CS)) {
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
    
    // Filtro por cifrado (necesita ajuste, por ahora se guarda todo)
    // En una implementación real, se podría obtener el tipo de cifrado de la red
    // pero lo dejamos simple por ahora.
    
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
    Serial.println("Exportación KML no implementada (requiere procesamiento de CSV)");
}
