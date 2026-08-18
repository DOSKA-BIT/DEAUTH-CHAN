#ifndef WIFIHUNTER_H
#define WIFIHUNTER_H

#include <WiFi.h>
#include "config.h"

typedef void (*HandshakeCallback)(const uint8_t* frame, uint32_t len);

#define MAX_PENDING_HS 5
#define MAX_CLIENTES 20

struct PendingHS {
    uint8_t frame[256];
    uint32_t len;
};

struct ClienteInfo {
    uint8_t mac[6];
    int rssi;
    int canal;
    uint8_t bssid[6];
};

class WiFiHunter {
public:
    void begin();
    void startScan();
    bool isScanDone();
    void getScanResults(RedInfo* resultados, int maxRedes, int& encontradas);
    
    void scanClients(const uint8_t* bssid, int canal, ClienteInfo* clientes, int maxClientes, int& encontrados);
    void deauth(const RedInfo& red, const uint8_t* clienteMac = nullptr, int numPaquetes = 20);
    
    void setHandshakeCallback(HandshakeCallback cb);
    void processPendingHandshakes();
    
private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void sendDeauthFrame(const uint8_t* bssid, const uint8_t* clienteMac, int channel);
    
    PendingHS pending[MAX_PENDING_HS];
    volatile int head = 0;
    volatile int tail = 0;
    
    static HandshakeCallback handshakeCB;
    static WiFiHunter* instance;
};

#endif
