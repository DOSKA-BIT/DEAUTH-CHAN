#include "PCAPWriter.h"

bool PCAPWriter::begin(const char* filename) {
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Card failed");
        return false;
    }
    
    // Crear nombre único con timestamp
    char fullPath[64];
    sprintf(fullPath, "/%s_%lu.pcap", filename, millis());
    
    pcapFile = SD.open(fullPath, FILE_WRITE);
    if (!pcapFile) {
        Serial.println("Failed to open file");
        return false;
    }
    
    // Escribir header global
    PCAPHeader header;
    pcapFile.write((uint8_t*)&header, sizeof(header));
    pcapFile.flush();
    
    fileOpen = true;
    Serial.printf("PCAP started: %s\n", fullPath    // Crear CSV para wardriving
    File csv = SD.open("/wardriving.csv", FILE_WRITE);
    if (csv) {
        csv.println("timestamp,ssid,bssid,rssi,channel,lat,lng,altitude");
        csv.close();
    }
}
