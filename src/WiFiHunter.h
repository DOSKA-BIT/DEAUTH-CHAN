#ifndef WIFIHUNTER_H
#define WIFIHUNTER_H

#include <WiFi.h>
#include "config.h"

typedef void (*HandshakeCallback)(const uint8_t* frame, uint32_t len);

#define MAX_PENDING_HS 5
#define MAX_CLIENTES 20
#define MAX_PAQUETES_BUFFER 100

struct PendingHS {
    uint8_t frame[256];
    uint32_t len;
};

struct ClienteInfo {
    uint8_t mac[6];
    int rssi;
    int canal;
    uint8_t bssid[6];
    unsigned long lastSeen;
};

struct PaqueteRaw {
    uint8_t data[128];
    uint16_t len;
    int rssi;
    int canal;
    unsigned long timestamp;
};

class WiFiHunter {
public:
    void begin();
    void startScan();
    bool isScanDone();
    void getScanResults(RedInfo* resultados, int maxRedes, int& encontradas);
    
    void startClientScan(const uint8_t* bssid, int canal);
    void stopClientScan();
    bool isClientScanActive();
    void getClients(ClienteInfo* clientes, int maxClientes, int& encontrados);
    
    void deauth(const RedInfo& red, const uint8_t* clienteMac = nullptr, int numPaquetes = 20);
    void deauthAll(const RedInfo& red, int numPaquetes = 10);
    
    void setHandshakeCallback(HandshakeCallback cb);
    void processPendingHandshakes();
    
private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void sendDeauthFrame(const uint8_t* bssid, const uint8_t* clienteMac, int channel);
    void procesarPaquete(const uint8_t* frame, uint16_t len, int rssi, int canal);
    bool esClienteValido(const uint8_t* mac, const uint8_t* bssid);
    void actualizarCliente(const uint8_t* mac, const uint8_t* bssid, int rssi, int canal);
    
    PendingHS pending[MAX_PENDING_HS];
    volatile int head = 0;
    volatile int tail = 0;
    
    ClienteInfo clientes[MAX_CLIENTES];
    int numClientes = 0;
    uint8_t bssidObjetivo[6];
    bool escaneandoClientes = false;
    unsigned long lastClientUpdate = 0;
    
    static HandshakeCallback handshakeCB;
    static WiFiHunter* instance;
};

#endif
