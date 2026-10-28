#ifndef MASCOTA_CLASS_H
#define MASCOTA_CLASS_H

#include <TFT_eSPI.h>
#include "config.h"

class Mascota {
public:
    Mascota();
    void init(TFT_eSPI* tft);
    void update();
    void setEstado(EstadoMascota estado);
    EstadoMascota getEstado() const { return estadoActual; }

    // Dibuja todo: la barra de arriba y la cara grande. Es lo que se
    // usa en la pantalla principal, que es "la mascota" en si.
    //
    // limpiarTodo=true borra antes TODA la franja de contenido (hace
    // falta al entrar a esta pantalla, para no dejar restos de la
    // pantalla anterior). En los redibujados normales (solo para
    // animar) no hace falta: el encabezado y el sprite ya limpian su
    // propia zona cada uno, y agregar un borrado de toda la franja
    // ademas de eso es lo que hacia parpadear la pantalla cada vez que
    // tocaba animar (bobbing, cambio de frame, etc).
    void dibujar(bool limpiarTodo = false);

    // Solo la barra de arriba (estado, contador de redes), sin la
    // cara grande. Las demas pantallas (escaneo, stats, etc) la usan
    // para no perder de vista como esta la mascota mientras estas
    // mirando otra cosa, sin que la cara les tape el contenido.
    void dibujarEncabezado();

    void incrementarRedesNuevas();
    void setRedesEncontradas(int cantidad);
    void tocar(int tx, int ty);

    int getRedesNuevas() { return redesNuevas; }
    int getRedesEncontradas() { return redesEncontradas; }

    // Dice si hay que volver a dibujar la CARA GRANDE (solo tiene
    // sentido en la pantalla principal): cambia en cada tick de
    // animacion (bobbing, salto, cambio de frame). Guarda internamente
    // la ultima "firma visual" para comparar, asi que hay que llamarlo
    // una sola vez por vuelta del loop.
    bool necesitaRedibujar();

    // Dice si hay que volver a dibujar el ENCABEZADO (la franja de
    // arriba con REDES/NUEVAS y la pildora de estado). A diferencia de
    // necesitaRedibujar(), esto NO seria cada tick de animacion - el
    // encabezado no tiene nada que se mueva solo, asi que solo hace
    // falta repintarlo cuando el texto que muestra realmente cambio
    // (cambio de estado, redes nuevas o encontradas). Antes el
    // encabezado se repintaba cada vez que la cara grande necesitaba
    // animarse, lo que lo hacia parpadear en TODAS las pantallas
    // (no solo en la principal) cada ~200ms sin necesidad.
    bool necesitaRedibujarEncabezado();

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

    // Ultimos valores con los que se dibujo el encabezado, para
    // detectar cuando hace falta repintarlo (ver
    // necesitaRedibujarEncabezado() arriba).
    EstadoMascota estadoEncabezadoAnterior;
    int redesNuevasEncabezadoAnterior;
    int redesEncontradasEncabezadoAnterior;

    // La cara (ver MascotaCaras.h) se dibuja centrada segun el ancho
    // real de la pantalla (240 en la 2.8/2.4", mas ancho en las
    // 3.5"), asi que se calcula una vez en init() en vez de tenerlo
    // hardcodeado. Los nombres quedan como "sprite" por costumbre del
    // resto del archivo, aunque ahora es un bitmap monocromo chico y
    // no el arte a color de antes.
    int spriteX;
    int spriteY;

    // Pequeno balanceo vertical para que no quede tan tieso parado
    // ahi, calculado a partir del frame de animacion.
    int bobbing() const;

    void dibujarCara();
    const char* nombreEstado() const;
    uint16_t colorEstado() const;
};

#endif
