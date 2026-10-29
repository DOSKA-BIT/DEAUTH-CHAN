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

    // Puntero directo al array interno, para la pantalla de "redes
    // conocidas" que necesita recorrerlas todas y ordenarlas por
    // señal. No es lo mas prolijo exponer el array asi nomas, pero
    // para un proyecto de un ESP32 alcanza y sobra sin tener que
    // armar un iterador como la gente.
    RedStats* getTodas() { return stats; }

    // Guarda el historial completo en la SD. Antes se llamaba sola
    // desde registrarAvistamiento() en cada red del escaneo - si
    // encontrabas 15 redes, eso eran 15 remontajes de SD seguidos.
    // Ahora el que llama a registrarAvistamiento() en un loop se
    // encarga de llamar a esto una sola vez al final.
    void guardarEnSD();

    // Vacia el historial completo, en RAM y en la SD - para el boton
    // "Borrar redes guardadas" de AJUSTES. Es una forma de limpiar la
    // memoria de redes conocidas sin tener que reflashear ni sacar la
    // tarjeta a mano. Reutiliza guardarEnSD() (con numStats en 0) en
    // vez de un SD.remove() aparte: cargarDesdeSD() solo lee
    // "numStats" registros del archivo, asi que un numStats=0 alcanza
    // para que el proximo arranque no traiga nada de vuelta, sin
    // importar si quedan bytes viejos despues en el archivo.
    void borrarHistorial();

private:
    RedStats stats[MAX_REDES_ESCANEO];
    int numStats = 0;

    void cargarDesdeSD();
    RedStats* buscar(const uint8_t* bssid);
};

#endif
