#include "Mascota.h"
#include "Theme.h"
#include "MascotaSprite.h"

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
    spriteX = (TFT_PANEL_WIDTH / 2) - (MASCOTA_W / 2);
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
    int x = spriteX;
    int y = spriteY;

    switch (estadoActual) {
        case ESTADO_IDLE:
            // el vaiven suave de siempre, de estar tranquila nomas
            y += bobbing();
            break;
        case ESTADO_HAPPY:
            // un pequeno salto en vez del vaiven lento, para que se
            // note que esta contenta
            y += ((frameAnimacion % 8) < 4) ? -4 : 0;
            break;
        case ESTADO_SCANNING:
        case ESTADO_CURIOSA:
        case ESTADO_SLEEP:
        default:
            // quieta; lo que se mueve en estos estados es el
            // indicador de al lado, no el sprite en si
            break;
    }

    // El sprite tiene fondo transparente y se puede mover (bobbing,
    // salto), asi que hay que limpiar una zona mas grande que el
    // sprite mismo para no dejar restos del frame anterior ni de los
    // indicadores (antena, zZz) que se dibujan arriba/al costado.
    display->fillRect(x - 10, spriteY - 40, MASCOTA_W + 100, MASCOTA_H + 50, COLOR_BG);

    display->pushImage(x, y, MASCOTA_W, MASCOTA_H, mascotaRGB565, MASCOTA_TRANSPARENT);

    // El sprite en si es un dibujo fijo (ya no hay una pose distinta
    // por estado como con el arte a mano de antes), asi que un
    // indicador chiquito al lado es lo que le avisa al usuario que
    // esta pasando, ademas de la pildora de estado de arriba.
    int indicadorX = x + MASCOTA_W + 10;
    int indicadorY = y + MASCOTA_H / 2 - 10;

    switch (estadoActual) {
        case ESTADO_HAPPY:
            display->setTextColor(COLOR_ACCENT, COLOR_BG);
            display->setTextFont(2);
            display->setCursor(indicadorX, indicadorY);
            display->print("WII!");
            display->setTextFont(1);
            break;

        case ESTADO_CURIOSA:
            // Antes esta cara era la de "ataque" (enojada, con rayos).
            // Ahora es solo el signo de pregunta, para cuando aparece
            // una red que no estaba en el historial. Nada de rayos ni
            // caras de bronca, total la mascota ya no ataca nada.
            display->setTextColor(COLOR_WARN, COLOR_BG);
            display->setTextFont(2);
            display->setCursor(indicadorX, indicadorY);
            display->print("?");
            display->setTextFont(1);
            break;

        case ESTADO_SCANNING: {
            // antena/radar simple barriendo de un lado al otro
            // mientras busca redes
            int offset = (frameAnimacion % 8);
            if (offset > 4) offset = 8 - offset;
            int puntaX = x + MASCOTA_W / 2 - 12 + offset * 6;
            display->drawLine(x + MASCOTA_W / 2, spriteY - 4, puntaX, spriteY - 20, COLOR_DANGER);
            display->fillCircle(puntaX, spriteY - 22, 3, COLOR_DANGER);
            break;
        }

        case ESTADO_SLEEP: {
            int zOffset = (frameAnimacion % 16);
            display->setTextColor(COLOR_SLEEP_BORDE, COLOR_BG);
            display->setTextFont(2);

            display->setCursor(x + MASCOTA_W - 10, y - 10 - zOffset);
            display->print("z");

            if (zOffset > 5) {
                display->setCursor(x + MASCOTA_W + 2, y - 22 - zOffset + 5);
                display->print("Z");
            }

            if (zOffset > 10) {
                display->setCursor(x + MASCOTA_W + 16, y - 34 - zOffset + 10);
                display->print("Z");
            }

            display->setTextFont(1);
            break;
        }

        default:
            break;
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
    if (tx >= spriteX && tx <= spriteX + MASCOTA_W &&
        ty >= spriteY && ty <= spriteY + MASCOTA_H) {
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
