#include "Mascota.h"

Mascota::Mascota() {
    estadoActual = ESTADO_IDLE;
    estadoAnterior = ESTADO_IDLE;
    lastFrame = 0;
    estadoStartTime = 0;
    frameAnimacion = 0;
    handshakes = 0;
    redesEncontradas = 0;
}

void Mascota::init(TFT_eSPI *tft) {
    display = tft;
    // Crear el sprite del tamaño del área de la mascota (160x180)
    sprite.createSprite(160, 180);
    sprite.setColorDepth(16);
    display->fillScreen(TFT_BLACK);
}

void Mascota::update() {
    unsigned long ahora = millis();
    
    // Cambio de frame de animación
    if (ahora - lastFrame > ANIM_FRAME_MS) {
        frameAnimacion++;
        lastFrame = ahora;
    }
    
    // Auto-volver a IDLE desde HAPPY después de 3 segundos
    if (estadoActual == ESTADO_HAPPY && ahora - estadoStartTime > 3000) {
        setEstado(ESTADO_IDLE);
    }
    
    // Auto-volver a IDLE desde ATTACK después de 2 segundos
    if (estadoActual == ESTADO_ATTACK && ahora - estadoStartTime > 2000) {
        setEstado(ESTADO_IDLE);
    }
}

void Mascota::setEstado(EstadoMascota nuevo) {
    if (estadoActual != nuevo) {
        estadoAnterior = estadoActual;
        estadoActual = nuevo;
        estadoStartTime = millis();
        frameAnimacion = 0;
    }
}

void Mascota::dibujar() {
    // Limpiamos el sprite
    sprite.fillSprite(TFT_BLACK);
    
    // Dibujamos todo sobre el sprite
    dibujarSprite();
    dibujarUI();
    
    // Volcamos el sprite a la pantalla en la posición (80, 100)
    display->pushSprite(80, 100, 0, 0, 160, 180);
}

void Mascota::dibujarUI() {
    // Barra superior con estadísticas (fuera del sprite, directamente en la pantalla)
    display->fillRect(0, 0, 240, 30, TFT_DARKGREY);
    display->setTextColor(TFT_WHITE);
    display->setTextSize(1);
    
    // Handshakes
    display->setCursor(5, 5);
    display->print("HS:");
    display->print(handshakes);
    
    // Redes encontradas
    display->setCursor(80, 5);
    display->print("RED:");
    display->print(redesEncontradas);
    
    // Estado actual
    display->setCursor(150, 5);
    switch(estadoActual) {
        case ESTADO_IDLE: display->print("IDLE"); break;
        case ESTADO_SCANNING: display->print("SCAN"); break;
        case ESTADO_HAPPY: display->print("NICE!"); break;
        case ESTADO_ATTACK: display->print("ATT!"); break;
        case ESTADO_SLEEP: display->print("Zzz"); break;
        default: display->print("???");
    }
    
    // Barra de humor (basada en handshakes, solo para divertirnos)
    display->drawRect(5, 20, 100, 6, TFT_WHITE);
    int humorWidth = (handshakes * 10) % 100;
    if (humorWidth > 100) humorWidth = 100;
    display->fillRect(6, 21, humorWidth, 4, TFT_GREEN);
}

// ---- Dibujos de la mascota (todo en el sprite) ----

void Mascota::dibujarSprite() {
    switch(estadoActual) {
        case ESTADO_IDLE: dibujarIdle(); break;
        case ESTADO_SCANNING: dibujarScanning(); break;
        case ESTADO_HAPPY: dibujarHappy(); break;
        case ESTADO_ATTACK: dibujarAttack(); break;
        case ESTADO_SLEEP: dibujarSleep(); break;
        default: dibujarIdle(); break;
    }
}

void Mascota::dibujarCaraBase(uint16_t color) {
    int x = 24;   // dentro del sprite (160x180), centrado en 80+?
    int y = 20;   // ajustado para que quede bien
    
    // Cabeza redonda
    sprite.fillRoundRect(x, y, 64, 64, 16, color);
    sprite.drawRoundRect(x, y, 64, 64, 16, TFT_WHITE);
    
    // Brillo (detalle simpático)
    sprite.drawLine(x + 10, y + 5, x + 25, y + 5, TFT_WHITE);
    sprite.drawPixel(x + 9, y + 6, TFT_WHITE);
}

void Mascota::dibujarIdle() {
    int x = 24, y = 20;
    dibujarCaraBase(TFT_GREEN);
    
    // Ojos parpadeantes (cada 8 frames, 2 frames cerrados)
    if (frameAnimacion % 8 < 2) {
        // Parpadeo: ojos cerrados (líneas)
        sprite.drawLine(x + 12, y + 20, x + 20, y + 20, TFT_BLACK);
        sprite.drawLine(x + 44, y + 20, x + 52, y + 20, TFT_BLACK);
    } else {
        // Ojos abiertos
        sprite.fillCircle(x + 16, y + 20, 4, TFT_WHITE);
        sprite.fillCircle(x + 48, y + 20, 4, TFT_WHITE);
        sprite.fillCircle(x + 16, y + 20, 2, TFT_BLACK);
        sprite.fillCircle(x + 48, y + 20, 2, TFT_BLACK);
    }
    
    // Sonrisa tranquila
    sprite.drawLine(x + 20, y + 40, x + 32, y + 44, TFT_BLACK);
    sprite.drawLine(x + 32, y + 44, x + 44, y + 40, TFT_BLACK);
}

