#include "WiFiScanner.h"
#include <esp_wifi.h>

WiFiScanner* WiFiScanner::instance = nullptr;

void WiFiScanner::begin() {
    instance = this;
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    Serial.println("Escaner en modo promiscuo (solo escucha)");
}

void WiFiScanner::startScan() {
    esp_wifi_set_promiscuous(false);
    WiFi.scanNetworks(true, false);
}

bool WiFiScanner::isScanDone() {
    return (WiFi.scanComplete() >= 0);
}

void WiFiScanner::getScanResults(RedInfo* resultados, int maxRedes, int& encontradas) {
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
    }
    WiFi.scanDelete();
    esp_wifi_set_promiscuous(true);
    Serial.printf("%d redes encontradas\n", encontradas);
}

void WiFiScanner::contarClientes(const uint8_t* bssid, int canal) {
    memcpy(bssidObjetivo, bssid, 6);
    canalObjetivo = canal;
    numClientes = 0;
    escuchandoClientes = true;

    esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
    Serial.printf("Escuchando trafico en canal %d\n", canal);

    unsigned long start = millis();
    while (millis() - start < 3000) {
        delay(10);
    }

    unsigned long ahora = millis();
    for (int i = 0; i < numClientes; i++) {
        if (ahora - clientes[i].ultimaVez > CLIENTE_TIMEOUT) {
            for (int j = i; j < numClientes - 1; j++) {
                memcpy(&clientes[j], &clientes[j + 1], sizeof(ClienteInfo));
            }
            numClientes--;
            i--;
        }
    }

    escuchandoClientes = false;
    Serial.printf("Dispositivos distintos escuchados: %d\n", numClientes);
}

ClienteInfo* WiFiScanner::getClientes(int& count) {
    count = numClientes;
    return clientes;
}

void WiFiScanner::setLED(bool state) {
    digitalWrite(TFT_BL_PIN, state ? HIGH : LOW);
}

void WiFiScanner::registrarAnomalia(bool esDisassoc, uint8_t* origen, uint8_t* destino, int canal, int rssi) {
    if (esDisassoc) {
        contadorDisassoc++;
    } else {
        contadorDeauth++;
    }

    // Shift simple, igual que en EventLog: con 8 lugares nomas no
    // vale la pena un indice de buffer circular para esto.
    for (int i = MAX_ANOMALIAS_RAM - 1; i > 0; i--) {
        anomalias[i] = anomalias[i - 1];
    }
    anomalias[0].momento = millis();
    memcpy(anomalias[0].origen, origen, 6);
    memcpy(anomalias[0].destino, destino, 6);
    anomalias[0].canal = canal;
    anomalias[0].rssi = rssi;
    anomalias[0].esDisassoc = esDisassoc;

    if (numAnomalias < MAX_ANOMALIAS_RAM) numAnomalias++;
}

void WiFiScanner::getAnomaliasStats(unsigned long& deauth, unsigned long& disassoc) {
    deauth = contadorDeauth;
    disassoc = contadorDisassoc;
}

AnomaliaWiFi* WiFiScanner::getAnomalias(int& count) {
    count = numAnomalias;
    return anomalias;
}

void WiFiScanner::promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!instance) return;

    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t* frame = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;
    int rssi = pkt->rx_ctrl.rssi;

    if (len < 24) return;

    // Monitor pasivo de deauth/disassoc: se fija en esto para TODO
    // paquete que pasa por aca, no solo mientras se esta contando
    // clientes de un BSSID puntual (por eso va antes del "return" de
    // abajo). El frame control son los primeros 2 bytes de cualquier
    // frame 802.11; el tipo esta en los bits 2-3 y el subtipo en los
    // bits 4-7. Tipo 0 = management, subtipo 0x0C = deauth, 0x0A =
    // disassoc.
    uint8_t tipoFrame = (frame[0] >> 2) & 0x3;
    uint8_t subtipoFrame = (frame[0] >> 4) & 0xF;
    if (tipoFrame == 0 && (subtipoFrame == 0x0C || subtipoFrame == 0x0A)) {
        // addr1 = destino (frame[4..9]), addr2 = origen/transmisor
        // (frame[10..15]) - mismos offsets que usa el conteo de
        // clientes de aca abajo.
        instance->registrarAnomalia(subtipoFrame == 0x0A, &frame[10], &frame[4],
            pkt->rx_ctrl.channel, rssi);
    }

    if (!instance->escuchandoClientes) return;

    uint8_t* addr1 = &frame[4];
    uint8_t* addr2 = &frame[10];
    uint8_t* addr3 = &frame[16];

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

void WiFiScanner::actualizarCliente(uint8_t* mac, int rssi, uint8_t* bssid, int canal) {
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
