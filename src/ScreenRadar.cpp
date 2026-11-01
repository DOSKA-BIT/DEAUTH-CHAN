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
    // Reusa la clave de nombre de la herramienta (TXT_HERR_RADAR_NOMBRE,
    // "RADAR" en los dos idiomas) en vez de una clave de titulo aparte
    // - mismo criterio que RASTREADOR mas abajo.
    dibujarTituloSeccion(tr(TXT_HERR_RADAR_NOMBRE), COLOR_CAT_GREEN, ICONO_RADAR);
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

    // Cuantos de los que hay en memoria se dibujan DE VERDAD esta
    // vuelta (pasaron el filtro de 30s) y si alguno de ESOS es Find My
    // - bug real que encontre repasando este archivo (no reportado):
    // antes el pie de pagina usaba "numDisp" (el total guardado, que
    // puede incluir dispositivos que hace rato no se escuchan) para
    // decir "X cerca", mientras que el radar solo pintaba los puntos
    // que pasaban el filtro de 30s - con la radio llena de
    // dispositivos viejos, el numero del pie no coincidia con los
    // puntos que se veian en pantalla. Y la advertencia de Find My
    // usaba el contador HISTORICO de toda la sesion (totalFindMy,
    // que una vez que sube ya no baja nunca), asi que una vez que
    // aparecia UN dispositivo asi, el pie se quedaba mostrando esa
    // advertencia para siempre, aunque ese dispositivo ya no se
    // escuche mas y no haya ningun punto rojo en pantalla.
    int numVisibles = 0;
    bool hayFindMyVisible = false;

    for (int i = 0; i < numDisp; i++) {
        DispositivoBLE& d = dispositivos[i];
        if (ahora - d.ultimaVez > 30000) continue; // mas de 30s sin escucharlo: no lo pintamos mas
        numVisibles++;
        if (d.esFindMyApple) hayFindMyVisible = true;

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
    }

    if (numVisibles == 0) {
        dibujarPieAyuda(tr(TXT_RADAR_PIE_BUSCANDO));
    } else if (hayFindMyVisible) {
        dibujarPieAyuda(tr(TXT_RADAR_PIE_FINDMY));
    } else {
        // Unico caso con un valor que cambia (numVisibles): el texto
        // traducido viene con un "%d" adentro, como formato de
        // snprintf - mismo patron que el resumen de escaneo en
        // main.cpp, pero tomando el formato desde la tabla de idioma
        // en vez de hardcodeado.
        char pie[48];
        snprintf(pie, sizeof(pie), tr(TXT_RADAR_PIE_CERCA), numVisibles);
        dibujarPieAyuda(pie);
    }
}

// ---------------------------------------------------------------------
// RASTREADOR (tracker finder)
//
// Segunda herramienta de la categoria RADAR: en vez de pintar TODOS
// los dispositivos BLE en el barrido circular (eso ya lo hace RADAR),
// esta pantalla sigue a UNO SOLO con un medidor de senial grande, para
// poder caminar y ubicarlo por "mas cerca/mas lejos" - util para
// encontrar un tracker (AirTag u otro) que ya detectaste en RADAR, o
// cualquier otro dispositivo BLE puntual. Sigue siendo 100% recepcion
// pasiva: lo unico que lee es el RSSI de los advertisements que ya
// escucha BLEScanner de fondo (ver el comentario largo en
// BLEScanner.h) - no se manda nada al aire para "buscar" nada.
//
// Dos modos, controlados por rastreadorModo:
//   0 = lista de seleccion (los mismos dispositivos que ve RADAR,
//       filtrados igual por los ultimos 30s)
//   1 = siguiendo: medidor de senial en vivo del dispositivo elegido,
//       identificado por su MAC (rastreadorMac). Tocar la pantalla en
//       este modo vuelve a la lista - no hay otro control que hitear.

