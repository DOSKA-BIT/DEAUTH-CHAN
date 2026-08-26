#ifndef GPSMODULE_H
#define GPSMODULE_H

#include <TinyGPS++.h>
#include <HardwareSerial.h>

struct GPSData {
    double lat;
    double lng;
    double altitude;
    int satellites;
    bool valid;
    char timestamp[32];
};

class GPSModule {
public:
    void begin();
    void update();
    GPSData getData();
    bool hasFix();

private:
    TinyGPSPlus gps;
    HardwareSerial* gpsSerial;
    unsigned long lastValidFix = 0;
};

#endif
