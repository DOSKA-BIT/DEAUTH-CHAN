#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <SD.h>
#include <FS.h>
#include "config.h"
#include "GPSModule.h"

// Cuantos eventos recientes se guardan en RAM para mostrar en
// pantalla. Es una lista chica a proposito (no un historial completo
// en memoria): el historial completo de la sesion queda en la SD,
// esto es solo lo ultimo para consultar sin tener que sacar la
// tarjeta y abrir el archivo en otro lado.
#define MAX_EVENTOS_RAM 8

struct EventoLog {
    char timestamp[20];
    char texto[40];
};

// Historial de eventos de la sesion: escaneos completados (con
// cuantas redes nuevas trajeron), entradas y salidas del modo sleep,
// y el inicio del sistema. No registra cada red individual - eso ya
// se guarda aparte en wardriving.csv y en la memoria de Learning, acá
// solo van los eventos "grandes" para no terminar escribiendo la SD
// veinte veces por escaneo como pasaba antes en Learning.
class EventLog {
public:
    void begin();

    // pos es opcional: si viene con una hora de GPS valida se usa esa
    // (asi el log tiene hora real y no "tiempo encendido"), si no se
    // guarda el uptime como "T+HH:MM:SS". Nunca se guarda un millis()
    // crudo como si fuera una fecha real.
    void registrar(const char* texto, const GPSData* pos = nullptr);

    EventoLog* getEventos(int& count);

private:
    EventoLog eventos[MAX_EVENTOS_RAM];
    int numEventos = 0;
    File logFile;
    bool sdOk = false;
};

#endif
