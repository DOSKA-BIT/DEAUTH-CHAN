#ifndef TOUCH_CAPACITIVE_H
#define TOUCH_CAPACITIVE_H

#include <TAMC_GT911.h>
#include "TouchDriver.h"
#include "boards/BoardConfig.h"

// GT911 por I2C para la 3248S035C. Importante: esta placa tiene un
// error de fabrica en el ruteo del pin INT del GT911 (esta atado a
// GND en vez de pasar por el jumper R25 hacia el GPIO que le
// correspondia), asi que la interrupcion por hardware no funciona a
// menos que le hagas la modificacion de soldadura que documenta la
// comunidad. Por eso esta clase no depende de la IRQ para nada y
// hace polling puro llamando read() cada vez, igual que se hace con
// el resto de las placas.
class TouchCapacitive : public TouchDriver {
public:
    TouchCapacitive()
        : gt911(TOUCH_SDA_PIN, TOUCH_SCL_PIN, TOUCH_INT_PIN, TOUCH_RST_PIN,
                TFT_PANEL_WIDTH, TFT_PANEL_HEIGHT) {}

    void begin() override {
        gt911.begin();
        gt911.setRotation(ROTATION_NORMAL);
    }

    TouchPoint read() override {
        TouchPoint p = {0, 0, false};
        gt911.read();
        if (gt911.isTouched && gt911.touches > 0) {
            p.pressed = true;
            p.x = constrain((int)gt911.points[0].x, 0, TFT_PANEL_WIDTH - 1);
            p.y = constrain((int)gt911.points[0].y, 0, TFT_PANEL_HEIGHT - 1);
        }
        return p;
    }

private:
    TAMC_GT911 gt911;
};

#endif
