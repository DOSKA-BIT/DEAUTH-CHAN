#ifndef LEARNING_H
#define LEARNING_H

#include <SD.h>
#include "config.h"
#include "boards/BoardConfig.h"

// Guarda cuantas veces vimos cada red a lo largo del tiempo, no para
// nada relacionado con ataques (eso ya no existe en este proyecto),
// sino para que la mascota reaccione distinto ante una red que ve por
// primera vez versus una que ya conoce del barrio.
struct RedStats {
    uint8_t bssid[6];
    char ssid[33];
    int vecesVista;
    int mejorRSSI;
    unsigned long primeraVez;
    unsigned long ultimaVez;
};

class Learning {
public:
    void begin();

    // Devuelve true si la red no estaba en el historial (primera vez
    // que se ve), false si ya la teniamos guardada.
    bool registrarAvistamiento(const RedInfo& red);

    RedStats* getStats(const uint8_t* bssid);
    int getTotalRedesConocidas() { return numStats; }

private:
    RedStats stats[MAX_REDES_ESCANEO];
    int numStats = 0;

    void guardarEnSD();
    void cargarDesdeSD();
    RedStats* buscar(const uint8_t* bssid);
};

#endif
