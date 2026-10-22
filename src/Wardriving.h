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
    // Ya no se guarda un File abierto toda la sesion (era "csvFile"
    // antes) - ver el comentario largo en saveNetwork(), en
    // Wardriving.cpp, de por que eso hacia que dejaran de guardarse
    // redes nuevas despues de visitar Config o Wardrive.
    bool filterEnabled = false;
    int filterAuthMode = WIFI_AUTH_OPEN;
};

#endif
