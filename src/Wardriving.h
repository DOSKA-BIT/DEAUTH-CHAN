#ifndef WARDRIVING_H
#define WARDRIVING_H

#include <SD.h>
#include <FS.h>
#include <WiFi.h>
#include "config.h"
#include "GPSModule.h"

class Wardriving {
public:
    void begin();
    void saveNetwork(const RedInfo& red, const GPSData& pos);

    // Lee wardriving.csv y escribe wardriving.kml con un marcador por
    // cada red que tenga posicion GPS real, coloreado segun la señal.
    // Devuelve cuantos puntos escribio, o -1 si no pudo (sin SD, o sin
    // wardriving.csv todavia).
    int exportKML();

    void setFilterByEncryption(bool enabled, int authMode);

private:
    File csvFile;
    bool filterEnabled = false;
    int filterAuthMode = WIFI_AUTH_OPEN;
    void ensureHeader();
};

#endif
