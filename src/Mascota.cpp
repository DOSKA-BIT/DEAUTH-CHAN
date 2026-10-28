#include "Mascota.h"
#include "Theme.h"
#include "MascotaCaras.h"

// Elige cual de los 2 frames le toca a cada estado. Vive afuera de la
// clase (no hace falta declararla en el header) igual que
// escribirTextoEscapado() en Wardriving.cpp.
//
// Antes esto devolvia un puntero a un sprite RGB565 a color
// (MascotaSprite.h); ahora devuelve un bitmap monocromo chico de
// MascotaCaras.h (pixel art estilo kaomoji, ver el comentario largo
// en tools/generar_caras.py de por que no es texto unicode real).
static const uint8_t* caraParaEstado(EstadoMascota estado, bool frame2) {
    switch (estado) {
        case ESTADO_IDLE:     return frame2 ? CARA_IDLE_2     : CARA_IDLE_1;
        case ESTADO_HAPPY:    return frame2 ? CARA_HAPPY_2    : CARA_HAPPY_1;
        case ESTADO_SCANNING: return frame2 ? CARA_SCANNING_2 : CARA_SCANNING_1;
        case ESTADO_CURIOSA:  return frame2 ? CARA_CURIOSA_2  : CARA_CURIOSA_1;
        case ESTADO_SLEEP:    return frame2 ? CARA_SLEEP_2    : CARA_SLEEP_1;
        default:              return CARA_IDLE_1;
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
    spriteX = (TFT_PANEL_WIDTH / 2) - (CARA_ANCHO / 2);
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
        // animacion, dibujarEncabezado() y dibujarCara() ya limpian
        // cada uno su propia zona - repetir un borrado de toda la
        // franja ademas de eso solo suma un parpadeo mas grande sin
        // necesidad.
        int alto = TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - CONTENT_TOP;
        display->fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alto, COLOR_BG);
    }
    dibujarEncabezado();
    dibujarCara();
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
    // Fase 2 del rediseño "herramienta retro": la franja de arriba
    // ahora es una barra azul intensa bien visible (COLOR_HEADER), en
    // vez del mismo negro/navy del fondo - calcado de la referencia
    // que mando el usuario (encabezado "azul intenso" diferenciado
    // del resto de la pantalla). Como este encabezado es compartido
    // por TODAS las pantallas (lo llama cada una via
    // dibujarEncabezado()), este solo cambio ya se nota en el
    // firmware entero, no solo en la mascota.
    display->fillRect(0, 0, TFT_PANEL_WIDTH, 32, COLOR_HEADER);
    display->drawFastHLine(0, 32, TFT_PANEL_WIDTH, COLOR_ACCENT_DIM);

    display->setTextFont(1);
    display->setTextSize(1);

    display->setTextColor(COLOR_TEXT_DIM, COLOR_HEADER);
    display->setCursor(6, 5);
    display->print("REDES");
    display->setTextColor(COLOR_TEXT, COLOR_HEADER);
    display->setCursor(6, 16);
    display->print(redesEncontradas);

    display->setTextColor(COLOR_TEXT_DIM, COLOR_HEADER);
    display->setCursor(60, 5);
    display->print("NUEVAS");
    display->setTextColor(COLOR_WARN, COLOR_HEADER);
    display->setCursor(60, 16);
    display->print(redesNuevas);

    // Etiqueta de estado a la derecha: bloque cuadrado relleno con el
    // color de colorEstado(), texto oscuro encima para que resalte
    // sobre la barra azul igual que resaltaba sobre el negro antes.
    uint16_t color = colorEstado();
    const char* nombre = nombreEstado();
    int anchoPildora = 6 + strlen(nombre) * 6;
    int xPildora = TFT_PANEL_WIDTH - anchoPildora - 8;
    display->fillRect(xPildora, 8, anchoPildora, 16, color);
    display->setTextColor(COLOR_BG, color);
    display->setCursor(xPildora + 3, 12);
    display->print(nombre);

    // Barra de "actividad": cuanto mas redes nuevas encontro en lo que
    // va de la sesion, mas llena.
    int anchoBarra = TFT_PANEL_WIDTH - 12;
    display->drawRect(6, 26, anchoBarra, 4, COLOR_TEXT_DIM);
    int lleno = redesNuevas * 4;
    if (lleno > anchoBarra - 2) lleno = anchoBarra - 2;
    if (lleno > 0) {
        display->fillRect(7, 27, lleno, 2, COLOR_ACCENT);
    }
}

