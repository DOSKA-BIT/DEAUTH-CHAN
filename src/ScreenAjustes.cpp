// Pantalla de la categoria AJUSTES (CONFIG). Hasta esta vuelta era
// pura lectura (diagnostico de la placa) y no tenia toque asociado -
// el usuario pidio sumarle brillo de pantalla, tiempo de autosleep y
// una forma de borrar el historial de redes conocidas sin reflashear,
// asi que ahora SI tiene case en manejarToque() (ver main.cpp).
#include "Globals.h"

// ---- Layout de las 3 filas de controles (brillo / autosleep /
// borrar). Los mismos numeros los usa manejarToqueConfig() para saber
// donde cayo el dedo - si se cambia un valor aca hay que cambiarlo
// alla tambien, mismo criterio que ya usa el Analizador de canales en
// ScreenWifi.cpp para sus controles de Channel/X/Y.
static const int Y_FILA_BRILLO     = CONTENT_TOP + 122;
static const int Y_FILA_AUTOSLEEP  = CONTENT_TOP + 150;
static const int Y_FILA_BORRAR     = CONTENT_TOP + 178;
static const int ALTO_FILA_AJUSTES = 28; // separacion entre el inicio de una fila y la siguiente
static const int ANCHO_BTN_AJUSTES = 22;
static const int ALTO_BTN_AJUSTES  = 22;

// Estado de "armado" del boton de borrar - no es un enum de la pantalla
// completa (eso ya es MenuScreen), es nada mas la maquinita de 3
// estados de ESTA fila: hace falta tocarla dos veces (NORMAL ->
// ARMADO -> borra y pasa a HECHO) para que un toque sin querer no
// tire el historial entero. Vive aca como estatico de archivo, igual
// que "momentoDestello" vive en main.cpp, porque es puramente visual
// y no hace falta que ningun otro Screen*.cpp lo vea.
enum EstadoBorrar { BORRAR_NORMAL, BORRAR_ARMADO, BORRAR_HECHO };
static EstadoBorrar estadoBorrar = BORRAR_NORMAL;
static unsigned long momentoEstadoBorrar = 0;
static const unsigned long ARMADO_BORRAR_MS = 4000; // si no confirmas en 4s, se desarma solo
static const unsigned long HECHO_BORRAR_MS  = 1500; // cuanto se queda mostrando "listo"

