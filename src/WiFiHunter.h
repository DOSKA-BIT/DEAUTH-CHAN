#ifndef WIFIHUNTER_H
#define WIFIHUNTER_H

#include <WiFi.h>
#include "config.h"

// Typedef para el callback
typedef void (*HandshakeCallback)(const uint8_t* frame, uint32_t len);

class WiFiHunter {
public:
    void begin();
    void scan(RedInfo* resultados, int maxRedes, int& encontradas);
    void deauth(const RedInfo& red);
    void setHandshakeCallback(HandshakeCallback cb);
    
private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
};

#endif
