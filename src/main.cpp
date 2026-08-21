#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Iniciando prueba de pantalla...");

    // Encender backlight
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);

    // Inicializar pantalla
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setTextSize(3);
    tft.setCursor(40, 100);
    tft.println("HOLA");
    tft.setTextSize(1);
    tft.setCursor(20, 160);
    tft.println("Pantalla OK");
    Serial.println("Texto dibujado");
}

void loop() {
    delay(1000);
    Serial.print(".");
}
