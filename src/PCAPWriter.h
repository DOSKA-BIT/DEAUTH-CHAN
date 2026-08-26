#ifndef PCAPWRITER_H
#define PCAPWRITER_H

#include <SD.h>
#include <FS.h>

// Escribe frames crudos de management (beacons, probe requests/responses)
// a un archivo .pcap estandar para poder abrirlo despues en Wireshark.
// No hay nada aca que capture handshakes ni trafico de datos, es
// unicamente lo que un escaneo pasivo de wardriving levanta al aire.
class PCAPWriter {
public:
    bool begin(const char* filename);
    void writePacket(const uint8_t* data, uint32_t len, uint32_t ts_sec, uint32_t ts_usec);
    void close();
    bool isOpen() { return fileOpen; }

private:
    File pcapFile;
    bool fileOpen = false;

    struct PCAPHeader {
        uint32_t magicNumber = 0xa1b2c3d4;
        uint16_t versionMajor = 2;
        uint16_t versionMinor = 4;
        int32_t  thiszone = 0;
        uint32_t sigfigs = 0;
        uint32_t snaplen = 65535;
        uint32_t network = 127;  // LINKTYPE_IEEE802_11
    };

    struct PacketHeader {
        uint32_t ts_sec;
        uint32_t ts_usec;
        uint32_t incl_len;
        uint32_t orig_len;
    };
};

#endif
