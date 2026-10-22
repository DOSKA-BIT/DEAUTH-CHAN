#include "EventLog.h"
#include "boards/BoardConfig.h"

void EventLog::begin() {
    // Antes esto dejaba "logFile" abierto toda la sesion y registrar()
    // escribia directamente ahi. El problema: dibujarPantallaConfig()
    // y dibujarPantallaWardrive() hacen SD.end()/SD.begin() cada vez
    // que se dibujan (para chequear "SD: OK" o contar lineas del CSV),
    // y exportKML() tambien - ese SD.end() desmonta la tarjeta y
    // invalida cualquier File que haya quedado abierto de antes.
    // Resultado: apenas se visitaba Config o Wardrive una vez, los
    // eventos siguientes dejaban de anotarse en eventos.csv, sin
    // ningun error visible (logFile.print() en un handle invalido no
    // crashea, simplemente no escribe nada). Ahora cada registrar()
    // abre, escribe y cierra de nuevo (ver mas abajo), igual que ya
    // hace Learning::guardarEnSD() - mas remontajes de SD, pero
    // ninguno queda "viejo" esperando a romperse con el proximo toque
    // a otra pantalla.
    SD.end();
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD no disponible para el log de eventos");
        return;
    }

    if (!SD.exists("/eventos.csv")) {
        File f = SD.open("/eventos.csv", FILE_WRITE);
        if (f) {
            f.println("timestamp,evento");
            f.close();
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

    // Se remonta, abre, escribe y cierra en cada llamada (ver el
    // comentario largo en begin()) en vez de reusar un File que haya
    // quedado abierto de antes - asi nunca importa si otra pantalla
    // desmonto la SD entre medio.
    SD.end();
    if (!SD.begin(SD_CS_PIN)) return;
    File f = SD.open("/eventos.csv", FILE_APPEND);
    if (!f) return;
    f.print(eventos[0].timestamp);
    f.print(",");
    f.println(texto);
    f.flush();
    f.close();
}

EventoLog* EventLog::getEventos(int& count) {
    count = numEventos;
    return eventos;
}
