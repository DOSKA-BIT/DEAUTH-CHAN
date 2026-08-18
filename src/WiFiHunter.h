#ifndef WIFIHUNTER_H
#define WIFIHUNTER_H

#include <WiFi.h>
#include "config.h"

typedef void (*HandshakeCallback)(const uint8_t* frame, uint32_t len);
#define MAX_PENDING_HS 5

struct PendingHS {
    uint8_t frame[256];
    uint32_t len;
};

// Estructura para almacenar información de un cliente
struct ClienteInfo {
    uint8_t mac[6];
    int rssi;
};

class WiFiHunter {
public:
    void begin();
    void startScan();
    bool isScanDone();
    void getScanResults(RedInfo* resultados, int maxRedes, int& encontradas);
    
    // NUEVO: escanea clientes de una red específica
    void scanClients(const uint8_t* bssid, ClienteInfo* clientes, int maxClientes, int& encontrados);
    
    // NUEVO: deauth mejorado con ráfaga y canal automático
    void deauth(const RedInfo& red, const uint8_t* clienteMac = nullptr, int numPaquetes = 20);
    
    void setHandshakeCallback(HandshakeCallback cb);
    void processPendingHandshakes();
    
private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    
    PendingHS pending[MAX_PENDING_HS];
    volatile int head = 0;
    volatile int tail = 0;
    
    static HandshakeCallback handshakeCB;
    static WiFiHunter* instance;
    
    // NUEVO: método interno para enviar un frame de deauth
    void sendDeauthFrame(const uint8_t* bssid, const uint8_t* clienteMac, int channel);
};

#endif