static int rastreadorModo = 0;
static uint8_t rastreadorMac[6] = {0, 0, 0, 0, 0, 0};
static char rastreadorNombre[24] = "";
static bool rastreadorEraFindMy = false;

// Tendencia (acercandose/alejandose), recalculada cada
// RASTREADOR_TREND_MS en vez de en cada refresco de 150ms - el RSSI
// crudo salta demasiado de una lectura a la siguiente como para sacar
// una tendencia confiable comparando dos lecturas seguidas.
static const unsigned long RASTREADOR_TREND_MS = 800;
static unsigned long rastreadorUltimoTrend = 0;
static int rastreadorRssiAnterior = 0;
static int rastreadorTrend = 0; // -1 alejandose, 0 sin cambio, 1 acercandose

static const int GAP_LISTA_RASTREADOR = 3;

// Mismo filtro de "ultimos 30s" que usa dibujarPantallaRadar() de
// arriba, factorizado aca porque lo necesitan tanto el dibujado como
// el toque de la lista de seleccion, y tienen que coincidir exacto o
// un toque podria seleccionar un dispositivo distinto al que se ve.
static int listaVisiblesRastreador(DispositivoBLE* dispositivos, int numDisp, int* indicesOut) {
    unsigned long ahora = millis();
    int n = 0;
    for (int i = 0; i < numDisp; i++) {
        if (ahora - dispositivos[i].ultimaVez > 30000) continue;
        indicesOut[n++] = i;
    }
    return n;
}

// Alto de fila + Y de arranque para la lista de seleccion - misma
// logica de "repartir el espacio disponible entre la cantidad real de
// filas" que layoutHerramientasDeCategoria() en Menu.cpp, para que la
// lista se vea bien tanto con 1 dispositivo como con los 12 maximos.
static void layoutListaRastreador(int cantidad, int& altoFila, int& yInicio) {
    int espacioDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int alto = (cantidad > 0)
        ? (espacioDisponible - GAP_LISTA_RASTREADOR * (cantidad - 1)) / cantidad
        : espacioDisponible;
    if (alto > 26) alto = 26;
    if (alto < 16) alto = 16;
    altoFila = alto;
    yInicio = CONTENT_TOP + ALTO_TITULO_SECCION;
}

static void dibujarRastreadorLista() {
    int numDisp = 0;
    DispositivoBLE* dispositivos = radar.getDispositivos(numDisp);
    int indices[MAX_DISPOSITIVOS_BLE];
    int cantidad = listaVisiblesRastreador(dispositivos, numDisp, indices);

    if (cantidad == 0) {
        dibujarPieAyuda(tr(TXT_RASTREADOR_PIE_BUSCANDO));
        return;
    }

    int altoFila, yInicio;
    layoutListaRastreador(cantidad, altoFila, yInicio);

    for (int k = 0; k < cantidad; k++) {
        DispositivoBLE& d = dispositivos[indices[k]];
        int y = yInicio + k * (altoFila + GAP_LISTA_RASTREADOR);

        uint16_t color = d.esFindMyApple ? COLOR_DANGER : COLOR_CAT_GREEN;
        dibujarEsquinas(2, y, TFT_PANEL_WIDTH - 4, altoFila, color, 3);

        char etiqueta[28];
        if (d.nombre[0] != '\0') {
            snprintf(etiqueta, sizeof(etiqueta), "%s", d.nombre);
        } else {
            // Sin nombre anunciado: identificamos el dispositivo por
            // los ultimos 2 bytes de su MAC, suficiente para
            // distinguirlo del resto de la lista sin imprimir la MAC
            // entera (que no aporta nada para elegir "cual es el
            // mio" a simple vista).
            snprintf(etiqueta, sizeof(etiqueta), "..%02X:%02X", d.mac[4], d.mac[5]);
        }
        dibujarTextoEspaciado(10, y + altoFila / 2 - 3, etiqueta, color, COLOR_BG, 2);

        char rssiTxt[8];
        snprintf(rssiTxt, sizeof(rssiTxt), "%d", d.rssi);
        int anchoRssi = anchoTextoEspaciado(rssiTxt);
        dibujarTextoEspaciado(TFT_PANEL_WIDTH - 10 - anchoRssi, y + altoFila / 2 - 3, rssiTxt, color, COLOR_BG, 1);
    }

    dibujarPieAyuda(tr(TXT_RASTREADOR_PIE_LISTA));
}

