#include "Mascota.h"

Mascota::Mascota() {
    estadoActual = ESTADO_IDLE;
    estadoAnterior = ESTADO_IDLE;
    lastFrame = 0;
    estadoStartTime = 0;
    frameAnimacion = 0;
    handshakes = 0;
    redesEncontradas = 0;
    sprite = nullptr;   // inicializamos el puntero
}

Mascota::~Mascota() {
    if (sprite) {
        sprite->deleteSprite();  // liberar memoria
        delete sprite;
    }
}

void Mascota::init(TFT_eSPI *tft) {
    display = tft;
    // Crear el sprite dinámicamente
    sprite = new TFT_eSprite(display);
    sprite->createSprite(160, 180);
    sprite->setColorDepth(16);
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
    if (!sprite) return;  // seguridad
    
    // Limpiar el sprite
    sprite->fillSprite(TFT_BLACK);
    
    // Dibujar la mascota en el sprite
    dibujarSprite();
    
    // Dibujar la UI en la pantalla (fuera del sprite)
    dibujarUI();
    
    // Volcar el sprite a la pantalla en (80, 100)
    sprite->pushSprite(80, 100);
}

void Mascota::dibujarUI() {
    // Barra superior con estadísticas (directamente en la pantalla)
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
        default: display->print("???");
    }
    
    // Barra de humor
    display->drawRect(5, 20, 100, 6, TFT_WHITE);
    int humorWidth = (handshakes * 10) % 100;
    if (humorWidth > 100) humorWidth = 100;
    display->fillRect(6, 21, humorWidth, 4, TFT_GREEN);
}

// ---- Todos los dibujos ahora usan sprite-> en lugar de display-> ----

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
    int x = 24, y = 20;
    sprite->fillRoundRect(x, y, 64, 64, 16, color);
    sprite->drawRoundRect(x, y, 64, 64, 16, TFT_WHITE);
    sprite->drawLine(x + 10, y + 5, x + 25, y + 5, TFT_WHITE);
    sprite->drawPixel(x + 9, y + 6, TFT_WHITE);
}

void Mascota::dibujarIdle() {
    int x = 24, y = 20;
    dibujarCaraBase(TFT_GREEN);
    
    if (frameAnimacion % 8 < 2) {
        sprite->drawLine(x + 12, y + 20, x + 20, y + 20, TFT_BLACK);
        sprite->drawLine(x + 44, y + 20, x + 52, y + 20, TFT_BLACK);
    } else {
        sprite->fillCircle(x + 16, y + 20, 4, TFT_WHITE);
        sprite->fillCircle(x + 48, y + 20, 4, TFT_WHITE);
        sprite->fillCircle(x + 16, y + 20, 2, TFT_BLACK);
        sprite->fillCircle(x + 48, y + 20, 2, TFT_BLACK);
    }
    
    sprite->drawLine(x + 20, y + 40, x + 32, y + 44, TFT_BLACK);
    sprite->drawLine(x + 32, y + 44, x + 44, y + 40, TFT_BLACK);
}

void Mascota::dibujarScanning() {
    int x = 24, y = 20;
    dibujarCaraBase(TFT_CYAN);
    
    sprite->fillCircle(x + 16, y + 20, 5, TFT_WHITE);
    sprite->fillCircle(x + 48, y + 20, 5, TFT_WHITE);
    sprite->fillCircle(x + 16, y + 20, 2, TFT_BLACK);
    sprite->fillCircle(x + 48, y + 20, 2, TFT_BLACK);
    
    int offset = (frameAnimacion % 4) * 4;
    sprite->drawLine(x + 32, y, x + 32 + offset - 6, y - 12, TFT_YELLOW);
    sprite->drawLine(x + 32, y, x + 32 - offset + 6, y - 12, TFT_YELLOW);
    
    sprite->drawLine(x + 24, y + 45, x + 40, y + 45, TFT_BLACK);
}

void Mascota::dibujarHappy() {
    int x = 24, y = 20;
    dibujarCaraBase(TFT_YELLOW);
    
    sprite->drawLine(x + 10, y + 16, x + 18, y + 24, TFT_BLACK);
    sprite->drawLine(x + 18, y + 24, x + 26, y + 16, TFT_BLACK);
    sprite->drawLine(x + 42, y + 16, x + 50, y + 24, TFT_BLACK);
    sprite->drawLine(x + 50, y + 24, x + 58, y + 16, TFT_BLACK);
    
    sprite->fillCircle(x + 32, y + 42, 8, TFT_RED);
    sprite->fillRect(x + 24, y + 42, 16, 4, TFT_RED);
    
    if (frameAnimacion % 2 == 0) {
        sprite->fillCircle(x + 70, y - 10 - (frameAnimacion % 16), 3, TFT_RED);
        sprite->fillCircle(x + 74, y - 10 - (frameAnimacion % 16), 3, TFT_RED);
        sprite->fillTriangle(x + 67, y - 7 - (frameAnimacion % 16), 
                             x + 77, y - 7 - (frameAnimacion % 16), 
                             x + 72, y - 2 - (frameAnimacion % 16), TFT_RED);
    }
}

void Mascota::dibujarAttack() {
    int x = 24, y = 20;
    dibujarCaraBase(TFT_RED);
    
    sprite->drawLine(x + 8, y + 12, x + 24, y + 20, TFT_BLACK);
    sprite->drawLine(x + 40, y + 20, x + 56, y + 12, TFT_BLACK);
    sprite->fillCircle(x + 16, y + 22, 3, TFT_BLACK);
    sprite->fillCircle(x + 48, y + 22, 3, TFT_BLACK);
    
    sprite->fillCircle(x + 32, y + 46, 8, TFT_BLACK);
    sprite->fillCircle(x + 32, y + 46, 5, TFT_RED);
    
    sprite->drawLine(x + 64, y + 16, x + 90, y - 8, TFT_YELLOW);
    sprite->drawLine(x + 64, y + 32, x + 85, y + 16, TFT_YELLOW);
    sprite->drawLine(x + 64, y + 48, x + 80, y + 40, TFT_YELLOW);
}

void Mascota::dibujarSleep() {
    int x = 24, y = 20;
    
    sprite->fillRoundRect(x, y, 64, 64, 16, TFT_NAVY);
    sprite->drawRoundRect(x, y, 64, 64, 16, TFT_BLUE);
    
    sprite->drawLine(x + 12, y + 22, x + 20, y + 22, TFT_WHITE);
    sprite->drawLine(x + 44, y + 22, x + 52, y + 22, TFT_WHITE);
    sprite->drawPixel(x + 32, y + 42, TFT_WHITE);
    
    int zOffset = (frameAnimacion % 16);
    sprite->setTextColor(TFT_WHITE);
    sprite->setTextSize(2);
    sprite->setCursor(x + 70, y - 20 - zOffset);
    sprite->print("Z");
    if (zOffset > 5) {
        sprite->setCursor(x + 80, y - 30 - zOffset + 5);
        sprite->print("Z");
    }
    if (zOffset > 10) {
        sprite->setCursor(x + 90, y - 40 - zOffset + 10);
        sprite->print("Z");
    }
}

// ---- Interacciones ----

void Mascota::incrementarHandshakes() {
    handshakes++;
    setEstado(ESTADO_HAPPY);
}

void Mascota::setRedesEncontradas(int cantidad) {
    redesEncontradas = cantidad;
}

void Mascota::tocar(int tx, int ty) {
    if (tx >= 80 && tx <= 240 && ty >= 100 && ty <= 280) {
        setEstado(ESTADO_HAPPY);
    }
}
