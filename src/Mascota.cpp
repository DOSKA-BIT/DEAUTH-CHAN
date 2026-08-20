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

// ---- DIBUJO DEL PERSONAJE ANIME ----

void Mascota::dibujarCuerpo() {
    int x = 40, y = 60;
    sprite->fillRoundRect(x+10, y+30, 60, 50, 20, TFT_DARKGREY);
    sprite->drawRoundRect(x+10, y+30, 60, 50, 20, TFT_WHITE);
    
    // Cola
    sprite->fillRoundRect(x+60, y+50, 20, 10, 8, TFT_PURPLE);
    sprite->fillRoundRect(x+70, y+45, 15, 10, 8, TFT_PURPLE);
    sprite->fillRoundRect(x+80, y+40, 10, 10, 8, TFT_PURPLE);
    
    // Brazos
    sprite->fillRoundRect(x, y+35, 10, 20, 5, TFT_DARKGREY);
    sprite->fillRoundRect(x+70, y+35, 10, 20, 5, TFT_DARKGREY);
    
    // Manos
    sprite->fillCircle(x+5, y+55, 5, TFT_WHITE);
    
    if (estadoActual == ESTADO_ATTACK) {
        // AK-47 en la mano derecha
        sprite->fillRoundRect(x+75, y+40, 25, 6, 3, TFT_BLACK);
        sprite->fillRoundRect(x+80, y+35, 8, 6, 3, TFT_BLACK);
        sprite->fillRect(x+80, y+40, 8, 6, TFT_BLACK);
        sprite->fillRoundRect(x+72, y+42, 6, 8, 3, TFT_BROWN);
        sprite->fillRect(x+95, y+40, 15, 4, TFT_BLACK);
        sprite->fillRect(x+83, y+44, 4, 10, TFT_BLACK);
        if (frameAnimacion % 2 == 0) {
            sprite->fillCircle(x+110, y+42, 6, TFT_YELLOW);
            sprite->fillCircle(x+112, y+40, 3, TFT_ORANGE);
        }
    } else {
        sprite->fillCircle(x+75, y+55, 5, TFT_WHITE);
    }
}

void Mascota::dibujarCabello() {
    int x = 24, y = 10;
    sprite->fillRoundRect(x+12, y-5, 56, 20, 10, TFT_PURPLE);
    sprite->fillTriangle(x+20, y-10, x+40, y-20, x+60, y-10, TFT_PURPLE);
    sprite->fillTriangle(x+30, y-15, x+50, y-25, x+70, y-15, TFT_PURPLE);
    sprite->fillRoundRect(x-5, y+5, 10, 25, 5, TFT_PURPLE);
    sprite->fillRoundRect(x+75, y+5, 10, 25, 5, TFT_PURPLE);
    
    // Orejas de gato
    sprite->fillTriangle(x+8, y-5, x+18, y-25, x+28, y-5, TFT_PURPLE);
    sprite->drawTriangle(x+8, y-5, x+18, y-25, x+28, y-5, TFT_WHITE);
    sprite->fillTriangle(x+52, y-5, x+62, y-25, x+72, y-5, TFT_PURPLE);
    sprite->drawTriangle(x+52, y-5, x+62, y-25, x+72, y-5, TFT_WHITE);
    sprite->fillTriangle(x+12, y-8, x+18, y-20, x+24, y-8, TFT_PINK);
    sprite->fillTriangle(x+56, y-8, x+62, y-20, x+68, y-8, TFT_PINK);
    
    if (estadoActual == ESTADO_SLEEP) {
        sprite->fillTriangle(x+8, y-5, x+14, y-10, x+18, y-5, TFT_PURPLE);
        sprite->fillTriangle(x+52, y-5, x+58, y-10, x+62, y-5, TFT_PURPLE);
    }
}

