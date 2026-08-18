#ifndef MASCOTA_H
#define MASCOTA_H

#include <TFT_eSPI.h>
#include "config.h"

class Mascota {
public:
    Mascota();
    ~Mascota();  // para liberar memoria del sprite
    void init(TFT_eSPI *tft);
    void update();
    void setEstado(EstadoMascota estado);
    void dibujar();
    
    void incrementarHandshakes();
    void setRedesEncontradas(int cantidad);
    
    int getHandshakes() { return handshakes; }
    int getRedesEncontradas() { return redesEncontradas; }
    
    void tocar(int tx, int ty);
    
private:
    TFT_eSPI *display;
    TFT_eSprite *sprite;   // <-- ahora es un puntero
    
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