void Mascota::dibujarCara() {
    int x = spriteX;
    int y = spriteY;
    unsigned long transcurrido = millis() - estadoStartTime;

    // Cada estado tiene 2 frames (ver MascotaCaras.h / tools/generar_caras.py).
    // Para HAPPY y CURIOSA, que duran poco y vuelven solas a IDLE (ver
    // update()), el frame 2 entra a mitad de camino para que se alcancen
    // a ver las dos poses antes de que el estado termine. IDLE, SCANNING
    // y SLEEP no tienen limite de tiempo, asi que ahi alternamos solas
    // cada ~4s via frameAnimacion.
    bool frame2;
    switch (estadoActual) {
        case ESTADO_HAPPY:
            // sonrisa grande primero, ojitos entrecerrados de contenta despues
            frame2 = transcurrido > 1500;
            // ademas un pequeno salto en vez del vaiven lento, para que
            // se note mas que esta contenta
            y += ((frameAnimacion % 8) < 4) ? -4 : 0;
            break;
        case ESTADO_CURIOSA:
            // primero la ceja levantada (duda), despues la sorpresa de
            // la red nueva (ojos bien abiertos)
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

    // La cara se puede mover (bobbing, salto) y ademas HAPPY/CURIOSA
    // dibujan texto al costado, asi que hay que limpiar una zona mas
    // grande que el bitmap mismo para no dejar restos del frame
    // anterior ni del texto de un estado previo.
    display->fillRect(x - 10, spriteY - 40, CARA_ANCHO + 110, CARA_ALTO + 60, COLOR_BG);

    // Bitmap monocromo (1 bit por pixel, igual que los iconos de
    // Iconos.h): un solo color de "tinta" por llamada, nada de
    // pushImage ni setSwapBytes (eso era especifico del sprite RGB565
    // de antes). El color de la cara es el mismo que ya usa la
    // pildora de estado del encabezado, para que las dos cosas
    // combinen.
    const uint8_t* cara = caraParaEstado(estadoActual, frame2);
    display->drawBitmap(x, y, cara, CARA_ANCHO, CARA_ALTO, colorEstado(), COLOR_BG);

    // HAPPY y CURIOSA llevan ademas un indicador de texto al costado -
    // SCANNING y SLEEP ya no lo necesitan porque sus frames 2 (la onda
    // de radar corrida y las "Z" apiladas) ya traen ese detalle
    // dibujado en la cara.
    if (estadoActual == ESTADO_HAPPY) {
        display->setTextColor(COLOR_ACCENT, COLOR_BG);
        display->setTextFont(2);
        display->setCursor(x + CARA_ANCHO + 10, y + CARA_ALTO / 2 - 10);
        display->print("WII!");
        display->setTextFont(1);
    } else if (estadoActual == ESTADO_CURIOSA) {
        // Antes esta cara era la de "ataque" (enojada, con rayos). Ahora
        // es solo curiosidad, para cuando aparece una red que no estaba
        // en el historial. Nada de rayos ni caras de bronca, total la
        // mascota ya no ataca nada.
        display->setTextColor(COLOR_WARN, COLOR_BG);
        display->setTextFont(2);
        display->setCursor(x + CARA_ANCHO + 10, y + CARA_ALTO / 2 - 10);
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
    if (tx >= spriteX && tx <= spriteX + CARA_ANCHO &&
        ty >= spriteY && ty <= spriteY + CARA_ALTO) {
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
