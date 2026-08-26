#include "Mascota.h"

Mascota::Mascota() {
    estadoActual = ESTADO_IDLE;
    estadoAnterior = ESTADO_IDLE;
    lastFrame = 0;
    estadoStartTime = 0;
    frameAnimacion = 0;
    redesNuevas = 0;
    redesEncontradas = 0;
    spriteX = 0;
    spriteY = 120;
}

void Mascota::init(TFT_eSPI* tft) {
    display = tft;
    display->fillScreen(TFT_BLACK);

    // Centrado horizontal segun el ancho real de la placa que este
    // corriendo, para no dejar la cara pegada a un costado en las
    // pantallas de 320 o 480px de ancho.
    spriteX = (TFT_PANEL_WIDTH / 2) - 32;
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

    if (estadoActual == ESTADO_CURIOSA && ahora - estadoStartTime > 2000) {
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
    display->fillRect(0, 100, TFT_PANEL_WIDTH, 180, TFT_BLACK);
    dibujarSprite();
    dibujarUI();
}

void Mascota::dibujarUI() {
    display->fillRect(0, 0, TFT_PANEL_WIDTH, 30, TFT_DARKGREY);
    display->setTextColor(TFT_WHITE);
    display->setTextSize(1);

    display->setCursor(5, 5);
    display->print("REDES:");
    display->print(redesEncontradas);

    display->setCursor(90, 5);
    display->print("NUEVAS:");
    display->print(redesNuevas);

    display->setCursor(TFT_PANEL_WIDTH - 45, 5);
    switch (estadoActual) {
        case ESTADO_IDLE:     display->print("IDLE"); break;
        case ESTADO_SCANNING: display->print("SCAN"); break;
        case ESTADO_HAPPY:    display->print("NICE!"); break;
        case ESTADO_CURIOSA:  display->print("?"); break;
        case ESTADO_SLEEP:    display->print("Zzz"); break;
        default: break;
    }

    display->drawRect(5, 20, 100, 6, TFT_WHITE);
    int humorWidth = (redesEncontradas * 5) % 100;
    if (humorWidth > 100) humorWidth = 100;
    display->fillRect(6, 21, humorWidth, 4, TFT_GREEN);
}

void Mascota::dibujarSprite() {
    switch (estadoActual) {
        case ESTADO_IDLE:     dibujarIdle(); break;
        case ESTADO_SCANNING: dibujarScanning(); break;
        case ESTADO_HAPPY:    dibujarHappy(); break;
        case ESTADO_CURIOSA:  dibujarCuriosa(); break;
        case ESTADO_SLEEP:    dibujarSleep(); break;
        default: dibujarIdle(); break;
    }
}

void Mascota::dibujarCaraBase(uint16_t color) {
    int x = spriteX;
    int y = spriteY;
    display->fillRoundRect(x, y, 64, 64, 16, color);
    display->drawRoundRect(x, y, 64, 64, 16, TFT_WHITE);
    display->drawLine(x + 10, y + 5, x + 25, y + 5, TFT_WHITE);
    display->drawPixel(x + 9, y + 6, TFT_WHITE);
}

void Mascota::dibujarIdle() {
    int x = spriteX;
    int y = spriteY;
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
    int x = spriteX;
    int y = spriteY;
    dibujarCaraBase(TFT_CYAN);

    display->fillCircle(x + 16, y + 20, 5, TFT_WHITE);
    display->fillCircle(x + 48, y + 20, 5, TFT_WHITE);
    display->fillCircle(x + 16, y + 20, 2, TFT_BLACK);
    display->fillCircle(x + 48, y + 20, 2, TFT_BLACK);

    // antena girando mientras busca
    int offset = (frameAnimacion % 4) * 4;
    display->drawLine(x + 32, y - 10, x + 32 + offset, y - 25, TFT_RED);
}

void Mascota::dibujarHappy() {
    int x = spriteX;
    int y = spriteY;
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

void Mascota::dibujarCuriosa() {
    // Antes esta cara era la de "ataque" (enojada, con rayos). La dejo
    // como cara curiosa: ceja levantada + antena parada, para cuando
    // aparece una red que no estaba en el historial. Nada de rayos ni
    // caras de bronca, total la mascota ya no ataca nada.
    int x = spriteX;
    int y = spriteY;
    dibujarCaraBase(TFT_CYAN);

    display->drawLine(x + 10, y + 16, x + 24, y + 12, TFT_BLACK);
    display->fillCircle(x + 16, y + 20, 4, TFT_WHITE);
    display->fillCircle(x + 48, y + 20, 4, TFT_WHITE);
    display->fillCircle(x + 16, y + 20, 2, TFT_BLACK);
    display->fillCircle(x + 48, y + 20, 2, TFT_BLACK);

    display->drawCircle(x + 32, y + 46, 4, TFT_BLACK);

    display->drawLine(x + 32, y, x + 32, y - 14, TFT_YELLOW);
    display->fillCircle(x + 32, y - 16, 3, TFT_YELLOW);

    display->setTextColor(TFT_CYAN);
    display->setTextSize(2);
    display->setCursor(x + 70, y - 5);
    display->print("?");
}

void Mascota::dibujarSleep() {
    int x = spriteX;
    int y = spriteY;

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

void Mascota::incrementarRedesNuevas() {
    redesNuevas++;
    setEstado(ESTADO_CURIOSA);
}

void Mascota::setRedesEncontradas(int cantidad) {
    redesEncontradas = cantidad;
}

void Mascota::tocar(int tx, int ty) {
    if (tx >= spriteX && tx <= spriteX + 64 && ty >= spriteY && ty <= spriteY + 64) {
        setEstado(ESTADO_HAPPY);
    }
}
