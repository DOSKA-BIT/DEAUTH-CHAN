#include "Learning.h"

void Learning::begin() {
    loadFromSD();
    Serial.println("Sistema de aprendizaje iniciado");
}

void Learning::registerHandshake(const RedInfo& red) {
    RedStats* s = findOrCreate(red.bssid);
    if (s) {
        s->handshakesCapturados++;
        if (s->ssid[0] == '\0') {
            strncpy(s->ssid, red.ssid, 32);
            s->ssid[32] = '\0';
        }
        saveToSD();
    }
}

void Learning::registerAttack(const RedInfo& red) {
    RedStats* s = findOrCreate(red.bssid);
    if (s) {
        s->intentosAtaque++;
        s->ultimoAtaque = millis();
        saveToSD();
    }
}

RedStats* Learning::getStats(const uint8_t* bssid) {
    return findOrCreate(bssid);
}

void Learning::prioritizeNetworks(RedInfo* redes, int numRedes) {
    for (int i = 0; i < numRedes - 1; i++) {
        for (int j = 0; j < numRedes - i - 1; j++) {
            RedStats* stats1 = getStats(redes[j].bssid);
            RedStats* stats2 = getStats(redes[j+1].bssid);
            int score1 = stats1 ? stats1->handshakesCapturados : 0;
            int score2 = stats2 ? stats2->handshakesCapturados : 0;
            if (score1 < score2) {
                RedInfo temp = redes[j];
                redes[j] = redes[j+1];
                redes[j+1] = temp;
            }
        }
    }
}

RedStats* Learning::findOrCreate(const uint8_t* bssid) {
    for (int i = 0; i < numStats; i++) {
        if (memcmp(stats[i].bssid, bssid, 6) == 0) {
            return &stats[i];
        }
    }
    if (numStats < 20) {
        memcpy(stats[numStats].bssid, bssid, 6);
        stats[numStats].ssid[0] = '\0';
        stats[numStats].handshakesCapturados = 0;
        stats[numStats].intentosAtaque = 0;
        stats[numStats].ultimoAtaque = 0;
        return &stats[numStats++];
    }
    return nullptr;
}

void Learning::saveToSD() {
    if (!SD.begin(SD_CS)) return;
    File file = SD.open("/learning.bin", FILE_WRITE);
    if (file) {
        file.write((uint8_t*)&numStats, sizeof(numStats));
        file.write((uint8_t*)stats, sizeof(RedStats) * numStats);
        file.close();
    }
}

void Learning::loadFromSD() {
    if (!SD.begin(SD_CS)) return;
    File file = SD.open("/learning.bin", FILE_READ);
    if (file) {
        file.read((uint8_t*)&numStats, sizeof(numStats));
        if (numStats > 20) numStats = 20;
        file.read((uint8_t*)stats, sizeof(RedStats) * numStats);
        file.close();
    } else {
        numStats = 0;
    }
}
