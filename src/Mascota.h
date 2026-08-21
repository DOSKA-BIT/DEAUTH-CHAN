#ifndef MASCOTA_H
#define MASCOTA_H

#include <TFT_eSPI.h>
#include "config.h"

class Mascota {
public:
    Mascota();
    void init(TFT_eSPI *tft);
    void update();
    void setEstado(EstadoMascota estado);
    void dibujar();
    void incrementarHandshakes();
    void setRedesEncontradas(int cantidad);
    void tocar(int tx, int ty);  // ✅ AGREGADO
    
    int getHandshakes() { return handshakes; }
    int getRedesEncontradas() { return redesEncontradas; }

private:
    TFT_eSPI *display;
    EstadoMascota estadoActual;
    EstadoMascota estadoAnterior;
    unsigned long lastFrame;
    unsigned long estadoStartTime;
    uint8_t frameAnimacion;
    int handshakes;
    int redesEncontradas;
    
    void dibujarSprite();
    void dibujarUI();
    void dibujarIdle();
    void dibujarScanning();
    void dibujarHappy();
    void dibujarAttack();
    void dibujarSleep();
    void dibujarCaraBase(uint16_t color);
};

#endif