// Repinta solo los numeros de diagnostico que cambian con el tiempo
// (heap libre/minimo, uptime, CPU). A proposito NO toca la SD ni nada
// del resto de la pantalla: esto se llama una vez por segundo
// mientras estamos parados en Config, y si remontara la SD en cada
// llamada volveriamos al mismo problema de lentitud que ya arreglamos
// en Wardrive/Stats.
void actualizarDiagnosticoHardware() {
    // Rango acotado para no pisar la linea punteada de arriba (y=80)
    // ni la de abajo (y=118) - antes este bloque tenia toda la
    // pantalla para el, ahora comparte espacio con las filas nuevas
    // de AJUSTES y hay que ser mas quirurgico con el area que se
    // limpia.
    const int yBloque = CONTENT_TOP + 86;
    tft.fillRect(0, yBloque - 3, TFT_PANEL_WIDTH, 34, COLOR_BG);

    uint32_t heapLibre = ESP.getFreeHeap() / 1024;
    uint32_t heapMinimo = ESP.getMinFreeHeap() / 1024;

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yBloque);
    tft.print("Heap: ");
    // Por debajo de 20KB el ESP32 empieza a fallar cosas como el WiFi
    // scan o el TLS, asi que marco en rojo cuando se acerca a eso en
    // vez de esperar a que algo explote para enterarme.
    tft.setTextColor(heapLibre > 20 ? COLOR_TEXT : COLOR_DANGER, COLOR_BG);
    tft.print(heapLibre);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print("/");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(heapMinimo);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print(" KB libre/minimo");

    unsigned long segundos = millis() / 1000;
    int horas = segundos / 3600;
    int minutos = (segundos % 3600) / 60;
    int segs = segundos % 60;

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yBloque + 16);
    tft.print("Uptime: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    char uptimeBuf[12];
    sprintf(uptimeBuf, "%02d:%02d:%02d", horas, minutos, segs);
    tft.print(uptimeBuf);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print("  CPU ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(ESP.getCpuFreqMHz());
    tft.print("MHz");
}

// Dibuja (o redibuja) la fila de Brillo. Separada de
// dibujarPantallaConfig() para poder repintar SOLO esta fila despues
// de un toque en - o +, en vez de volver a dibujar toda la pantalla
// (mismo criterio de "repintar lo minimo" que el resto del firmware).
static void dibujarFilaBrillo() {
    tft.fillRect(0, Y_FILA_BRILLO, TFT_PANEL_WIDTH, ALTO_FILA_AJUSTES, COLOR_BG);

    int xMas = TFT_PANEL_WIDTH - ANCHO_BTN_AJUSTES - 10;
    int xMenos = xMas - ANCHO_BTN_AJUSTES - 60;
    int yBtn = Y_FILA_BRILLO + 3;

    dibujarTextoEspaciado(10, yBtn + ALTO_BTN_AJUSTES / 2 - 3, "BRILLO", COLOR_TEXT_DIM, COLOR_BG, 1);

    dibujarEsquinas(xMenos, yBtn, ANCHO_BTN_AJUSTES, ALTO_BTN_AJUSTES, COLOR_ACCENT_DIM, 3);
    dibujarTextoEspaciado(xMenos + 7, yBtn + ALTO_BTN_AJUSTES / 2 - 3, "-", COLOR_TEXT, COLOR_BG, 0);

    dibujarEsquinas(xMas, yBtn, ANCHO_BTN_AJUSTES, ALTO_BTN_AJUSTES, COLOR_ACCENT_DIM, 3);
    dibujarTextoEspaciado(xMas + 7, yBtn + ALTO_BTN_AJUSTES / 2 - 3, "+", COLOR_TEXT, COLOR_BG, 0);

    char valor[6];
    snprintf(valor, sizeof(valor), "%d%%", ajustes.getBrillo());
    int anchoValor = anchoTextoEspaciado(valor, 1);
    int xValor = xMenos + ANCHO_BTN_AJUSTES + ((xMas - (xMenos + ANCHO_BTN_AJUSTES)) - anchoValor) / 2;
    dibujarTextoEspaciado(xValor, yBtn + ALTO_BTN_AJUSTES / 2 - 3, valor, COLOR_TEXT, COLOR_BG, 1);
}

// Idem para Autosleep. Muestra "NUNCA" en vez de un numero cuando el
// usuario eligio 0 (deshabilitado) - ver PRESETS_AUTOSLEEP en
// Settings.cpp.
static void dibujarFilaAutosleep() {
    tft.fillRect(0, Y_FILA_AUTOSLEEP, TFT_PANEL_WIDTH, ALTO_FILA_AJUSTES, COLOR_BG);

    int xMas = TFT_PANEL_WIDTH - ANCHO_BTN_AJUSTES - 10;
    int xMenos = xMas - ANCHO_BTN_AJUSTES - 60;
    int yBtn = Y_FILA_AUTOSLEEP + 3;

    dibujarTextoEspaciado(10, yBtn + ALTO_BTN_AJUSTES / 2 - 3, "AUTOSLEEP", COLOR_TEXT_DIM, COLOR_BG, 1);

    dibujarEsquinas(xMenos, yBtn, ANCHO_BTN_AJUSTES, ALTO_BTN_AJUSTES, COLOR_ACCENT_DIM, 3);
    dibujarTextoEspaciado(xMenos + 7, yBtn + ALTO_BTN_AJUSTES / 2 - 3, "-", COLOR_TEXT, COLOR_BG, 0);

    dibujarEsquinas(xMas, yBtn, ANCHO_BTN_AJUSTES, ALTO_BTN_AJUSTES, COLOR_ACCENT_DIM, 3);
    dibujarTextoEspaciado(xMas + 7, yBtn + ALTO_BTN_AJUSTES / 2 - 3, "+", COLOR_TEXT, COLOR_BG, 0);

    int minutos = ajustes.getAutosleepMinutos();
    char valor[8];
    if (minutos == 0) {
        snprintf(valor, sizeof(valor), "NUNCA");
    } else {
        snprintf(valor, sizeof(valor), "%dm", minutos);
    }
    int anchoValor = anchoTextoEspaciado(valor, 1);
    int xValor = xMenos + ANCHO_BTN_AJUSTES + ((xMas - (xMenos + ANCHO_BTN_AJUSTES)) - anchoValor) / 2;
    dibujarTextoEspaciado(xValor, yBtn + ALTO_BTN_AJUSTES / 2 - 3, valor, COLOR_TEXT, COLOR_BG, 1);
}

// Fila de "Borrar redes guardadas", en sus 3 estados posibles (ver
// EstadoBorrar arriba). Es una sola caja que ocupa casi todo el
// ancho, no un grupo -/+ como las otras dos filas.
static void dibujarFilaBorrar() {
    tft.fillRect(0, Y_FILA_BORRAR, TFT_PANEL_WIDTH, ALTO_FILA_AJUSTES, COLOR_BG);

    int xCaja = 10;
    int anchoCaja = TFT_PANEL_WIDTH - 20;
    int yCaja = Y_FILA_BORRAR + 3;

    const char* texto;
    uint16_t color;
    switch (estadoBorrar) {
        case BORRAR_ARMADO:
            texto = "SEGURO? TOCA DE NUEVO";
            color = COLOR_DANGER;
            break;
        case BORRAR_HECHO:
            texto = "LISTO, SE BORRARON";
            color = COLOR_ACCENT;
            break;
        default:
            texto = "BORRAR REDES GUARDADAS";
            color = COLOR_TEXT_DIM;
            break;
    }

    dibujarEsquinas(xCaja, yCaja, anchoCaja, ALTO_BTN_AJUSTES, color, 4);
    int anchoTexto = anchoTextoEspaciado(texto, 1);
    int xTexto = xCaja + (anchoCaja - anchoTexto) / 2;
    dibujarTextoEspaciado(xTexto, yCaja + ALTO_BTN_AJUSTES / 2 - 3, texto, color, COLOR_BG, 1);
}

void actualizarEstadoBorrado() {
    if (estadoBorrar == BORRAR_ARMADO && millis() - momentoEstadoBorrar > ARMADO_BORRAR_MS) {
        estadoBorrar = BORRAR_NORMAL;
        dibujarFilaBorrar();
    } else if (estadoBorrar == BORRAR_HECHO && millis() - momentoEstadoBorrar > HECHO_BORRAR_MS) {
        estadoBorrar = BORRAR_NORMAL;
        dibujarFilaBorrar();
    }
}

void dibujarPantallaConfig() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CONFIG", COLOR_CAT_BLUE, ICONO_CONFIG);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 34);
    tft.print("Placa: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(BOARD_NAME);

    // Pantalla y Touch comparten linea (antes eran 2 lineas separadas)
    // para hacer lugar a los controles nuevos de mas abajo sin que la
    // pantalla se quede sin espacio en las placas de 240x320.
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 50);
    tft.print("Pant: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(TFT_PANEL_WIDTH);
    tft.print("x");
    tft.print(TFT_PANEL_HEIGHT);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print("  Touch: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(TOUCH_IS_CAPACITIVE ? "Capacitivo" : "Resistivo");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 66);
    tft.print("SD: ");
    SD.end();
    bool sdOk = SD.begin(SD_CS_PIN);
    tft.setTextColor(sdOk ? COLOR_CAT_BLUE : COLOR_DANGER, COLOR_BG);
    tft.print(sdOk ? "OK" : "No detectada");

    // Diagnostico de hardware: nada de esto pide libreria nueva, son
    // funciones que ya trae el core de Arduino-ESP32. Los numeros en
    // si se pintan aparte (actualizarDiagnosticoHardware) porque ese
    // bloque se repinta solo, una vez por segundo.
    dibujarLineaPunteada(10, CONTENT_TOP + 80, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);
    actualizarDiagnosticoHardware();
    dibujarLineaPunteada(10, CONTENT_TOP + 118, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);

    // Controles de usuario: brillo, autosleep, borrar historial. A
    // diferencia de todo lo de arriba (que es de solo lectura), esto
    // SI reacciona al toque - ver manejarToqueConfig() mas abajo.
    dibujarFilaBrillo();
    dibujarFilaAutosleep();
    dibujarFilaBorrar();

    dibujarPieAyuda("Toca - / + para ajustar, o Borrar redes abajo");

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        touchDriver.reclaimBus();
    #endif
}

// Aplica el brillo actual (ajustes.getBrillo(), 10-100) al backlight
// real via PWM. Se llama cada vez que el usuario toca - o + en la
// fila de Brillo - nunca desde el dibujado solo, para no tocar el
// hardware en cada repintado de pantalla sin necesidad.
static void aplicarBrilloAlBacklight() {
    int duty = map(ajustes.getBrillo(), 0, 100, 0, 255);
    ledcWrite(BACKLIGHT_LEDC_CHANNEL, duty);
}

void manejarToqueConfig(int x, int y) {
    // Fila BRILLO
    if (y >= Y_FILA_BRILLO && y < Y_FILA_BRILLO + ALTO_FILA_AJUSTES) {
        int xMas = TFT_PANEL_WIDTH - ANCHO_BTN_AJUSTES - 10;
        int xMenos = xMas - ANCHO_BTN_AJUSTES - 60;
        int yBtn = Y_FILA_BRILLO + 3;
        if (y < yBtn || y > yBtn + ALTO_BTN_AJUSTES) return;

        if (x >= xMenos && x <= xMenos + ANCHO_BTN_AJUSTES) {
            ajustes.setBrillo(ajustes.getBrillo() - 10);
            aplicarBrilloAlBacklight();
            dibujarFilaBrillo();
        } else if (x >= xMas && x <= xMas + ANCHO_BTN_AJUSTES) {
            ajustes.setBrillo(ajustes.getBrillo() + 10);
            aplicarBrilloAlBacklight();
            dibujarFilaBrillo();
        }
        return;
    }

    // Fila AUTOSLEEP
    if (y >= Y_FILA_AUTOSLEEP && y < Y_FILA_AUTOSLEEP + ALTO_FILA_AJUSTES) {
        int xMas = TFT_PANEL_WIDTH - ANCHO_BTN_AJUSTES - 10;
        int xMenos = xMas - ANCHO_BTN_AJUSTES - 60;
        int yBtn = Y_FILA_AUTOSLEEP + 3;
        if (y < yBtn || y > yBtn + ALTO_BTN_AJUSTES) return;

        if (x >= xMenos && x <= xMenos + ANCHO_BTN_AJUSTES) {
            ajustes.bajarAutosleep();
            dibujarFilaAutosleep();
        } else if (x >= xMas && x <= xMas + ANCHO_BTN_AJUSTES) {
            ajustes.subirAutosleep();
            dibujarFilaAutosleep();
        }
        return;
    }

    // Fila BORRAR REDES GUARDADAS: primer toque arma, segundo toque
    // (dentro de ARMADO_BORRAR_MS) confirma de verdad. Mientras
    // muestra "LISTO" (BORRAR_HECHO) no reacciona a mas toques, para
    // no re-armarse sin querer apenas termina de borrar.
    if (y >= Y_FILA_BORRAR && y < Y_FILA_BORRAR + ALTO_FILA_AJUSTES) {
        if (estadoBorrar == BORRAR_NORMAL) {
            estadoBorrar = BORRAR_ARMADO;
            momentoEstadoBorrar = millis();
            dibujarFilaBorrar();
        } else if (estadoBorrar == BORRAR_ARMADO) {
            aprendizaje.borrarHistorial();
            eventos.registrar("Historial de redes borrado", nullptr);
            estadoBorrar = BORRAR_HECHO;
            momentoEstadoBorrar = millis();
            dibujarFilaBorrar();
        }
        return;
    }
}
