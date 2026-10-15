#ifndef MASCOTA_H
#define MASCOTA_H

#include <TFT_eSPI.h>
#include "config.h"

class Mascota {
public:
    Mascota();
    void init(TFT_eSPI* tft);
    void update();
    void setEstado(EstadoMascota estado);
    void dibujar();
    void incrementarRedesNuevas();
    void setRedesEncontradas(int cantidad);
    void tocar(int tx, int ty);

    int getRedesNuevas() { return redesNuevas; }
    int getRedesEncontradas() { return redesEncontradas; }

    // Dice si hay que volver a dibujar la mascota: no cada vez que pasa
    // un tick de animacion, sino solo cuando lo que se ve en pantalla
    // cambiaria de verdad. Guarda internamente la ultima "firma visual"
    // para comparar, asi que hay que llamarlo una sola vez por vuelta
    // del loop (no sirve para consultar sin consumir el cambio).
    bool necesitaRedibujar();

private:
    TFT_eSPI* display;
    EstadoMascota estadoActual;
    EstadoMascota estadoAnterior;
    unsigned long lastFrame;
    unsigned long estadoStartTime;
    uint8_t frameAnimacion;
    int redesNuevas;
    int redesEncontradas;

    EstadoMascota estadoFirmaAnterior;
    uint8_t firmaAnterior;
    uint8_t firmaVisual() const;

    // El sprite se dibuja centrado sobre un cuadrado fijo de 64x64, pero
    // la posicion de ese cuadrado depende del ancho real de la pantalla
    // (240 en la 2.8/2.4", mas ancho en las 3.5"), asi que se calcula
    // una vez en init() en vez de tenerlo hardcodeado como en la version vieja.
    int spriteX;
    int spriteY;

    void dibujarSprite();
    void dibujarUI();
    void dibujarIdle();
    void dibujarScanning();
    void dibujarHappy();
    void dibujarCuriosa();
    void dibujarSleep();
    void dibujarCaraBase(uint16_t color);
};

#endif
