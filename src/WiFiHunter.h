#ifndef WIFIHUNTER_H
#define WIFIHUNTER_H

#include <WiFi.h>
#include "config.h"
#include "OUI_DB.h"

#define MAX_PENDING_HS 5
#define MAX_CLIENTES 20
#define CLIENTE_TIMEOUT 10000

typedef void (*HandshakeCallback)(const uint8_t* frame, uint32_t len);

struct PendingHS {
    uint8_t frame[256];
    uint32_t len;
};

struct ClienteInfo {
    uint8_t mac[6];
    int rssi;
    int canal;
    uint8_t bssid[6];
    unsigned long ultimaVez;
    char fabricante[16];
};

class WiFiHunter {
public:
    void begin();
    void startScan();
    bool isScanDone();
    void getScanResults(RedInfo* resultados, int maxRedes, int& encontradas);
    
    void scanClients(const uint8_t* bssid, int canal);
    void deauth(const RedInfo& red, const uint8_t* clienteMac = nullptr, int numPaquetes = 20);
    void setHandshakeCallback(HandshakeCallback cb);
    void processPendingHandshakes();
    
    ClienteInfo* getClientes(int& count);
    
    // Sigilo
    void setSilentMode(bool enable);
    void randomizeMAC();
    void setLED(bool state);
    bool isSilentMode() { return silentMode; }
    
    // Ataques activos
    void beaconFlood(const char* ssid, int numBeacons = 50);
    void sendProbeRequest(const char* ssid);
    void deauthAllClients(const RedInfo& red, int numPaquetes = 30);
    
private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void sendDeauthFrame(const uint8_t* bssid, const uint8_t* clienteMac, int channel);
    void actualizarCliente(uint8_t* mac, int rssi, uint8_t* bssid, int canal);
    
    PendingHS pending[MAX_PENDING_HS];
    volatile int head = 0;
    volatile int tail = 0;
    
    ClienteInfo clientes[MAX_CLIENTES];
    int numClientes = 0;
    uint8_t bssidObjetivo[6];
    int canalObjetivo = 0;
    bool escaneandoClientes = false;
    
    static HandshakeCallback handshakeCB;
    static WiFiHunter* instance;
    
    bool silentMode = false;
};

#endif