static void dibujarRastreadorSiguiendo() {
    int numDisp = 0;
    DispositivoBLE* dispositivos = radar.getDispositivos(numDisp);

    int idx = -1;
    for (int i = 0; i < numDisp; i++) {
        if (memcmp(dispositivos[i].mac, rastreadorMac, 6) == 0) {
            idx = i;
            break;
        }
    }

    int top = CONTENT_TOP + ALTO_TITULO_SECCION + 6;

    if (idx == -1) {
        // No deberia pasar casi nunca (el array de BLEScanner solo
        // reemplaza el slot mas viejo cuando esta lleno, ver
        // BLEScanner::onDispositivoVisto) pero si justo paso, avisamos
        // en vez de mostrar numeros viejos como si fueran en vivo.
        dibujarTextoEspaciado(10, top + 30, tr(TXT_RASTREADOR_PERDIDO), COLOR_DANGER, COLOR_BG, 2);
        dibujarPieAyuda(tr(TXT_RASTREADOR_PIE_VOLVER));
        return;
    }

    DispositivoBLE& d = dispositivos[idx];
    unsigned long antiguedad = millis() - d.ultimaVez;

    char etiqueta[28];
    if (rastreadorNombre[0] != '\0') {
        snprintf(etiqueta, sizeof(etiqueta), "%s", rastreadorNombre);
    } else {
        snprintf(etiqueta, sizeof(etiqueta), "..%02X:%02X", rastreadorMac[4], rastreadorMac[5]);
    }
    uint16_t colorEtiqueta = rastreadorEraFindMy ? COLOR_DANGER : COLOR_CAT_GREEN;
    dibujarTextoEspaciado(10, top, etiqueta, colorEtiqueta, COLOR_BG, 2);

    // RSSI numerico grande, mismo recorte -30/-95 que usa el barrido
    // circular de RADAR (ver el comentario largo junto a
    // anguloVisualBLE() arriba) para que el medidor no se vaya de
    // rango con un pico raro.
    int rssi = d.rssi;
    if (rssi > -30) rssi = -30;
    if (rssi < -95) rssi = -95;
    float t = (float)(rssi - (-30)) / ((-95) - (-30)); // 0 = cerca, 1 = lejos

    char rssiTxt[12];
    snprintf(rssiTxt, sizeof(rssiTxt), "%d dBm", d.rssi);
    dibujarTextoEspaciado(10, top + 22, rssiTxt, COLOR_TEXT_DIM, COLOR_BG, 2);

    // Medidor de proximidad: barra segmentada horizontal, mas llena =
    // mas cerca. Reusa dibujarBarraSegmentada() (Menu.cpp), el mismo
    // helper que usa la pantalla CANALES, nada nuevo que mantener.
    int segmentos = 16;
    int llenos = (int)((1.0f - t) * segmentos + 0.5f);
    int yBarra = top + 48;
    int wBarra = TFT_PANEL_WIDTH - 20;
    dibujarBarraSegmentada(10, yBarra, wBarra, 20, segmentos, llenos, COLOR_CAT_GREEN, COLOR_PANEL);
    dibujarEsquinas(10, yBarra, wBarra, 20, COLOR_ACCENT_DIM, 4);

    // Tendencia: se recalcula cada RASTREADOR_TREND_MS comparando
    // contra la lectura anterior, no en cada refresco de 150ms (ver el
    // comentario largo junto a la declaracion de estas estaticas, mas
    // arriba) - un RSSI crudo salta demasiado como para que dos
    // lecturas seguidas digan algo real.
    unsigned long ahora = millis();
    if (ahora - rastreadorUltimoTrend >= RASTREADOR_TREND_MS) {
        int diferencia = d.rssi - rastreadorRssiAnterior;
        if (diferencia >= 3) rastreadorTrend = 1;
        else if (diferencia <= -3) rastreadorTrend = -1;
        else rastreadorTrend = 0;
        rastreadorRssiAnterior = d.rssi;
        rastreadorUltimoTrend = ahora;
    }

    const char* textoTrend;
    uint16_t colorTrend;
    if (antiguedad > 5000) {
        textoTrend = tr(TXT_RASTREADOR_SIN_SENIAL);
        colorTrend = COLOR_TEXT_DIM;
    } else if (rastreadorTrend > 0) {
        textoTrend = tr(TXT_RASTREADOR_ACERCANDOSE);
        colorTrend = COLOR_CAT_GREEN;
    } else if (rastreadorTrend < 0) {
        textoTrend = tr(TXT_RASTREADOR_ALEJANDOSE);
        colorTrend = COLOR_DANGER;
    } else {
        textoTrend = tr(TXT_RASTREADOR_SIN_CAMBIOS);
        colorTrend = COLOR_TEXT_DIM;
    }
    dibujarTextoEspaciado(10, yBarra + 32, textoTrend, colorTrend, COLOR_BG, 2);

    dibujarPieAyuda(tr(TXT_RASTREADOR_PIE_VOLVER));
}

