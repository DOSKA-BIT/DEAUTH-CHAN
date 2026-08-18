#include "PCAPWriter.h"
#include "config.h"   // 

bool PCAPWriter::begin(const char* filename) {
    // Intento montar la SD
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Card falló, revisa conexiones");
        return false;
    }
    
    // Nombre del archivo con timestamp para no pisar
    char fullPath[64];
    sprintf(fullPath, "/%s_%lu.pcap", filename, millis());
    
    pcapFile = SD.open(fullPath, FILE_WRITE);
    if (!pcapFile) {
        Serial.println("No pude abrir el archivo .pcap");
        return false;
    }
    
    // Escribir el header global
    PCAPHeader header;
    pcapFile.write((uint8_t*)&header, sizeof(header));
    pcapFile.flush();
    
    fileOpen = true;
    Serial.printf("PCAP iniciado: %s\n", fullPath);   
    
    // Crear CSV para wardriving si no existe
    File csv = SD.open("/wardriving.csv", FILE_WRITE);
    if (csv) {
        csv.println("timestamp,ssid,bssid,rssi,channel,lat,lng,altitude");
        csv.close();
        Serial.println("CSV de wardriving creado");
    }
    
    return true;   // importante: devolver true
}

// Esto lo dejaré vacío por ahora, pero hay que implementarlo si se usa
void PCAPWriter::writePacket(const uint8_t* data, uint32_t len, uint32_t ts_sec, uint32_t ts_usec) {
    if (!fileOpen) return;
    // Aquí iría la escritura del paquete... pero lo dejo para después
    // TODO: implementar escritura de paquetes
}

void PCAPWriter::close() {
    if (fileOpen) {
        pcapFile.close();
        fileOpen = false;
    }
}
