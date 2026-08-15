#ifndef CONFIG_H
#define CONFIG_H

// Pines del CYD (ajusta si tu modelo es diferente)
#define TFT_LED_PIN 21      // Backlight
#define TOUCH_CS 14         // Táctil
#define TOUCH_IRQ 13        // Opcional
#define SD_CS 5             // SD Card

// Config de la mascota
#define ANIM_FRAME_MS 150   // Velocidad de animación
#define SCAN_INTERVAL 3000  // Cada cuánto escanea

// Estados
enum EstadoMascota {
    IDLE =
