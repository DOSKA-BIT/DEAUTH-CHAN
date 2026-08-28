#ifndef TOUCH_RESISTIVE_H
#define TOUCH_RESISTIVE_H

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "TouchDriver.h"
#include "boards/BoardConfig.h"

// Wrapper de la libreria de Paul Stoffregen para XPT2046, para las
// placas donde el touch tiene su propio bus VSPI separado del que usa
// la pantalla (2432S028 y 2432S024). Si tu placa comparte el mismo
// bus SPI entre pantalla y touch (caso 3248S035R), esta clase no es
// la que corresponde: usa TouchResistiveSharedBus en su lugar.
class TouchResistive : public TouchDriver {
public:
    TouchResistive()
        : touchSPI(VSPI), ts(TOUCH_CS_PIN, TOUCH_IRQ_PIN) {}

    void begin() override {
        // Mapea el periferico VSPI a los pines fisicos del touch. Si
        // en algun momento se toco la SD (que tambien vive en VSPI,
        // ver el comentario largo en Board_2432S028.h), hay que
        // volver a llamar esto para recuperar el mapeo antes de leer
        // el touch de nuevo.
        touchSPI.begin(TOUCH_SCLK_PIN, TOUCH_MISO_PIN, TOUCH_MOSI_PIN, TOUCH_CS_PIN);
        ts.begin(touchSPI);
        ts.setRotation(rotationActual);
    }

    // Llamar despues de cualquier operacion de SD para recuperar el
    // mapeo de pines del touch sobre el periferico VSPI compartido.
    void reclaimBus() {
        touchSPI.begin(TOUCH_SCLK_PIN, TOUCH_MISO_PIN, TOUCH_MOSI_PIN, TOUCH_CS_PIN);
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
    SPIClass touchSPI;
    XPT2046_Touchscreen ts;
    uint8_t rotationActual = 0;

    static const int RAW_X_MIN = 200;
    static const int RAW_X_MAX = 3900;
    static const int RAW_Y_MIN = 200;
    static const int RAW_Y_MAX = 3900;
};

#endif
