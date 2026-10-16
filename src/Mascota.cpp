#include "Mascota.h"
#include "Theme.h"

Mascota::Mascota() {
    estadoActual = ESTADO_IDLE;
    estadoAnterior = ESTADO_IDLE;
    lastFrame = 0;
    estadoStartTime = 0;
    frameAnimacion = 0;
    redesNuevas = 0;
    redesEncontradas = 0;
    spriteX = 0;
    spriteY = CONTENT_TOP + 60;
    estadoFirmaAnterior = ESTADO_COUNT; // valor invalido a proposito, para forzar el primer redibujado
    firmaAnterior = 255;
}

void Mascota::init(TFT_eSPI* tft) {
    display = tft;
    display->fillScreen(COLOR_BG);

    // Centrado horizontal segun el ancho real de la placa que este
    // corriendo, para no dejar la cara pegada a un costado en las
    // pantallas de 320 o 480px de ancho.
    spriteX = (TFT_PANEL_WIDTH / 2) - (SPRITE_SIZE / 2);
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

int Mascota::bobbing() const {
    // Un vaiven vertical bien chico, nada mas para que no quede
    // pegada como estampita. Triangular en vez de seno porque no
    // vale la pena meter una tabla de senos para 3px de diferencia.
    uint8_t t = frameAnimacion % 16;
    int triangular = (t < 8) ? t : (16 - t);
    return (triangular / 2) - 2; // onda suave, entre -2 y +1 px
}

void Mascota::dibujar() {
    int alto = TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - CONTENT_TOP;
    display->fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alto, COLOR_BG);
    dibujarEncabezado();
    dibujarSprite();
}

const char* Mascota::nombreEstado() const {
    switch (estadoActual) {
        case ESTADO_IDLE:     return "DE PASEO";
        case ESTADO_SCANNING: return "BUSCANDO";
        case ESTADO_HAPPY:    return "CONTENTA";
        case ESTADO_CURIOSA:  return "QUE ES ESO?";
        case ESTADO_SLEEP:    return "DURMIENDO";
        default:              return "";
    }
}

uint16_t Mascota::colorEstado() const {
    switch (estadoActual) {
        case ESTADO_IDLE:     return COLOR_ACCENT;
        case ESTADO_SCANNING: return COLOR_WARN;
        case ESTADO_HAPPY:    return COLOR_ACCENT;
        case ESTADO_CURIOSA:  return COLOR_WARN;
        case ESTADO_SLEEP:    return COLOR_TEXT_DIM;
        default:              return COLOR_TEXT;
    }
}

