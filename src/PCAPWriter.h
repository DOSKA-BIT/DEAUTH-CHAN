#ifndef PCAPWRITER_H
#define PCAPWRITER_H

#include <SD.h>
#include <FS.h>

class PCAPWriter {
public:
    bool begin(const char* filename);
    void writePacket(const uint8_t* data, uint32_t len, uint32_t ts_sec, uint32_t ts_usec);
    void close();
    bool isOpen() { return fileOpen; }
    
private:
    File pcapFile;
    bool fileOpen = false;
    
    // PCAP Global Header (24 bytes)
    struct PCAPHeader {
        uint32_t magicNumber = 0xa1b2c3d4;  // Microsegundos
        uint16_t versionMajor = 2;
        uint16_t versionMinor = 4;
        int32_t  thiszone = 0;              // GMT
        uint32_t sigfigs = 0;
        uint32_t snaplen = 65535;
        uint32_t network = 127;              // 127 = IEEE 802.11
    };
    
    // PCAP Packet Header (16 bytes)
    struct PacketHeader {
        uint32_t ts_sec;
        uint32_t ts_usec;
        uint32_t incl_len;
        uint32_t orig_len;
    };
};

#endif
