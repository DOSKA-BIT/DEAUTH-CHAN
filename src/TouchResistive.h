#ifndef TOUCH_RESISTIVE_H
#define TOUCH_RESISTIVE_H

#include <XPT2046_Touchscreen.h>
#include "TouchDriver.h"
#include "boards/BoardConfig.h"

// Wrapper de la libreria de Paul Stoffregen para XPT2046. La calibracion
// de fabrica de estas placas chinas viene bastante consistente entre
// unidades, pero si tu pantalla queda corrida un par de pixeles los
// valores RAW_MIN/MAX de abajo son los que hay que tocar primero.
class TouchResistive : public TouchDriver {
public:
    TouchResistive() : ts(TOUCH_CS_PIN, TOUCH_IRQ_PIN) {}

    void begin() override {
        ts.begin();
        ts.setRotation(rotationActual);
    }

    void setRotation(uint8_t rotacion) {
        rotationActual = rotacion;
        ts.setRotation(rotacion);
    }

    TouchPoint read() override {
        TouchPoint p = {0, 0, false};
        if (!ts.touched()) {
            return p;
        }

        TS_Point raw = ts.getPoint();
        p.pressed = true;
        p.x = map(raw.x, RAW_X_MIN, RAW_X_MAX, 0, TFT_PANEL_WIDTH);
        p.y = map(raw.y, RAW_Y_MIN, RAW_Y_MAX, 0, TFT_PANEL_HEIGHT);
        p.x = constrain(p.x, 0, TFT_PANEL_WIDTH - 1);
        p.y = constrain(p.y, 0, TFT_PANEL_HEIGHT - 1);
        return p;
    }

private:
    XPT2046_Touchscreen ts;
    uint8_t rotationActual = 0;

    // Rango tipico de los modulos XPT2046 que traen las CYD de fabrica.
    static const int RAW_X_MIN = 200;
    static const int RAW_X_MAX = 3900;
    static const int RAW_Y_MIN = 200;
    static const int RAW_Y_MAX = 3900;
};

#endif
