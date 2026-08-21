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
    display->fillScreen(TFT_BLACK);
}

void Mascota::update() {
    unsigned long ahora = millis();
    
    if (ahora - lastFrame > ANIM_FRAME_MS) {
        frameAnimacion++;
        lastFrame = ahora;
    }
    
    if (estadoActual == ESTADO_HAPPY && ahora - estadoStartTime > 3000) {
        setEstado(ESTADO_IDLE);
    }
    
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
    display->fillRect(80, 100, 160, 180, TFT_BLACK);
    dibujarSprite();
    dibujarUI();
}

void Mascota::dibujarUI() {
    display->fillRect(0, 0, 240, 30, TFT_DARKGREY);
    display->setTextColor(TFT_WHITE);
    display->setTextSize(1);
    
    display->setCursor(5, 5);
    display->print("HS:");
    display->print(handshakes);
    
    display->setCursor(80, 5);
    display->print("RED:");
    display->print(redesEncontradas);
    
    display->setCursor(150, 5);
    switch(estadoActual) {
        case ESTADO_IDLE: display->print("IDLE"); break;
        case ESTADO_SCANNING: display->print("SCAN"); break;
        case ESTADO_HAPPY: display->print("NICE!"); break;
        case ESTADO_ATTACK: display->print("ATT!"); break;
        case ESTADO_SLEEP: display->print("Zzz"); break;
    }
    
    display->drawRect(5, 20, 100, 6, TFT_WHITE);
    int humorWidth = (handshakes * 10) % 100;
    if (humorWidth > 100) humorWidth = 100;
    display->fillRect(6, 21, humorWidth, 4, TFT_GREEN);
}

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
    int x = 104;
    int y = 120;
    display->fillRoundRect(x, y, 64, 64, 16, color);
    display->drawRoundRect(x, y, 64, 64, 16, TFT_WHITE);
    display->drawLine(x + 10, y + 5, x + 25, y + 5, TFT_WHITE);
    display->drawPixel(x + 9, y + 6, TFT_WHITE);
}

void Mascota::dibujarIdle() {
    int x = 104;
    int y = 120;
    dibujarCaraBase(TFT_GREEN);
    
    if (frameAnimacion % 8 < 2) {
        display->drawLine(x + 12, y + 20, x + 20, y + 20, TFT_BLACK);
        display->drawLine(x + 44, y + 20, x + 52, y + 20, TFT_BLACK);
    } else {
        display->fillCircle(x + 16, y + 20, 4, TFT_WHITE);
        display->fillCircle(x + 48, y + 20, 4, TFT_WHITE);
        display->fillCircle(x + 16, y + 20, 2, TFT_BLACK);
        display->fillCircle(x + 48, y + 20, 2, TFT_BLACK);
    }
    
    display->drawLine(x + 20, y + 40, x + 32, y + 44, TFT_BLACK);
    display->drawLine(x + 32, y + 44, x + 44, y + 40, TFT_BLACK);
}

void Mascota::dibujarScanning() {
    int x = 104;
    int y = 120;
    dibujarCaraBase(TFT_CYAN);
    
    display->fillCircle(x + 16, y + 20, 5, TFT_WHITE);
    display->fillCircle(x + 48, y + 20, 5, TFT_WHITE);
    display->fillCircle(x + 16, y + 20, 2, TFT_BLACK);
    display->fillCircle(x + 48, y + 20, 2, TFT_BLACK);
    
    // ✅ CORREGIDO - Antena girando
    int offset = (frameAnimacion % 4) * 4;
    display->drawLine(x + 32, y - 10, x + 32 + offset, y - 25, TFT_RED);
}

void Mascota::dibujarHappy() {
    int x = 104;
    int y = 120;
    dibujarCaraBase(TFT_YELLOW);
    
    display->fillCircle(x + 16, y + 20, 4, TFT_WHITE);
    display->fillCircle(x + 48, y + 20, 4, TFT_WHITE);
    display->fillCircle(x + 16, y + 20, 2, TFT_BLACK);
    display->fillCircle(x + 48, y + 20, 2, TFT_BLACK);
    
    display->drawLine(x + 20, y + 45, x + 32, y + 50, TFT_BLACK);
    display->drawLine(x + 32, y + 50, x + 44, y + 45, TFT_BLACK);
    
    display->setTextColor(TFT_WHITE);
    display->setTextSize(2);
    display->setCursor(x + 70, y - 10);
    display->print("YAY!");
}

void Mascota::dibujarAttack() {
    int x = 104;
    int y = 120;
    dibujarCaraBase(TFT_RED);
    
    display->drawLine(x + 8, y + 12, x + 24, y + 20, TFT_BLACK);
    display->drawLine(x + 40, y + 20, x + 56, y + 12, TFT_BLACK);
    display->fillCircle(x + 16, y + 22, 3, TFT_BLACK);
    display->fillCircle(x + 48, y + 22, 3, TFT_BLACK);
    
    display->fillEllipse(x + 32, y + 46, 8, 10, TFT_BLACK);
    display->fillEllipse(x + 32, y + 46, 5, 7, TFT_RED);
    
    display->drawLine(x + 64, y + 16, x + 90, y - 8, TFT_YELLOW);
    display->drawLine(x + 64, y + 32, x + 85, y + 16, TFT_YELLOW);
    display->drawLine(x + 64, y + 48, x + 80, y + 40, TFT_YELLOW);
}

void Mascota::dibujarSleep() {
    int x = 104;
    int y = 120;
    
    display->fillRoundRect(x, y, 64, 64, 16, TFT_NAVY);
    display->drawRoundRect(x, y, 64, 64, 16, TFT_BLUE);
    
    display->drawLine(x + 12, y + 22, x + 20, y + 22, TFT_WHITE);
    display->drawLine(x + 44, y + 22, x + 52, y + 22, TFT_WHITE);
    display->drawPixel(x + 32, y + 42, TFT_WHITE);
    
    int zOffset = (frameAnimacion % 16);
    display->setTextColor(TFT_WHITE);
    display->setTextSize(2);
    display->setCursor(x + 70, y - 20 - zOffset);
    display->print("Z");
    
    if (zOffset > 5) {
        display->setCursor(x + 80, y - 30 - zOffset + 5);
        display->print("Z");
    }
    
    if (zOffset > 10) {
        display->setCursor(x + 90, y - 40 - zOffset + 10);
        display->print("Z");
    }
}

void Mascota::incrementarHandshakes() {
    handshakes++;
    setEstado(ESTADO_HAPPY);
}

void Mascota::setRedesEncontradas(int cantidad) {
    redesEncontradas = cantidad;
}

void Mascota::tocar(int tx, int ty) {
    if (tx >= 104 && tx <= 168 && ty >= 120 && ty <= 184) {
        setEstado(ESTADO_HAPPY);
    }
}