void dibujarPantallaRastreador() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    // Reusa la clave de nombre de la herramienta (TXT_HERR_RASTREADOR_NOMBRE)
    // en vez de una clave de titulo aparte - mismo texto que ya se ve
    // en la lista de Herramientas, nada que mantener duplicado.
    dibujarTituloSeccion(tr(TXT_HERR_RASTREADOR_NOMBRE), COLOR_CAT_GREEN, ICONO_RADAR);

    if (rastreadorModo == 0) {
        dibujarRastreadorLista();
    } else {
        dibujarRastreadorSiguiendo();
    }
}

void manejarToqueRastreador(int x, int y) {
    if (rastreadorModo == 1) {
        // En modo "siguiendo" cualquier toque en la pantalla vuelve a
        // la lista - no hace falta puntear una zona en particular,
        // ahi no hay otra cosa tocable.
        rastreadorModo = 0;
        contenidoSucio = true;
        return;
    }

    int numDisp = 0;
    DispositivoBLE* dispositivos = radar.getDispositivos(numDisp);
    int indices[MAX_DISPOSITIVOS_BLE];
    int cantidad = listaVisiblesRastreador(dispositivos, numDisp, indices);
    if (cantidad == 0) return;

    int altoFila, yInicio;
    layoutListaRastreador(cantidad, altoFila, yInicio);

    for (int k = 0; k < cantidad; k++) {
        int fy = yInicio + k * (altoFila + GAP_LISTA_RASTREADOR);
        if (y >= fy && y <= fy + altoFila) {
            DispositivoBLE& d = dispositivos[indices[k]];
            memcpy(rastreadorMac, d.mac, 6);
            strncpy(rastreadorNombre, d.nombre, sizeof(rastreadorNombre) - 1);
            rastreadorNombre[sizeof(rastreadorNombre) - 1] = '\0';
            rastreadorEraFindMy = d.esFindMyApple;

            // Arranca la tendencia desde esta misma lectura, para que
            // el primer calculo (a los RASTREADOR_TREND_MS) compare
            // contra un valor real y no contra el 0 de arranque.
            rastreadorRssiAnterior = d.rssi;
            rastreadorTrend = 0;
            rastreadorUltimoTrend = millis();

            rastreadorModo = 1;
            contenidoSucio = true;
            return;
        }
    }
}