void Mascota::dibujarOjos(int estado) {
    int x = 24, y = 20;
    if (estado == ESTADO_SLEEP) {
        sprite->drawLine(x+10, y+22, x+20, y+18, TFT_BLACK);
        sprite->drawLine(x+20, y+18, x+30, y+22, TFT_BLACK);
        sprite->drawLine(x+42, y+22, x+52, y+18, TFT_BLACK);
        sprite->drawLine(x+52, y+18, x+62, y+22, TFT_BLACK);
        return;
    }
    
    sprite->fillRoundRect(x+8, y+14, 24, 14, 6, TFT_WHITE);
    sprite->fillRoundRect(x+40, y+14, 24, 14, 6, TFT_WHITE);
    
    if (estado == ESTADO_IDLE || estado == ESTADO_SCANNING) {
        sprite->fillRect(x+16, y+18, 4, 8, TFT_BLACK);
        sprite->fillRect(x+48, y+18, 4, 8, TFT_BLACK);
        sprite->fillRect(x+14, y+16, 3, 3, TFT_WHITE);
        sprite->fillRect(x+46, y+16, 3, 3, TFT_WHITE);
    } else {
        sprite->fillCircle(x+18, y+22, 5, TFT_BLACK);
        sprite->fillCircle(x+50, y+22, 5, TFT_BLACK);
        sprite->fillCircle(x+16, y+18, 3, TFT_WHITE);
        sprite->fillCircle(x+48, y+18, 3, TFT_WHITE);
    }
    
    if (estado == ESTADO_HAPPY) {
        for (int i = 0; i < 4; i++) {
            int angle = i * 45 + 22;
            int x1 = 18 + 6 * cos(radians(angle));
            int y1 = 22 + 6 * sin(radians(angle));
            sprite->drawPixel(x1, y1, TFT_WHITE);
            int x2 = 50 + 6 * cos(radians(angle));
            int y2 = 22 + 6 * sin(radians(angle));
            sprite->drawPixel(x2, y2, TFT_WHITE);
        }
    } else if (estado == ESTADO_ATTACK) {
        sprite->fillRoundRect(x+8, y+14, 24, 14, 6, TFT_RED);
        sprite->fillRoundRect(x+40, y+14, 24, 14, 6, TFT_RED);
        sprite->fillCircle(x+18, y+22, 4, TFT_BLACK);
        sprite->fillCircle(x+50, y+22, 4, TFT_BLACK);
        sprite->drawLine(x+6, y+12, x+24, y+8, TFT_BLACK);
        sprite->drawLine(x+42, y+8, x+60, y+12, TFT_BLACK);
    }
}

void Mascota::dibujarBoca(int estado) {
    int x = 24, y = 20;
    if (estado == ESTADO_HAPPY) {
        sprite->fillRoundRect(x+18, y+40, 30, 10, 4, TFT_RED);
        sprite->drawLine(x+20, y+44, x+45, y+44, TFT_WHITE);
    } else if (estado == ESTADO_ATTACK) {
        sprite->fillCircle(x+32, y+46, 10, TFT_BLACK);
        sprite->fillCircle(x+32, y+46, 7, TFT_RED);
    } else if (estado == ESTADO_SLEEP) {
        sprite->drawLine(x+28, y+44, x+36, y+44, TFT_BLACK);
    } else {
        sprite->drawLine(x+22, y+42, x+32, y+46, TFT_BLACK);
        sprite->drawLine(x+32, y+46, x+42, y+42, TFT_BLACK);
    }
}

void Mascota::dibujarSudadera() {
    int x = 40, y = 60;
    sprite->fillRoundRect(x+5, y-5, 70, 20, 8, TFT_DARKGREY);
    sprite->drawRoundRect(x+5, y-5, 70, 20, 8, TFT_WHITE);
    sprite->fillTriangle(x+10, y-5, x+18, y-12, x+26, y-5, TFT_DARKGREY);
    sprite->fillTriangle(x+54, y-5, x+62, y-12, x+70, y-5, TFT_DARKGREY);
    sprite->drawLine(x+30, y+5, x+30, y+15, TFT_WHITE);
    sprite->drawLine(x+50, y+5, x+50, y+15, TFT_WHITE);
}

void Mascota::dibujarAccesorio(int nivel) {
    int x = 24, y = 20;
    switch (nivel) {
        case 2: {
            sprite->fillRect(x+8, y+18, 48, 8, TFT_BLACK);
            sprite->fillRect(x+6, y+16, 10, 12, TFT_BLACK);
            sprite->fillRect(x+48, y+16, 10, 12, TFT_BLACK);
            break;
        }
        case 3: {
            sprite->drawLine(x+32, y-20, x+32, y-8, TFT_SILVER);
            sprite->fillCircle(x+32, y-22, 4, TFT_RED);
            sprite->fillRect(x+16, y-12, 32, 6, TFT_SILVER);
            break;
        }
        case 4: {
            for (int i = 0; i < 16; i++) {
                int angle = i * 22 + (frameAnimacion % 4) * 4;
                int r = 45 + sin(radians(angle)) * 8;
                int x1 = x + 32 + r * cos(radians(angle));
                int y1 = y + 32 + r * sin(radians(angle));
                sprite->drawPixel(x1, y1, TFT_YELLOW);
            }
            break;
        }
    }
}

// ---- MÉTODOS PRINCIPALES ----

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

void Mascota::dibujarSprite() {
    int nivel = getNivel();
    
    dibujarCuerpo();
    dibujarSudadera();
    dibujarCabello();
    
    uint16_t color = getColorNivel();
    sprite->fillRoundRect(20, 16, 64, 64, 16, color);
    sprite->drawRoundRect(20, 16, 64, 64, 16, TFT_WHITE);
    sprite->drawLine(30, 22, 45, 22, TFT_WHITE);
    sprite->drawPixel(29, 23, TFT_WHITE);
    
    dibujarOjos(estadoActual);
    dibujarBoca(estadoActual);
    
    if (nivel >= 2) {
        dibujarAccesorio(nivel);
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
    if (tx >= 80 && tx <= 240 && ty >= 100 && ty <= 280) {
        setEstado(ESTADO_HAPPY);
    }
}
