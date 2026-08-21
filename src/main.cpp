#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Iniciando prueba...");

    // Inicializar la pantalla
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLUE);
    
    // Dibujar texto en blanco
    tft.setTextColor(TFT_WHITE);
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
