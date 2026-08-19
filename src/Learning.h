#ifndef LEARNING_H
#define LEARNING_H

#include <SD.h>
#include "config.h"

struct RedStats {
    uint8_t bssid[6];
    char ssid[33];
    int handshakesCapturados;
    int intentosAtaque;
    unsigned long ultimoAtaque;
};

class Learning {
public:
    void begin();
    void registerHandshake(const RedInfo& red);
    void registerAttack(const RedInfo& red);
    RedStats* getStats(const uint8_t* bssid);
    void saveStats();
    void loadStats();
    void prioritizeNetworks(RedInfo* redes, int numRedes);
    
private:
    RedStats stats[20];
    int numStats = 0;
    void saveToSD();
    void loadFromSD();
    RedStats* findOrCreate(const uint8_t* bssid);
};

#endif
