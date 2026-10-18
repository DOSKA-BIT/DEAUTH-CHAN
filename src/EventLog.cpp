#include "EventLog.h"
#include "boards/BoardConfig.h"

void EventLog::begin() {
    SD.end();
    sdOk = SD.begin(SD_CS_PIN);
    if (!sdOk) {
        Serial.println("SD no disponible para el log de eventos");
        return;
    }

    logFile = SD.open("/eventos.csv", FILE_APPEND);
    if (!logFile) {
        logFile = SD.open("/eventos.csv", FILE_WRITE);
        if (logFile) {
            logFile.println("timestamp,evento");
            logFile.close();
            logFile = SD.open("/eventos.csv", FILE_APPEND);
        }
    }
}

void EventLog::registrar(const char* texto, const GPSData* pos) {
    // Shift simple en vez de un indice de buffer circular: con solo 8
    // lugares mover el array es barato (8 copias de una struct de
    // ~60 bytes, nada comparado con lo que ya hace falta para dibujar
    // un frame), y queda mucho mas facil de leer que llevar la cuenta
    // de un indice que da la vuelta.
    for (int i = MAX_EVENTOS_RAM - 1; i > 0; i--) {
        eventos[i] = eventos[i - 1];
    }

    // pos->timestamp puede ser una hora real ("2026-10-18 13:05:02"),
    // o los textos fijos "NO_FIX"/"TIME_INVALID" cuando el GPS todavia
    // no tiene esa info. Una hora real siempre arranca con un digito,
    // asi que alcanza con mirar el primer caracter para decidir si
    // vale la pena usarla o si conviene caer al uptime.
    if (pos != nullptr && pos->timestamp[0] >= '0' && pos->timestamp[0] <= '9') {
        strncpy(eventos[0].timestamp, pos->timestamp, sizeof(eventos[0].timestamp) - 1);
    } else {
        unsigned long segundos = millis() / 1000;
        int horas = segundos / 3600;
        int minutos = (segundos % 3600) / 60;
        int segs = segundos % 60;
        snprintf(eventos[0].timestamp, sizeof(eventos[0].timestamp), "T+%02d:%02d:%02d", horas, minutos, segs);
    }
    eventos[0].timestamp[sizeof(eventos[0].timestamp) - 1] = '\0';

    strncpy(eventos[0].texto, texto, sizeof(eventos[0].texto) - 1);
    eventos[0].texto[sizeof(eventos[0].texto) - 1] = '\0';

    if (numEventos < MAX_EVENTOS_RAM) numEventos++;

    if (sdOk && logFile) {
        logFile.print(eventos[0].timestamp);
        logFile.print(",");
        logFile.println(texto);
        logFile.flush();
    }
}

EventoLog* EventLog::getEventos(int& count) {
    count = numEventos;
    return eventos;
}
