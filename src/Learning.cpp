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
    // Mismo tema que en PCAPWriter.cpp y Wardriving.cpp: el end() es
    // necesario para que la SD realmente vuelva a mapear sus pines
    // si el touch se quedo con el bus VSPI compartido.
    SD.end();
    if (!SD.begin(SD_CS_PIN)) return;

    // SD.remove() antes de volver a crear el archivo - bug real
    // reportado por el usuario ("Borrar redes guardadas" dejaba
    // algunas redes viejas despues de reiniciar la placa). Antes esto
    // abria directo con FILE_WRITE sobre el archivo ya existente,
    // confiando en que una escritura mas CORTA que el archivo viejo
    // (como numStats=0 despues de un borrado, contra un archivo que
    // tenia varias redes guardadas) de alguna forma dejaba el resto
    // del archivo en un estado inofensivo. Esa suposicion dependia de
    // como interprete exactamente FILE_WRITE la implementacion de SD
    // de turno (si guarda la posicion en el byte 0 sin truncar el
    // archivo, los bytes viejos que sobran al final quedan colgados;
    // si la apertura fuera de "agregar al final" en vez de
    // sobreescribir desde el principio, el header nuevo ni siquiera
    // queda al principio del archivo) - nada de eso esta garantizado
    // por la documentacion, asi que no hay que confiar en eso. Borrar
    // el archivo entero y crearlo de cero cada vez que se guarda saca
    // esa ambiguedad por completo: lo que queda en la SD es siempre
    // EXACTAMENTE lo que hay en memoria en este momento, ni un byte
    // viejo de mas.
    SD.remove("/redes_conocidas.bin");

    File file = SD.open("/redes_conocidas.bin", FILE_WRITE);
    if (file) {
        file.write((uint8_t*)&numStats, sizeof(numStats));
        file.write((uint8_t*)stats, sizeof(RedStats) * numStats);
        file.close();
    }
}

void Learning::borrarHistorial() {
    numStats = 0;
    guardarEnSD();
}

void Learning::cargarDesdeSD() {
    SD.end();
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
