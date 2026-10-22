#include "Mascota.h"
#include "Theme.h"
#include "MascotaSprite.h"

// Elige cual de los 2 frames de arte real le toca a cada estado. Vive
// afuera de la clase (no hace falta declararla en el header) igual que
// escribirTextoEscapado() en Wardriving.cpp.
static const uint16_t* spriteParaEstado(EstadoMascota estado, bool frame2) {
    switch (estado) {
        case ESTADO_IDLE:     return frame2 ? mascotaIdle2     : mascotaIdle1;
        case ESTADO_HAPPY:    return frame2 ? mascotaHappy2    : mascotaHappy1;
        case ESTADO_SCANNING: return frame2 ? mascotaScanning2 : mascotaScanning1;
        case ESTADO_CURIOSA:  return frame2 ? mascotaCuriosa2  : mascotaCuriosa1;
        case ESTADO_SLEEP:    return frame2 ? mascotaSleep2    : mascotaSleep1;
        default:              return mascotaIdle1;
    }
}

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
    estadoEncabezadoAnterior = ESTADO_COUNT; // idem, fuerza el primer dibujo del encabezado
    redesNuevasEncabezadoAnterior = -1;
    redesEncontradasEncabezadoAnterior = -1;
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

void Mascota::dibujar(bool limpiarTodo) {
    if (limpiarTodo) {
        // Solo hace falta al entrar a esta pantalla (para tapar lo que
        // haya quedado de la pantalla anterior). En los redibujados de
        // animacion, dibujarEncabezado() y dibujarSprite() ya limpian
        // cada uno su propia zona - repetir un borrado de toda la
        // franja ademas de eso solo suma un parpadeo mas grande sin
        // necesidad.
        int alto = TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - CONTENT_TOP;
        display->fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alto, COLOR_BG);
    }
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
    // Sin panel relleno de fondo (antes era un bloque COLOR_PANEL) -
    // en la estetica de terminal todo va sobre el mismo negro, con una
    // linea fina como unica separacion.
    display->fillRect(0, 0, TFT_PANEL_WIDTH, 32, COLOR_BG);
    display->drawFastHLine(0, 32, TFT_PANEL_WIDTH, COLOR_ACCENT_DIM);

    display->setTextFont(1);
    display->setTextSize(1);

    display->setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    display->setCursor(6, 5);
    display->print("REDES");
    display->setTextColor(COLOR_TEXT, COLOR_BG);
    display->setCursor(6, 16);
    display->print(redesEncontradas);

    display->setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    display->setCursor(60, 5);
    display->print("NUEVAS");
    display->setTextColor(COLOR_WARN, COLOR_BG);
    display->setCursor(60, 16);
    display->print(redesNuevas);

    // Etiqueta de estado a la derecha: antes era una pildora redondeada
    // rellena, ahora un bloque cuadrado invertido (fondo celeste, texto
    // negro) para que combine con el resto de la estetica de terminal.
    uint16_t color = colorEstado();
    const char* nombre = nombreEstado();
    int anchoPildora = 6 + strlen(nombre) * 6;
    int xPildora = TFT_PANEL_WIDTH - anchoPildora - 8;
    display->fillRect(xPildora, 8, anchoPildora, 16, color);
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
    unsigned long transcurrido = millis() - estadoStartTime;

    // Cada estado tiene 2 frames de arte real (ver assets/mascota_frames/
    // y tools/convert_mascota_frames.py). Para HAPPY y CURIOSA, que duran
    // poco y vuelven solos a IDLE (ver update()), el frame 2 entra a
    // mitad de camino para que se alcancen a ver las dos poses antes de
    // que el estado termine. IDLE, SCANNING y SLEEP no tienen limite de
    // tiempo, asi que ahi alternamos solas cada ~4s via frameAnimacion.
    bool frame2;
    switch (estadoActual) {
        case ESTADO_HAPPY:
            // sonrisa grande primero, ojitos cerrados de contenta despues
            frame2 = transcurrido > 1500;
            // ademas un pequeno salto en vez del vaiven lento, para que
            // se note mas que esta contenta
            y += ((frameAnimacion % 8) < 4) ? -4 : 0;
            break;
        case ESTADO_CURIOSA:
            // primero pensando (dedo en la mejilla), despues la sorpresa
            // de la red nueva
            frame2 = transcurrido > 1000;
            break;
        case ESTADO_IDLE:
            // el vaiven suave de siempre, de estar tranquila nomas
            y += bobbing();
            frame2 = (frameAnimacion % 40) >= 20;
            break;
        default: // SCANNING, SLEEP
            frame2 = (frameAnimacion % 40) >= 20;
            break;
    }

    // El sprite tiene fondo transparente y se puede mover (bobbing,
    // salto), asi que hay que limpiar una zona mas grande que el
    // sprite mismo para no dejar restos del frame anterior.
    display->fillRect(x - 10, spriteY - 40, MASCOTA_W + 100, MASCOTA_H + 50, COLOR_BG);

    // El array RGB565 esta generado como un entero de 16 bits comun
    // (el que arma el script de conversion), pero TFT_eSPI por defecto
    // asume que pushImage() recibe los bytes ya invertidos para la
    // transferencia SPI (_swapBytes = false de entrada). Sin este
    // setSwapBytes(true) el chip manda los 2 bytes de cada pixel al
    // reves y los colores salen todos mezclados/con ruido en pantalla,
    // que es justo lo que se vio en la placa. Lo prendo solo para este
    // pushImage puntual y lo dejo como estaba despues, para no afectar
    // el resto de los dibujos de esta pantalla (texto, barras, etc).
    display->setSwapBytes(true);
    const uint16_t* sprite = spriteParaEstado(estadoActual, frame2);
    display->pushImage(x, y, MASCOTA_W, MASCOTA_H, sprite, MASCOTA_TRANSPARENT);
    display->setSwapBytes(false);

    // HAPPY y CURIOSA llevan ademas un indicador de texto al costado -
    // SCANNING y SLEEP ya no lo necesitan porque sus frames 2 (el HUD de
    // escaneo y las "zZz") ya traen ese detalle dibujado en el arte.
    if (estadoActual == ESTADO_HAPPY) {
        display->setTextColor(COLOR_ACCENT, COLOR_BG);
        display->setTextFont(2);
        display->setCursor(x + MASCOTA_W + 10, y + MASCOTA_H / 2 - 10);
        display->print("WII!");
        display->setTextFont(1);
    } else if (estadoActual == ESTADO_CURIOSA) {
        // Antes esta cara era la de "ataque" (enojada, con rayos). Ahora
        // es solo curiosidad, para cuando aparece una red que no estaba
        // en el historial. Nada de rayos ni caras de bronca, total la
        // mascota ya no ataca nada.
        display->setTextColor(COLOR_WARN, COLOR_BG);
        display->setTextFont(2);
        display->setCursor(x + MASCOTA_W + 10, y + MASCOTA_H / 2 - 10);
        display->print("?");
        display->setTextFont(1);
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
            // El vaiven (bobbing) cambia en cada tick y el frame cada
            // ~4s - frameAnimacion ya cambia en cada tick, asi que
            // devolverlo tal cual alcanza para detectar cualquiera de
            // los dos cambios.
            return frameAnimacion;
        case ESTADO_HAPPY: {
            // el salto necesita redibujar en cada tick (si no se queda
            // pegada en el aire o en el piso), y ademas hay que
            // detectar el cambio de frame a mitad de los 3s que dura
            bool frame2 = (millis() - estadoStartTime) > 1500;
            uint8_t salto = frameAnimacion % 8;
            return (uint8_t)((frame2 ? 128 : 0) | salto);
        }
        case ESTADO_CURIOSA:
            // pose fija dentro de cada mitad - solo cambia al pasar el
            // segundo del medio, o al entrar/salir del estado
            return (millis() - estadoStartTime) > 1000 ? 1 : 0;
        case ESTADO_SCANNING:
        case ESTADO_SLEEP:
            // sin limite de tiempo fijo; solo cambia cuando toca
            // alternar de frame cada ~4s
            return (frameAnimacion % 40) / 20;
        default:
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

bool Mascota::necesitaRedibujarEncabezado() {
    // El encabezado no tiene nada que se anime solo (ni bobbing ni
    // cambio de frame) - lo unico que puede cambiar es el texto/color
    // que muestra, asi que comparamos esos valores en vez de la firma
    // visual de la cara grande. Esto es lo que evita que la franja de
    // arriba parpadee en TODAS las pantallas cada vez que la mascota
    // tiene un tick de animacion para su propia cara.
    if (estadoActual != estadoEncabezadoAnterior ||
        redesNuevas != redesNuevasEncabezadoAnterior ||
        redesEncontradas != redesEncontradasEncabezadoAnterior) {
        estadoEncabezadoAnterior = estadoActual;
        redesNuevasEncabezadoAnterior = redesNuevas;
        redesEncontradasEncabezadoAnterior = redesEncontradas;
        return true;
    }
    return false;
}
