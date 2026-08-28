#ifndef TOUCH_RESISTIVE_SHARED_BUS_H
#define TOUCH_RESISTIVE_SHARED_BUS_H

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "TouchDriver.h"
#include "boards/BoardConfig.h"

// En la 3248S035R el XPT2046 esta en el mismo bus fisico que la
// pantalla (mismos MISO/MOSI/SCLK), solo cambia el CS y tiene su
// propia IRQ. Probamos primero con el soporte de touch integrado de
// TFT_eSPI (getTouch/setTouch activado via TOUCH_CS) y no compilo:
// varios reportes de la comunidad de TFT_eSPI confirman que ese
// camino no es confiable para el XPT2046 de estas placas clonadas.
//
// La alternativa correcta, documentada por el propio autor de
// TFT_eSPI, es reusar el SPIClass que la libreria ya abrio para la
// pantalla (via tft.getSPIinstance()) y pasarselo a
// XPT2046_Touchscreen con su propio CS/IRQ, en vez de crear un
// SPIClass nuevo que competiria por el mismo periferico de hardware.
class TouchResistiveSharedBus : public TouchDriver {
public:
    explicit TouchResistiveSharedBus(TFT_eSPI* display)
        : tft(display), ts(TOUCH_CS_PIN, TOUCH_IRQ_PIN) {}

    void begin() override {
        // tft->init() ya tiene que haberse llamado antes de esto,
        // sino getSPIinstance() todavia no tiene nada que devolver.
        ts.begin(tft->getSPIinstance());
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
    TFT_eSPI* tft;
    XPT2046_Touchscreen ts;
    uint8_t rotationActual = 0;

    // Rango tipico de los modulos XPT2046 que traen estas placas.
    // Si te queda corrido, este es el primer lugar para ajustar.
    static const int RAW_X_MIN = 200;
    static const int RAW_X_MAX = 3900;
    static const int RAW_Y_MIN = 200;
    static const int RAW_Y_MAX = 3900;
};

#endif
