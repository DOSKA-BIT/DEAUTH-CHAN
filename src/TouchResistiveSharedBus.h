#ifndef TOUCH_RESISTIVE_SHARED_BUS_H
#define TOUCH_RESISTIVE_SHARED_BUS_H

#include <TFT_eSPI.h>
#include "TouchDriver.h"
#include "boards/BoardConfig.h"

// En la 3248S035R el XPT2046 esta en el mismo bus fisico que la
// pantalla (mismos MISO/MOSI/SCLK), solo cambia el CS. Instanciar una
// libreria de touch aparte con su propio SPIClass pisaria el
// periferico que ya esta usando TFT_eSPI, asi que en esta placa hay
// que usar el soporte de touch que trae la propia TFT_eSPI (activado
// con TOUCH_CS en el User_Setup de esta placa) en vez de
// XPT2046_Touchscreen.
class TouchResistiveSharedBus : public TouchDriver {
public:
    explicit TouchResistiveSharedBus(TFT_eSPI* display) : tft(display) {}

    void begin() override {
        // La calibracion de fabrica de estas placas varia bastante
        // mas que en la 2432S028, así que si el touch te queda
        // corrido conviene correr el sketch de calibracion de
        // TFT_eSPI (touch_calibrate) y pisar estos valores.
        uint16_t calData[5] = { 300, 3600, 300, 3600, 7 };
        tft->setTouch(calData);
    }

    TouchPoint read() override {
        TouchPoint p = {0, 0, false};
        uint16_t x, y;
        if (tft->getTouch(&x, &y)) {
            p.pressed = true;
            p.x = constrain((int)x, 0, TFT_PANEL_WIDTH - 1);
            p.y = constrain((int)y, 0, TFT_PANEL_HEIGHT - 1);
        }
        return p;
    }

private:
    TFT_eSPI* tft;
};

#endif