void Mascota::dibujarScanning() {
    int x = 24, y = 20;
    dibujarCaraBase(TFT_CYAN);
    
    // Ojos enfocados (más grandes)
    sprite.fillCircle(x + 16, y + 20, 5, TFT_WHITE);
    sprite.fillCircle(x + 48, y + 20, 5, TFT_WHITE);
    sprite.fillCircle(x + 16, y + 20, 2, TFT_BLACK);
    sprite.fillCircle(x + 48, y + 20, 2, TFT_BLACK);
    
    // Antena giratoria (efecto radar)
    int offset = (frameAnimacion % 4) * 4;
    sprite.drawLine(x + 32, y, x + 32 + offset - 6, y - 12, TFT_YELLOW);
    sprite.drawLine(x + 32, y, x + 32 - offset + 6, y - 12, TFT_YELLOW);
    
    // Boca seria (recta)
    sprite.drawLine(x + 24, y + 45, x + 40, y + 45, TFT_BLACK);
}

void Mascota::dibujarHappy() {
    int x = 24, y = 20;
    dibujarCaraBase(TFT_YELLOW);
    
    // Ojos felices (forma de ^ ^)
    sprite.drawLine(x + 10, y + 16, x + 18, y + 24, TFT_BLACK);
    sprite.drawLine(x + 18, y + 24, x + 26, y + 16, TFT_BLACK);
    sprite.drawLine(x + 42, y + 16, x + 50, y + 24, TFT_BLACK);
    sprite.drawLine(x + 50, y + 24, x + 58, y + 16, TFT_BLACK);
    
    // Boca abierta (simulamos con círculo + rectángulo)
    sprite.fillCircle(x + 32, y + 42, 8, TFT_RED);
    sprite.fillRect(x + 24, y + 42, 16, 4, TFT_RED);
    
    // Corazones flotando (animados)
    if (frameAnimacion % 2 == 0) {
        sprite.fillCircle(x + 70, y - 10 - (frameAnimacion % 16), 3, TFT_RED);
        sprite.fillCircle(x + 74, y - 10 - (frameAnimacion % 16), 3, TFT_RED);
        sprite.fillTriangle(x + 67, y - 7 - (frameAnimacion % 16), 
                            x + 77, y - 7 - (frameAnimacion % 16), 
                            x + 72, y - 2 - (frameAnimacion % 16), TFT_RED);
    }
}

void Mascota::dibujarAttack() {
    int x = 24, y = 20;
    dibujarCaraBase(TFT_RED);
    
    // Ojos enojados (cejas fruncidas)
    sprite.drawLine(x + 8, y + 12, x + 24, y + 20, TFT_BLACK);
    sprite.drawLine(x + 40, y + 20, x + 56, y + 12, TFT_BLACK);
    sprite.fillCircle(x + 16, y + 22, 3, TFT_BLACK);
    sprite.fillCircle(x + 48, y + 22, 3, TFT_BLACK);
    
    // Boca gritando (círculo + rectángulo en negro y rojo)
    sprite.fillCircle(x + 32, y + 46, 8, TFT_BLACK);
    sprite.fillCircle(x + 32, y + 46, 5, TFT_RED);
    
    // Rayos de ataque (líneas amarillas)
    sprite.drawLine(x + 64, y + 16, x + 90, y - 8, TFT_YELLOW);
    sprite.drawLine(x + 64, y + 32, x + 85, y + 16, TFT_YELLOW);
    sprite.drawLine(x + 64, y + 48, x + 80, y + 40, TFT_YELLOW);
}

void Mascota::dibujarSleep() {
    int x = 24, y = 20;
    
    // Cabeza azul oscuro (dormido)
    sprite.fillRoundRect(x, y, 64, 64, 16, TFT_NAVY);
    sprite.drawRoundRect(x, y, 64, 64, 16, TFT_BLUE);
    
    // Ojos cerrados (líneas horizontales)
    sprite.drawLine(x + 12, y + 22, x + 20, y + 22, TFT_WHITE);
    sprite.drawLine(x + 44, y + 22, x + 52, y + 22, TFT_WHITE);
    
    // Boca pequeña (un punto)
    sprite.drawPixel(x + 32, y + 42, TFT_WHITE);
    
    // Zzz animados
    int zOffset = (frameAnimacion % 16);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextSize(2);
    sprite.setCursor(x + 70, y - 20 - zOffset);
    sprite.print("Z");
    if (zOffset > 5) {
        sprite.setCursor(x + 80, y - 30 - zOffset + 5);
        sprite.print("Z");
    }
    if (zOffset > 10) {
        sprite.setCursor(x + 90, y - 40 - zOffset + 10);
        sprite.print("Z");
    }
}

// ---- Métodos de interacción ----

void Mascota::incrementarHandshakes() {
    handshakes++;
    setEstado(ESTADO_HAPPY);
}

void Mascota::setRedesEncontradas(int cantidad) {
    redesEncontradas = cantidad;
}

void Mascota::tocar(int tx, int ty) {
    // Las coordenadas del sprite en pantalla: (80,100) a (240, 280)
    if (tx >= 80 && tx <= 240 && ty >= 100 && ty <= 280) {
        setEstado(ESTADO_HAPPY);   // al tocarla se pone feliz :D
    }
}
