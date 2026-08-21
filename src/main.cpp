#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Iniciando prueba de pantalla...");

    // Inicializar pantalla
    tft.init();
    tft.setRotation(1); // Prueba con 0, 1, 2, 3
    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setTextSize(3);
    tft.setCursor(20, 100);
    tft.println("HOLA");
    tft.setTextSize(1);
    tft.setCursor(10, 150);
    tft.println("Si ves esto, la pantalla funciona");
}

void loop() {
    // Nada, solo mostramos el texto
}
