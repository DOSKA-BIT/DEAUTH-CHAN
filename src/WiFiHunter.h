#ifndef WIFIHUNTER_H
#define WIFIHUNTER_H

#include <WiFi.h>
#include "config.h"

// Callback que se ejecuta cuando capturamos un handshake
typedef void (*HandshakeCallback)(const uint8_t* frame, uint32_t len);

// Tamaño de la cola de handshakes pendientes
#define MAX_PENDING_HS 5

// Estructura para guardar un handshake en la cola
struct PendingHS {
    uint8_t frame[256];
    uint32_t len;
};

class WiFiHunter {
public:
    void begin();
    void startScan();   // inicia escaneo asíncrono
    bool isScanDone();  // true si el escaneo terminó
    void getScanResults(RedInfo* resultados, int maxRedes, int& encontradas);
    void deauth(const RedInfo& red);
    void setHandshakeCallback(HandshakeCallback cb);
    void processPendingHandshakes();  // procesa los handshakes en cola (llamar desde loop)
    
private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    
    // Cola circular para handshakes
    PendingHS pending[MAX_PENDING_HS];
    volatile int head = 0;
    volatile int tail = 0;
    
    static HandshakeCallback handshakeCB;  // callback global
    static WiFiHunter* instance;           // para acceder desde la callback estática
};

#endif
