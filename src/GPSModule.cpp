#include "GPSModule.h"

void GPSModule::begin() {
    // NEO-6M en Serial2: TX=GPIO17, RX=GPIO16
    gpsSerial = &Serial2;
    gpsSerial->begin(9600, SERIAL_8N1, 16, 17);
    
    Serial.println("GPS iniciado en Serial2 (TX=17, RX=16)");
}

void GPSModule::update() {
    while (gpsSerial->available() > 0) {
        gps.encode(gpsSerial->read());
    }
}

GPSData GPSModule::getData() {
    GPSData data;
    
    if (gps.location.isValid()) {
        data.lat = gps.location.lat();
        data.lng = gps.location.lng();
        data.altitude = gps.altitude.isValid() ? gps.altitude.meters() : 0;
        data.satellites = gps.satellites.value();
        data.valid = true;
        lastValidFix = millis();
        
        // Crear timestamp
        if (gps.date.isValid() && gps.time.isValid()) {
            sprintf(data.timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
                gps.date.year(),
                gps.date.month(),
                gps.date.day(),
                gps.time.hour(),
                gps.time.minute(),
                gps.time.second()
            );
        } else {
            strcpy(data.timestamp, "TIME_INVALID");
        }
    } else {
        data.lat = 0;
        data.lng = 0;
        data.altitude = 0;
        data.satellites = 0;
        data.valid = false;
        strcpy(data.timestamp, "NO_FIX");
    }
    
    return data;
}

bool GPSModule::hasFix() {
    return gps.location.isValid() && gps.satellites.value() >= 4;
}
