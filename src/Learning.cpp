#include "Learning.h"

void Learning::begin() {
    cargarDesdeSD();
    Serial.printf("Memoria de redes cargada: %d redes conocidas\n", numStats);
}

bool Learning::registrarAvistamiento(const RedInfo& red) {
    RedStats* existente = buscar(red.bssid);
    unsigned long ahora = millis();

    if (existente) {
        existente->vecesVista++;
        existente->ultimaVez = ahora;
        if (red.rssi > existente->mejorRSSI) {
            existente->mejorRSSI = red.rssi;
        }
        guardarEnSD();
        return false;
    }

    if (numStats >= MAX_REDES_ESCANEO) {
        // Se lleno el historial: descarto la mas vieja para hacerle
        // lugar a la nueva, priorizando lo reciente por sobre lo viejo.
        int indiceMasViejo = 0;
        for (int i = 1; i < numStats; i++) {
            if (stats[i].ultimaVez < stats[indiceMasViejo].ultimaVez) {
                indiceMasViejo = i;
            }
        }
        memmove(&stats[indiceMasViejo], &stats[indiceMasViejo + 1],
                sizeof(RedStats) * (numStats - indiceMasViejo - 1));
        numStats--;
    }

    memcpy(stats[numStats].bssid, red.bssid, 6);
    strncpy(stats[numStats].ssid, red.ssid, 32);
    stats[numStats].ssid[32] = '\0';
    stats[numStats].vecesVista = 1;
    stats[numStats].mejorRSSI = red.rssi;
    stats[numStats].primeraVez = ahora;
    stats[numStats].ultimaVez = ahora;
    numStats++;

    guardarEnSD();
    return true;
}

RedStats* Learning::getStats(const uint8_t* bssid) {
    return buscar(bssid);
}

RedStats* Learning::buscar(const uint8_t* bssid) {
    for (int i = 0; i < numStats; i++) {
        if (memcmp(stats[i].bssid, bssid, 6) == 0) {
            return &stats[i];
        }
    }
    return nullptr;
}

void Learning::guardarEnSD() {
    if (!SD.begin(SD_CS_PIN)) return;
    File file = SD.open("/redes_conocidas.bin", FILE_WRITE);
    if (file) {
        file.write((uint8_t*)&numStats, sizeof(numStats));
        file.write((uint8_t*)stats, sizeof(RedStats) * numStats);
        file.close();
    }
}

void Learning::cargarDesdeSD() {
    if (!SD.begin(SD_CS_PIN)) {
        numStats = 0;
        return;
    }

    File file = SD.open("/redes_conocidas.bin", FILE_READ);
    if (file) {
        file.read((uint8_t*)&numStats, sizeof(numStats));
        if (numStats > MAX_REDES_ESCANEO) numStats = MAX_REDES_ESCANEO;
        file.read((uint8_t*)stats, sizeof(RedStats) * numStats);
        file.close();
    } else {
        numStats = 0;
    }
}
