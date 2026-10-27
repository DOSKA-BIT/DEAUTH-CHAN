// Pantalla de la categoria RADAR. No tiene toque asociado (no hay
// case para SCREEN_RADAR en manejarToque()). Movido tal cual desde
// main.cpp, sin cambiar ningun comportamiento.
#include <math.h>
#include "Globals.h"

// RADAR: barrido giratorio (ver el comentario largo en BLEScanner.h
// de que detecta esto de verdad y que no). Cuanto tarda el barrido en
// dar una vuelta completa, y cada cuanto se repinta la pantalla
// mientras estamos parados en ella (ver el bloque de refresco en
// loop(), en main.cpp, caso SCREEN_RADAR) - mas chico el segundo
// numero = barrido mas fluido, a costa de redibujar mas seguido.
const unsigned long RADAR_BARRIDO_MS = 4000;
const unsigned long RADAR_REFRESCO_MS = 150;

// Angulo "de pantalla" fijo para un dispositivo, sacado de su MAC.
// OJO, esto es puramente visual, NO es la direccion real hacia donde
// esta el dispositivo: ni el ESP32 ni una antena BLE comun como la de
// esta placa pueden saber de que lado viene una señal, solo que tan
// fuerte llega. Sin esto cada dispositivo aparecería siempre pegado
// al centro en el mismo punto (0 grados), lo que se superpondría con
// cualquier otro y además no se vería nada parecido a un radar. Lo
// unico que SI es un dato real es la distancia al centro (mas abajo),
// que sale de la señal (RSSI).
int anguloVisualBLE(const uint8_t* mac) {
    return (mac[2] * 7 + mac[3] * 13 + mac[4] * 19 + mac[5] * 29) % 360;
}

// RADAR: barrido circular en vivo (ver el comentario largo en
// BLEScanner.h de que detecta esto de verdad y que no - sigue
// aplicando igual aca, esto solo cambio como se MUESTRA el mismo
// dato). Centro = señal fuerte (dispositivo "cerca"), borde = señal
// debil ("lejos") - la distancia al centro sale del RSSI real de
// cada dispositivo. El angulo alrededor del centro NO es la
// direccion real (ver anguloVisualBLE) - es nada mas para que cada
// dispositivo tenga su propio lugar fijo en pantalla y no se pisen
// todos en el mismo punto.
void dibujarPantallaRadar() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("RADAR", COLOR_CAT_GREEN, ICONO_RADAR);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    int altoDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int cx = TFT_PANEL_WIDTH / 2;
    int cy = top + altoDisponible / 2;
    int radio = min(TFT_PANEL_WIDTH / 2 - 12, altoDisponible / 2 - 6);

    // Anillos de referencia + cruz del medio - la "grilla" clasica de
    // un radar, puramente decorativa.
    tft.drawCircle(cx, cy, radio, COLOR_PANEL);
    tft.drawCircle(cx, cy, (radio * 2) / 3, COLOR_PANEL);
    tft.drawCircle(cx, cy, radio / 3, COLOR_PANEL);
    tft.drawFastHLine(cx - radio, cy, radio * 2, COLOR_PANEL);
    tft.drawFastVLine(cx, cy - radio, radio * 2, COLOR_PANEL);

    // Barrido giratorio: una linea principal + dos mas atras (mas
    // oscuras) para que se note que "gira" en vez de ser una aguja
    // fija saltando de a tirones.
    float anguloBarrido = (millis() % RADAR_BARRIDO_MS) * 360.0 / RADAR_BARRIDO_MS;
    for (int i = 0; i < 3; i++) {
        float ang = radians(anguloBarrido - i * 10);
        uint16_t colorLinea = (i == 0) ? COLOR_CAT_GREEN : COLOR_ACCENT_DIM;
        int x2 = cx + (int)(cos(ang) * radio);
        int y2 = cy + (int)(sin(ang) * radio);
        tft.drawLine(cx, cy, x2, y2, colorLinea);
    }

    int numDisp = 0;
    DispositivoBLE* dispositivos = radar.getDispositivos(numDisp);
    unsigned long ahora = millis();
    int rssiMasFuerte = -999;

    for (int i = 0; i < numDisp; i++) {
        DispositivoBLE& d = dispositivos[i];
        if (ahora - d.ultimaVez > 30000) continue; // mas de 30s sin escucharlo: no lo pintamos mas

        // RSSI tipico de BLE anda entre -30 (pegado) y -95 (al limite
        // del alcance) - fuera de ese rango lo recortamos para que un
        // pico raro no mande el punto afuera del circulo.
        int rssi = d.rssi;
        if (rssi > -30) rssi = -30;
        if (rssi < -95) rssi = -95;
        float t = (float)(rssi - (-30)) / ((-95) - (-30)); // 0 = cerca, 1 = lejos
        int distancia = (int)(t * radio);

        float ang = radians(anguloVisualBLE(d.mac));
        int x = cx + (int)(cos(ang) * distancia);
        int y = cy + (int)(sin(ang) * distancia);

        uint16_t color = d.esFindMyApple ? COLOR_DANGER : COLOR_CAT_GREEN;
        tft.fillCircle(x, y, 3, color);
        if (d.esFindMyApple) {
            tft.drawCircle(x, y, 6, COLOR_DANGER); // aro extra para que resalte entre los demas
        }

        if (rssi > rssiMasFuerte) rssiMasFuerte = rssi;
    }

    unsigned long totalVistos = 0, totalFindMy = 0;
    radar.getStats(totalVistos, totalFindMy);

    char pie[44];
    if (numDisp == 0) {
        snprintf(pie, sizeof(pie), "Buscando...");
    } else if (totalFindMy > 0) {
        snprintf(pie, sizeof(pie), "Rojo = red Find My (no es 100%% un AirTag)");
    } else {
        snprintf(pie, sizeof(pie), "%d cerca - centro = senial mas fuerte", numDisp);
    }
    dibujarPieAyuda(pie);
}
