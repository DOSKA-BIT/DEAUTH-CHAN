#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <Arduino.h>

// Punto tactil ya mapeado a coordenadas de pantalla (0..ancho, 0..alto),
// no a los valores crudos del ADC. Cada implementacion se encarga de
// hacer ese mapeo internamente.
struct TouchPoint {
    int x;
    int y;
    bool pressed;
};

// Interfaz comun para que main.cpp no le importe si abajo hay un
// resistivo XPT2046 o (fase 2) un capacitivo GT911/CST816. Cada placa
// instancia la implementacion que le corresponde segun TOUCH_IS_CAPACITIVE.
class TouchDriver {
public:
    virtual ~TouchDriver() {}
    virtual void begin() = 0;
    virtual TouchPoint read() = 0;
};

#endif
