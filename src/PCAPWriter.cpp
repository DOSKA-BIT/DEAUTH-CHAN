#include "PCAPWriter.h"
#include "boards/BoardConfig.h"

bool PCAPWriter::begin(const char* filename) {
    SD.end();
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("No se pudo montar la SD");
        return false;
    }

    sprintf(path, "/%s_%lu.pcap", filename, millis());

    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        Serial.println("No se pudo crear el archivo pcap");
        return false;
    }

    PCAPHeader header;
    f.write((uint8_t*)&header, sizeof(header));
    f.close();
    listo = true;

    Serial.printf("PCAP iniciado: %s\n", path);
    return true;
}

void PCAPWriter::writePacket(const uint8_t* data, uint32_t len, uint32_t ts_sec, uint32_t ts_usec) {
    // Abre, escribe y cierra en CADA paquete, en vez de mantener
    // abierto entre llamadas el File que abria begin() (que es como
    // estaba esto originalmente, de cuando PCAPWriter todavia no se
    // usaba de verdad en ningun lado). Cualquier OTRA escritura a la
    // SD que pase en el medio - wardriving, eventos, el resumen de
    // sesion, el test de "SD: OK" en Config - hace su propio
    // SD.end()/SD.begin(), y eso invalida en silencio cualquier File
    // que hubieramos dejado abierto de antes: exactamente el mismo
    // bug que ya se habia encontrado y arreglado en Wardriving.cpp/
    // EventLog.cpp, solo que aca pasaria mucho mas seguido (muchos
    // paquetes por segundo en vez de una vez por red encontrada).
    //
    // No hace falta repetir el SD.end()/SD.begin() COMPLETO que usan
    // esas otras funciones (eso tiene sentido cuando se llama una vez
    // por red, no muchas veces por segundo como ESTO) - alcanza con
    // no guardar el File entre llamadas, porque lo unico que se
    // invalida con un remount ajeno es el objeto File en si, no el
    // montaje de la tarjeta (eso lo deja listo begin(), una sola vez,
    // y todo lo demas del firmware ya se encarga de dejarla montada
    // de nuevo despues de su propio SD.end()).
    if (!listo) return;

    File f = SD.open(path, FILE_APPEND);
    if (!f) return;

    PacketHeader pktHeader;
    pktHeader.ts_sec = ts_sec;
    pktHeader.ts_usec = ts_usec;
    pktHeader.incl_len = len;
    pktHeader.orig_len = len;

    f.write((uint8_t*)&pktHeader, sizeof(pktHeader));
    f.write(data, len);
    f.close();
}
