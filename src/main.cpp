#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "boards/BoardConfig.h"

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS_PIN);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Prueba minima de touch - arrancando");
    touchSPI.begin(TOUCH_SCLK_PIN, TOUCH_MISO_PIN, TOUCH_MOSI_PIN, TOUCH_CS_PIN);
    ts.begin(touchSPI);
    Serial.println("Touch inicializado, toca la pantalla...");
}

void loop() {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        Serial.printf("Touch: x=%d y=%d z=%d\n", p.x, p.y, p.z);
    }
    delay(100);
}
