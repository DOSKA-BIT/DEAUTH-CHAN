#include "PCAPWriter.h"
#include "boards/BoardConfig.h"

bool PCAPWriter::begin(const char* filename) {
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("No se pudo montar la SD");
        return false;
    }

    char fullPath[64];
    sprintf(fullPath, "/%s_%lu.pcap", filename, millis());

    pcapFile = SD.open(fullPath, FILE_WRITE);
    if (!pcapFile) {
        Serial.println("No se pudo crear el archivo pcap");
        return false;
    }

    PCAPHeader header;
    pcapFile.write((uint8_t*)&header, sizeof(header));
    pcapFile.flush();
    fileOpen = true;

    Serial.printf("PCAP iniciado: %s\n", fullPath);
    return true;
}

void PCAPWriter::writePacket(const uint8_t* data, uint32_t len, uint32_t ts_sec, uint32_t ts_usec) {
    if (!fileOpen) return;

    PacketHeader pktHeader;
    pktHeader.ts_sec = ts_sec;
    pktHeader.ts_usec = ts_usec;
    pktHeader.incl_len = len;
    pktHeader.orig_len = len;

    pcapFile.write((uint8_t*)&pktHeader, sizeof(pktHeader));
    pcapFile.write(data, len);
    pcapFile.flush();
}

void PCAPWriter::close() {
    if (fileOpen) {
        pcapFile.close();
        fileOpen = false;
    }
}