void Mascota::dibujarEncabezado() {
    display->fillRect(0, 0, TFT_PANEL_WIDTH, 32, COLOR_PANEL);
    display->drawFastHLine(0, 32, TFT_PANEL_WIDTH, COLOR_ACCENT_DIM);

    display->setTextFont(1);
    display->setTextSize(1);

    display->setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
    display->setCursor(6, 5);
    display->print("REDES");
    display->setTextColor(COLOR_TEXT, COLOR_PANEL);
    display->setCursor(6, 16);
    display->print(redesEncontradas);

    display->setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
    display->setCursor(60, 5);
    display->print("NUEVAS");
    display->setTextColor(COLOR_WARN, COLOR_PANEL);
    display->setCursor(60, 16);
    display->print(redesNuevas);

    // Etiqueta de estado a la derecha, como una pildora de color para
    // que se note de un vistazo sin tener que leer el texto.
    uint16_t color = colorEstado();
    const char* nombre = nombreEstado();
    int anchoPildora = 6 + strlen(nombre) * 6;
    int xPildora = TFT_PANEL_WIDTH - anchoPildora - 8;
    display->fillRoundRect(xPildora, 8, anchoPildora, 16, 4, color);
    display->setTextColor(COLOR_BG, color);
    display->setCursor(xPildora + 3, 12);
    display->print(nombre);

    // Barra de "actividad": cuanto mas redes nuevas encontro en lo que
    // va de la sesion, mas llena. Antes esto usaba un modulo que la
    // hacia vaciarse de golpe cada 20 redes sin ningun motivo, asi que
    // de paso lo dejo bien.
    int anchoBarra = TFT_PANEL_WIDTH - 12;
    display->drawRect(6, 26, anchoBarra, 4, COLOR_TEXT_DIM);
    int lleno = redesNuevas * 4;
    if (lleno > anchoBarra - 2) lleno = anchoBarra - 2;
    if (lleno > 0) {
        display->fillRect(7, 27, lleno, 2, COLOR_ACCENT);
    }
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

void Mascota::dibujarCaraBase(uint16_t color, int y) {
    int x = spriteX;
    int s = SPRITE_SIZE;

    // Un par de orejitas/antenitas antes del cuerpo, asi queda mas
    // bicho y menos cuadrado pelado.
    display->fillTriangle(x + 14, y + 6, x + 6, y - 10, x + 26, y, color);
    display->fillTriangle(x + s - 14, y + 6, x + s - 6, y - 10, x + s - 26, y, color);

    display->fillRoundRect(x, y, s, s, 20, color);
    display->drawRoundRect(x, y, s, s, 20, COLOR_TEXT);

    // Brillito arriba a la izquierda, da la sensacion de que el
    // cuerpo es medio brillante/plastico en vez de plano.
    display->drawLine(x + 14, y + 10, x + 32, y + 10, COLOR_TEXT);
    display->drawPixel(x + 12, y + 11, COLOR_TEXT);
}

void Mascota::dibujarOjos(int y, bool cerrados) {
    int x = spriteX;
    int s = SPRITE_SIZE;
    int ojoIzqX = x + s / 3;
    int ojoDerX = x + s - s / 3;
    int ojoY = y + s / 2 - 6;

    if (cerrados) {
        display->drawLine(ojoIzqX - 6, ojoY, ojoIzqX + 6, ojoY, COLOR_BG);
        display->drawLine(ojoDerX - 6, ojoY, ojoDerX + 6, ojoY, COLOR_BG);
        return;
    }

    display->fillCircle(ojoIzqX, ojoY, 7, COLOR_TEXT);
    display->fillCircle(ojoDerX, ojoY, 7, COLOR_TEXT);
    display->fillCircle(ojoIzqX, ojoY, 3, COLOR_BG);
    display->fillCircle(ojoDerX, ojoY, 3, COLOR_BG);
    // puntito de brillo en cada ojo, para que no quede con cara de
    // muneca de porcelana
    display->drawPixel(ojoIzqX - 1, ojoY - 1, COLOR_TEXT);
    display->drawPixel(ojoDerX - 1, ojoY - 1, COLOR_TEXT);
}

void Mascota::dibujarIdle() {
    int y = spriteY + bobbing();
    dibujarCaraBase(COLOR_ACCENT, y);

    bool parpadeo = (frameAnimacion % 16) < 2;
    dibujarOjos(y, parpadeo);

    int x = spriteX;
    int s = SPRITE_SIZE;
    // sonrisa chiquita, de estar tranquila nomas
    display->drawLine(x + s / 2 - 10, y + s - 24, x + s / 2, y + s - 18, COLOR_BG);
    display->drawLine(x + s / 2, y + s - 18, x + s / 2 + 10, y + s - 24, COLOR_BG);
}

void Mascota::dibujarScanning() {
    int y = spriteY; // sin bobbing aca, esta concentrada buscando
    dibujarCaraBase(COLOR_WARN, y);
    dibujarOjos(y, false);

    int x = spriteX;
    int s = SPRITE_SIZE;
    // boquita en "o", cara de concentrada
    display->drawCircle(x + s / 2, y + s - 20, 4, COLOR_BG);

    // antena en movimiento, como un radar barriendo de un lado al
    // otro mientras busca redes
    int offset = (frameAnimacion % 8);
    if (offset > 4) offset = 8 - offset;
    int puntaX = x + s / 2 - 12 + offset * 6;
    display->drawLine(x + s / 2, y - 6, puntaX, y - 24, COLOR_DANGER);
    display->fillCircle(puntaX, y - 26, 3, COLOR_DANGER);
}

void Mascota::dibujarHappy() {
    // un pequeno salto en vez del vaiven lento de siempre, para que
    // se note que esta contenta
    int salto = ((frameAnimacion % 8) < 4) ? -4 : 0;
    int y = spriteY + salto;
    dibujarCaraBase(COLOR_ACCENT, y);
    dibujarOjos(y, false);

    int x = spriteX;
    int s = SPRITE_SIZE;
    display->drawLine(x + s / 2 - 14, y + s - 26, x + s / 2, y + s - 14, COLOR_BG);
    display->drawLine(x + s / 2, y + s - 14, x + s / 2 + 14, y + s - 26, COLOR_BG);
    display->drawLine(x + s / 2 - 14, y + s - 25, x + s / 2, y + s - 13, COLOR_BG);
    display->drawLine(x + s / 2, y + s - 13, x + s / 2 + 14, y + s - 25, COLOR_BG);

    display->setTextColor(COLOR_ACCENT);
    display->setTextFont(2);
    display->setCursor(x + s + 10, y + 16);
    display->print("WII!");
    display->setTextFont(1);
}

void Mascota::dibujarCuriosa() {
    // Antes esta cara era la de "ataque" (enojada, con rayos). La dejo
    // como cara curiosa: ceja levantada + antena parada, para cuando
    // aparece una red que no estaba en el historial. Nada de rayos ni
    // caras de bronca, total la mascota ya no ataca nada.
    int y = spriteY;
    dibujarCaraBase(COLOR_WARN, y);
    dibujarOjos(y, false);

    int x = spriteX;
    int s = SPRITE_SIZE;

    // ceja levantada arriba del ojo izquierdo
    display->drawLine(x + s / 3 - 8, y + s / 2 - 16, x + s / 3 + 6, y + s / 2 - 20, COLOR_BG);

    // boca entreabierta, cara de "y esto que es?"
    display->drawCircle(x + s / 2, y + s - 20, 4, COLOR_BG);

    // antena bien parada con un signo de pregunta al lado
    display->drawLine(x + s / 2, y - 6, x + s / 2, y - 22, COLOR_WARN);
    display->fillCircle(x + s / 2, y - 24, 3, COLOR_WARN);

    display->setTextColor(COLOR_WARN);
    display->setTextFont(2);
    display->setCursor(x + s + 14, y + 14);
    display->print("?");
    display->setTextFont(1);
}

void Mascota::dibujarSleep() {
    int y = spriteY; // quieta, nada de bobbing mientras duerme
    int x = spriteX;
    int s = SPRITE_SIZE;

    dibujarCaraBase(COLOR_SLEEP, y);
    display->drawRoundRect(x, y, s, s, 20, COLOR_SLEEP_BORDE);
    dibujarOjos(y, true);

    // boquita chiquita, respirando tranquila
    display->drawLine(x + s / 2 - 4, y + s - 20, x + s / 2 + 4, y + s - 20, COLOR_SLEEP_BORDE);

    int zOffset = (frameAnimacion % 16);
    display->setTextColor(COLOR_SLEEP_BORDE);
    display->setTextFont(2);

    display->setCursor(x + s - 10, y - 10 - zOffset);
    display->print("z");

    if (zOffset > 5) {
        display->setCursor(x + s + 2, y - 22 - zOffset + 5);
        display->print("Z");
    }

    if (zOffset > 10) {
        display->setCursor(x + s + 16, y - 34 - zOffset + 10);
        display->print("Z");
    }

    display->setTextFont(1);
}

void Mascota::incrementarRedesNuevas() {
    redesNuevas++;
    setEstado(ESTADO_CURIOSA);
}

void Mascota::setRedesEncontradas(int cantidad) {
    redesEncontradas = cantidad;
}

void Mascota::tocar(int tx, int ty) {
    if (tx >= spriteX && tx <= spriteX + SPRITE_SIZE &&
        ty >= spriteY && ty <= spriteY + SPRITE_SIZE) {
        setEstado(ESTADO_HAPPY);
    }
}

uint8_t Mascota::firmaVisual() const {
    switch (estadoActual) {
        case ESTADO_IDLE:
            // Antes esto solo distinguia "parpadeando" de "no", pero
            // ahora la cara tambien se balancea sola, asi que hace
            // falta un numero que cambie en cada tick para que el
            // vaiven se vea fluido y no a los saltos.
            return frameAnimacion % 16;
        case ESTADO_SCANNING:
            return frameAnimacion % 8;
        case ESTADO_HAPPY:
            // el salto de "contenta" tambien necesita redibujar en
            // cada tick, si no se queda pegada en el aire o en el piso
            return frameAnimacion % 8;
        case ESTADO_SLEEP:
            return frameAnimacion % 16;
        default:
            // CURIOSA es una pose fija, solo cambia al entrar o salir
            // del estado.
            return 0;
    }
}

bool Mascota::necesitaRedibujar() {
    uint8_t firma = firmaVisual();
    if (estadoActual != estadoFirmaAnterior || firma != firmaAnterior) {
        estadoFirmaAnterior = estadoActual;
        firmaAnterior = firma;
        return true;
    }
    return false;
}
