#include <Arduino.h>
#include <TFT_eSPI.h>
#include "boards/BoardConfig.h"

// Sketch minimo para aislar el problema de pantalla: no toca touch, SD,
// WiFi, GPS ni nada del resto del proyecto. Si esto no se ve, el
// problema esta en la configuracion de pines o en el hardware, no en
// el resto del firmware.

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Prueba minima de pantalla - arrancando");

    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);
    Serial.println("Backlight encendido");

    tft.init();
    Serial.println("tft.init() completado");

    tft.setRotation(0);

    tft.fillScreen(TFT_RED);
    Serial.println("Pantalla pintada de ROJO");
    delay(2000);

    tft.fillScreen(TFT_GREEN);
    Serial.println("Pantalla pintada de VERDE");
    delay(2000);

    tft.fillScreen(TFT_BLUE);
    Serial.println("Pantalla pintada de AZUL");
    delay(2000);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.setCursor(10, 100);
    tft.print("HOLA CYD");
    Serial.println("Texto dibujado en blanco sobre negro");
}

void loop() {
    delay(1000);
}
