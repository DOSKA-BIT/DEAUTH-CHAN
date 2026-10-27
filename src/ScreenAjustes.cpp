// Pantalla de la categoria AJUSTES (CONFIG). No tiene toque asociado
// (no hay case para SCREEN_CONFIG en manejarToque()). Movido tal cual
// desde main.cpp, sin cambiar ningun comportamiento.
#include "Globals.h"

// Repinta solo los numeros de diagnostico que cambian con el tiempo
// (heap libre, heap minimo, uptime). A proposito NO toca la SD ni
// nada del resto de la pantalla: esto se llama una vez por segundo
// mientras estamos parados en Config, y si remontara la SD en cada
// llamada volveriamos al mismo problema de lentitud que ya arreglamos
// en Wardrive/Stats.
void actualizarDiagnosticoHardware() {
    const int yBloque = CONTENT_TOP + 136;
    // Alto fijo para las 3 lineas (heap libre, heap minimo, uptime);
    // se limpia entero antes de reescribir por si un numero se achica
    // de 3 a 2 digitos y deja basura del dibujo anterior.
    tft.fillRect(0, yBloque - 6, TFT_PANEL_WIDTH, 66, COLOR_BG);

    uint32_t heapLibre = ESP.getFreeHeap() / 1024;
    uint32_t heapMinimo = ESP.getMinFreeHeap() / 1024;

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yBloque);
    tft.print("Heap libre: ");
    // Por debajo de 20KB el ESP32 empieza a fallar cosas como el WiFi
    // scan o el TLS, asi que marco en rojo cuando se acerca a eso en
    // vez de esperar a que algo explote para enterarme.
    tft.setTextColor(heapLibre > 20 ? COLOR_TEXT : COLOR_DANGER, COLOR_BG);
    tft.print(heapLibre);
    tft.print(" KB");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yBloque + 20);
    tft.print("Heap minimo: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(heapMinimo);
    tft.print(" KB");

    unsigned long segundos = millis() / 1000;
    int horas = segundos / 3600;
    int minutos = (segundos % 3600) / 60;
    int segs = segundos % 60;

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yBloque + 40);
    tft.print("Encendido: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    char uptimeBuf[16];
    sprintf(uptimeBuf, "%02d:%02d:%02d", horas, minutos, segs);
    tft.print(uptimeBuf);
}

void dibujarPantallaConfig() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CONFIG", COLOR_CAT_BLUE, ICONO_CONFIG);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 35);
    tft.print("Placa: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(BOARD_NAME);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 55);
    tft.print("Pantalla: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(TFT_PANEL_WIDTH);
    tft.print("x");
    tft.print(TFT_PANEL_HEIGHT);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 75);
    tft.print("Touch: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(TOUCH_IS_CAPACITIVE ? "Capacitivo" : "Resistivo");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 95);
    tft.print("SD: ");
    SD.end();
    bool sdOk = SD.begin(SD_CS_PIN);
    tft.setTextColor(sdOk ? COLOR_CAT_BLUE : COLOR_DANGER, COLOR_BG);
    tft.print(sdOk ? "OK" : "No detectada");

    // La frecuencia de CPU no cambia sola en tiempo de ejecucion (no
    // llamamos a setCpuFrequencyMhz en ningun lado), asi que va junto
    // con los datos "de una vez" de arriba y no en el bloque que se
    // refresca cada segundo.
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 115);
    tft.print("CPU: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(ESP.getCpuFreqMHz());
    tft.print(" MHz");

    // Diagnostico de hardware: nada de esto pide libreria nueva, son
    // funciones que ya trae el core de Arduino-ESP32. Los numeros en
    // si se pintan aparte (actualizarDiagnosticoHardware) porque ese
    // bloque se repinta solo, una vez por segundo.
    tft.drawFastHLine(10, CONTENT_TOP + 132, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);
    actualizarDiagnosticoHardware();

    // Esto se dibuja una sola vez, al entrar a la pantalla - no hace
    // falta repetirlo en actualizarDiagnosticoHardware() porque el
    // texto no cambia nunca.
    dibujarPieAyuda("Info de la placa y el estado de la SD");

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        touchDriver.reclaimBus();
    #endif
}
