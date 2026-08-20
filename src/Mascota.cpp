#include "Mascota.h"

Mascota::Mascota() {
    estadoActual = ESTADO_IDLE;
    estadoAnterior = ESTADO_IDLE;
    lastFrame = 0;
    estadoStartTime = 0;
    frameAnimacion = 0;
    handshakes = 0;
    redesEncontradas = 0;
    sprite = nullptr;
}

Mascota::~Mascota() {
    if (sprite) {
        sprite->deleteSprite();
        delete sprite;
    }
}

void Mascota::init(TFT_eSPI *tft) {
    display = tft;
    sprite = new TFT_eSprite(display);
    sprite->createSprite(160, 180);
    sprite->setColorDepth(16);
    display->fillScreen(TFT_BLACK);
}

int Mascota::getNivel() {
    if (handshakes <= 5) return 1;
    else if (handshakes <= 15) return 2;
    else if (handshakes <= 30) return 3;
    else return 4;
}

uint16_t Mascota::getColorNivel() {
    switch (getNivel()) {
        case 1: return TFT_WHITE;
        case 2: return TFT_GREEN;
        case 3: return TFT_SILVER;
        case 4: return TFT_GOLD;
        default: return TFT_GREEN;
    }
}

void Mascota::dibujarAccesorio(int nivel) {
    int x = 24, y = 20; // offset dentro del sprite
    switch (nivel) {
        case 2: { // Gafas de sol
            sprite->fillRect(x+8, y+18, 48, 6, TFT_BLACK);
            sprite->fillRect(x+6, y+16, 10, 10, TFT_BLACK);
            sprite->fillRect(x+48, y+16, 10, 10, TFT_BLACK);
            break;
        }
        case 3: { // Sombrero de hacker (Guy Fawkes)
            sprite->fillRect(x+16, y-10, 32, 10, TFT_BLACK);
            sprite->fillTriangle(x+16, y-10, x+32, y-20, x+48, y-10, TFT_BLACK);
            sprite->fillCircle(x+32, y-18, 4, TFT_RED);
            break;
        }
        case 4: { // Aura dorada
            for (int i = 0; i < 12; i++) {
                int angle = i * 30 + (frameAnimacion % 4) * 4;
                int r = 40 + sin(radians(angle)) * 8;
                int x1 = x + 32 + r * cos(radians(angle));
                int y1 = y + 32 + r * sin(radians(angle));
                sprite->drawPixel(x1, y1, TFT_YELLOW);
            }
            break;
        }
    }
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
    if (!sprite) return;
    
    sprite->fillSprite(TFT_BLACK);
    dibujarSprite();
    dibujarUI();
    sprite->pushSprite(80, 100);
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
    display->print("LV:");
    display->print(getNivel());
    
    display->drawRect(5, 20, 100, 6, TFT_WHITE);
    int humorWidth = (handshakes * 10) % 100;
    if (humorWidth > 100) humorWidth = 100;
    display->fillRect(6, 21, humorWidth, 4, TFT_GREEN);
}

// ---- DIBUJOS DE LA MASCOTA ----

void Mascota::dibujarSprite() {
    int nivel = getNivel();
    uint16_t color = getColorNivel();
    
    switch(estadoActual) {
        case ESTADO_IDLE: dibujarIdle(); break;
        case ESTADO_SCANNING: dibujarScanning(); break;
        case ESTADO_HAPPY: dibujarHappy(); break;
        case ESTADO_ATTACK: dibujarAttack(); break;
        case ESTADO_SLEEP: dibujarSleep(); break;
        default: dibujarIdle(); break;
    }
    
    // Dibujar accesorio según nivel (si no es nivel 1)
    if (nivel >= 2) {
        dibujarAccesorio(nivel);
    }
}

void Mascota::dibujarCaraBase(uint16_t color) {
    int x = 24, y = 20;
    // La cabeza es un poco más grande según el nivel
    int size = 64 + (getNivel() - 1) * 4; // crece 4px por nivel
    sprite->fillRoundRect(x - 2, y - 2, size, size, 16, color);
    sprite->drawRoundRect(x - 2, y - 2, size, size, 16, TFT_WHITE);
    // Brillo
    sprite->drawLine(x + 10, y + 5, x + 25, y + 5, TFT_WHITE);
    sprite->drawPixel(x + 9, y + 6, TFT_WHITE);
}

// --- IDLE ---
void Mascota::dibujarIdle() {
    int x = 24, y = 20;
    dibujarCaraBase(getColorNivel());
    
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

// --- SCANNING ---
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

// --- HAPPY ---
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

// --- ATTACK ---
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

// --- SLEEP ---
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

// ---- INTERACCIONES ----

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
