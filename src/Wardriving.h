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

    // Cuantas redes hay guardadas en wardriving.csv en total (de esta
    // sesion y las anteriores). Se calcula una vez en begin() y se
    // actualiza con un ++ en cada saveNetwork() exitoso - no relee el
    // archivo, asi que llamar esto desde dibujarPantallaWardrive() en
    // cada redibujado no tiene costo (antes dibujarPantallaWardrive()
    // leia el CSV entero y remontaba la SD cada vez que se mostraba
    // esa pantalla).
    int getContadorRedes() const { return contadorRedes; }

private:
    // Ya no se guarda un File abierto toda la sesion (era "csvFile"
    // antes) - ver el comentario largo en saveNetwork(), en
    // Wardriving.cpp, de por que eso hacia que dejaran de guardarse
    // redes nuevas despues de visitar Config o Wardrive.
    bool filterEnabled = false;
    int filterAuthMode = WIFI_AUTH_OPEN;
    int contadorRedes = 0;
};

#endif
