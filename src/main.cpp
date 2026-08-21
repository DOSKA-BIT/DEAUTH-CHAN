#include <Arduino.h>

// Definir el pin de retroiluminación (cambia según tu placa)
#define TFT_BL 21

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Prueba de retroiluminación...");

    // Encender la retroiluminación
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    Serial.println("Retroiluminación encendida (espera 5 segundos)");
    delay(5000);
    
    // Apagar y encender para comprobar
    digitalWrite(TFT_BL, LOW);
    Serial.println("Apagada");
    delay(2000);
    digitalWrite(TFT_BL, HIGH);
    Serial.println("Encendida de nuevo");
}

void loop() {
    delay(1000);
    Serial.print(".");
}
