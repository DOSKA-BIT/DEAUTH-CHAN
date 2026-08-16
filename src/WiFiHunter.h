#ifndef WIFIHUNTER_H
#define WIFIHUNTER_H

#include <WiFi.h>
#include "config.h"

class WiFiHunter {
public:
    void begin();
    void scan(RedInfo* resultados, int maxRedes, int& encontradas);
    void deauth(const RedInfo& red);
    
private:
    bool promiscuoActivado = false;
};

#endif
