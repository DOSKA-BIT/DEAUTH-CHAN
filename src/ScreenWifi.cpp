// Todas las pantallas de la categoria WIFI (ESCANEAR, STATS,
// CONOCIDAS, MONITOR, EVENTOS, CANALES, DUPLICADO y el placeholder
// GAME) + sus toques asociados. Movido tal cual desde main.cpp, sin
// cambiar ningun comportamiento - cada funcion es copia exacta de la
// que tenia antes, solo que ahora vive en su propio archivo por
// categoria.
#include <strings.h> // strcasecmp(), usado para ordenar CONOCIDAS por nombre
#include "Globals.h"

void dibujarListaRedes() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("ESCANEAR", COLOR_CAT_CYAN, ICONO_ESCANEAR);

    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    // Mientras hay un escaneo en curso no tiene sentido mostrar la
    // lista vieja por debajo del cartel de "Escaneando...".
    if (escaneando) {
        tft.setTextColor(COLOR_WARN, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Escaneando...");
        dibujarPieAyuda("Esperando resultados...");
        return;
    }

    // Conteo de clientes de una red puntual en curso (no bloqueante,
    // ver WiFiScanner::update()).
    if (scanner.conteoEnProgreso()) {
        tft.setTextColor(COLOR_WARN, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Escuchando clientes de");
        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(10, top + 30);
        if (redIndiceConteo >= 0 && redIndiceConteo < numRedes) {
            tft.print(redes[redIndiceConteo].ssid);
        }
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 50);
        tft.print((scanner.tiempoRestanteConteo() / 1000) + 1);
        tft.print("s restantes");
        dibujarPieAyuda("Pasivo: solo escucha, no se conecta");
        return;
    }

    // Resultado del ultimo conteo de clientes: se queda mostrado
    // hasta que se toque de nuevo (vuelve a la lista) o se pida un
    // escaneo nuevo.
    if (redIndiceConteo >= 0 && redIndiceConteo < numRedes) {
        tft.setTextColor(COLOR_CAT_CYAN, COLOR_BG);
        tft.setCursor(5, top - 6);
        tft.print("Clientes de ");
        tft.print(redes[redIndiceConteo].ssid);

        int total = 0;
        ClienteInfo* lista = scanner.getClientes(total);

        if (total == 0) {
            tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
            tft.setCursor(10, top + 16);
            tft.print("Nadie hablando con esta red ahora");
        } else {
            int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - 36 - ALTO_PIE_AYUDA) / 18;
            int yOffset = top + 14;
            int mostradas = (total < maxVisibles) ? total : maxVisibles;
            for (int i = 0; i < mostradas; i++) {
                ClienteInfo& c = lista[i];
                tft.setTextColor(COLOR_TEXT, COLOR_BG);
                tft.setCursor(5, yOffset);
                tft.print(c.fabricante);

                tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
                tft.setCursor(TFT_PANEL_WIDTH - 60, yOffset);
                tft.print(c.rssi);
                tft.print("dBm");
                yOffset += 18;
            }
            if (total > mostradas) {
                tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
                tft.setCursor(5, yOffset);
                tft.print("+ ");
                tft.print(total - mostradas);
                tft.print(" mas");
            }
        }

        dibujarPieAyuda("Toca para volver a la lista");
        return;
    }

    int yOffset = top + 6;
    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / 20;

    for (int i = 0; i < numRedes && i < maxVisibles; i++) {
        uint16_t colorFila = redes[i].tieneClave ? COLOR_CAT_CYAN : COLOR_WARN;
        tft.drawBitmap(5, yOffset - 1, ICONO_ESCANEAR, ICONO_ANCHO, ICONO_ALTO, colorFila, COLOR_BG);
        int xTexto = 5 + ICONO_ANCHO + 4;

        tft.setTextColor(colorFila, COLOR_BG);
        tft.setCursor(xTexto, yOffset);
        tft.print(strlen(redes[i].ssid) == 0 ? "*OCULTA*" : redes[i].ssid);

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(TFT_PANEL_WIDTH - 70, yOffset);
        tft.print(redes[i].rssi);
        tft.print("dBm");
        yOffset += 20;
    }

    if (numRedes == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Toca aca para escanear");
        dibujarPieAyuda("Pasivo: no se conecta a ninguna red");
    } else {
        dibujarPieAyuda("Toca una red para ver sus clientes");
    }
}

void dibujarPantallaStats() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("STATS", COLOR_CAT_YELLOW, ICONO_STATS);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 35);
    tft.print("Redes conocidas: ");
    tft.print(aprendizaje.getTotalRedesConocidas());

    tft.setCursor(10, CONTENT_TOP + 55);
    tft.print("Vistas este escaneo: ");
    tft.print(numRedes);

    tft.setCursor(10, CONTENT_TOP + 75);
    tft.print("Nuevas descubiertas: ");
    tft.print(mascota.getRedesNuevas());

    if (numRedes > 0) {
        RedStats* s = aprendizaje.getStats(redes[0].bssid);
        if (s) {
            tft.setCursor(10, CONTENT_TOP + 105);
            tft.setTextColor(COLOR_WARN, COLOR_BG);
            tft.print("Ultima red top: ");
            tft.print(s->ssid);
            tft.setCursor(10, CONTENT_TOP + 125);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.print("Vista ");
            tft.print(s->vecesVista);
            tft.print(" veces");
        }
    }

    dibujarPieAyuda("Se actualiza con cada escaneo");
}

const char* nombreOrdenConocidas(int orden) {
    switch (orden) {
        case 1: return "VISTAS";
        case 2: return "NOMBRE";
        default: return "SENIAL";
    }
}

void dibujarPantallaConocidas() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CONOCIDAS", COLOR_CAT_WHITE, ICONO_CONOCIDAS);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    // Renglon de "Orden: X" tocable, mismo criterio que el boton de
    // exportar de EVENTOS (ALTO_BOTON_EXPORTAR): un bloque fijo
    // arriba de la lista, con su propio alto reservado, para que el
    // toque en manejarToqueWifi() (caso SCREEN_CONOCIDAS) siempre
    // coincida con lo que se ve dibujado aca.
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(8, top + 4);
    tft.print("Orden: ");
    tft.setTextColor(COLOR_CAT_WHITE, COLOR_BG);
    tft.print(nombreOrdenConocidas(ordenConocidas));
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print("  (tocar para cambiar)");
    dibujarLineaPunteada(5, top + ALTO_BOTON_ORDEN, TFT_PANEL_WIDTH - 10, COLOR_ACCENT_DIM);
    top += ALTO_BOTON_ORDEN;

    int total = aprendizaje.getTotalRedesConocidas();
    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Todavia no hay redes guardadas");
        dibujarPieAyuda("Escanea para empezar a guardar redes");
        return;
    }

    // NO hay orden por "ultima vez vista" - ver el comentario largo
    // junto a la declaracion de ordenConocidas, en main.cpp: ese dato
    // usa millis() de la sesion en que se vio, y comparar eso entre
    // sesiones distintas (reinicio de por medio) no dice nada real.
    RedStats* todas = aprendizaje.getTodas();
    int indices[MAX_REDES_ESCANEO];
    for (int i = 0; i < total; i++) indices[i] = i;
    for (int i = 0; i < total - 1; i++) {
        for (int j = 0; j < total - 1 - i; j++) {
            RedStats& a = todas[indices[j]];
            RedStats& b = todas[indices[j + 1]];
            bool debeSwap;
            switch (ordenConocidas) {
                case 1:
                    debeSwap = a.vecesVista < b.vecesVista;
                    break;
                case 2: {
                    // Las ocultas (sin SSID) siempre van al final,
                    // alfabetico no significa nada para un nombre
                    // vacio.
                    bool aOculta = strlen(a.ssid) == 0;
                    bool bOculta = strlen(b.ssid) == 0;
                    if (aOculta != bOculta) {
                        debeSwap = aOculta;
                    } else {
                        debeSwap = strcasecmp(a.ssid, b.ssid) > 0;
                    }
                    break;
                }
                default:
                    debeSwap = a.mejorRSSI < b.mejorRSSI;
                    break;
            }
            if (debeSwap) {
                int tmp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = tmp;
            }
        }
    }

    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_BOTON_ORDEN - ALTO_PIE_AYUDA) / 20;
    int yOffset = top + 6;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        RedStats& r = todas[indices[i]];

        tft.drawBitmap(5, yOffset - 1, ICONO_CONOCIDAS, ICONO_ANCHO, ICONO_ALTO, COLOR_CAT_WHITE, COLOR_BG);

        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(5 + ICONO_ANCHO + 4, yOffset);
        tft.print(strlen(r.ssid) > 0 ? r.ssid : "*OCULTA*");

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(TFT_PANEL_WIDTH - 95, yOffset);
        tft.print(r.mejorRSSI);
        tft.print("dBm x");
        tft.print(r.vecesVista);

        yOffset += 20;
    }

    if (total > mostradas) {
        char masTexto[32];
        snprintf(masTexto, sizeof(masTexto), "+ %d mas, por %s", total - mostradas, nombreOrdenConocidas(ordenConocidas));
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("Toca 'Orden' arriba para cambiar el criterio");
    }
}

void dibujarPantallaEventos() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("EVENTOS", COLOR_CAT_BLUE, ICONO_EVENTOS);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    // Boton fijo arriba de la lista para exportar /resumen.txt con
    // TODA la sesion (no solo los eventos - ver exportarResumenSesion()
    // mas abajo) a la SD. manejarToqueWifi(), caso SCREEN_EVENTOS,
    // usa estos mismos numeros para que el toque coincida con lo
    // dibujado.
    tft.setTextColor(COLOR_WARN, COLOR_BG);
    tft.setCursor(10, top + 2);
    tft.print("[Toca para exportar resumen .txt]");

    if (ultimoExportResumen < 0 && ultimoExportResumen != -2) {
        tft.setTextColor(COLOR_DANGER, COLOR_BG);
        tft.setCursor(10, top + 14);
        tft.print("Error: la SD no respondio");
    } else if (ultimoExportResumen >= 0) {
        tft.setTextColor(COLOR_CAT_BLUE, COLOR_BG);
        tft.setCursor(10, top + 14);
        tft.print("resumen.txt: ");
        tft.print(ultimoExportResumen);
        tft.print(" lineas");
    }

    dibujarLineaPunteada(10, top + ALTO_BOTON_EXPORTAR, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);
    int top2 = top + ALTO_BOTON_EXPORTAR + 4;

    int total = 0;
    EventoLog* lista = eventos.getEventos(total);

    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top2 + 10);
        tft.print("Todavia no paso nada en esta sesion");
        dibujarPieAyuda("El historial empieza al encender");
        return;
    }

    // El mas reciente ya queda primero en el array (ver EventLog::
    // registrar), asi que aca alcanza con recorrerlo en orden.
    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_BOTON_EXPORTAR - ALTO_PIE_AYUDA) / 34;
    int yOffset = top2 + 2;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        EventoLog& e = lista[i];

        tft.drawBitmap(5, yOffset + 2, ICONO_EVENTOS, ICONO_ANCHO, ICONO_ALTO, COLOR_CAT_BLUE, COLOR_BG);
        int xTexto = 5 + ICONO_ANCHO + 4;

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(xTexto, yOffset);
        tft.print(e.timestamp);

        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(xTexto, yOffset + 16);
        tft.print(e.texto);

        yOffset += 34;
    }

    if (total > mostradas) {
        char masTexto[32];
        snprintf(masTexto, sizeof(masTexto), "+ %d mas viejos", total - mostradas);
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("El mas reciente arriba de todo");
    }
}

void dibujarPantallaMonitor() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("MONITOR", COLOR_CAT_RED, ICONO_MONITOR);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    unsigned long totalDeauth = 0, totalDisassoc = 0;
    scanner.getAnomaliasStats(totalDeauth, totalDisassoc);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(5, top);
    tft.print("Deauth: ");
    tft.setTextColor(totalDeauth > 0 ? COLOR_DANGER : COLOR_TEXT, COLOR_BG);
    tft.print(totalDeauth);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(TFT_PANEL_WIDTH / 2 + 5, top);
    tft.print("Disassoc: ");
    tft.setTextColor(totalDisassoc > 0 ? COLOR_WARN : COLOR_TEXT, COLOR_BG);
    tft.print(totalDisassoc);

    dibujarLineaPunteada(5, top + 18, TFT_PANEL_WIDTH - 10, COLOR_ACCENT_DIM);

    // Captura cruda a .pcap. Tocar esta franja prende o apaga la
    // grabacion - manejarToqueWifi(), caso SCREEN_MONITOR, usa el
    // mismo ALTO_BLOQUE_MONITOR para que el toque coincida con lo
    // dibujado aca.
    bool pcapOn = scanner.pcapEstaActivo();
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(5, top + 22);
    tft.print("PCAP: ");
    tft.setTextColor(pcapOn ? COLOR_DANGER : COLOR_TEXT_DIM, COLOR_BG);
    tft.print(pcapOn ? "grabando (toca p/parar)" : "apagado (toca p/grabar)");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(5, top + 34);
    tft.print("Paquetes guardados: ");
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.print(scanner.getPCAPCapturados());

    dibujarLineaPunteada(5, top + ALTO_BLOQUE_MONITOR, TFT_PANEL_WIDTH - 10, COLOR_ACCENT_DIM);

    int total = 0;
    AnomaliaWiFi* lista = scanner.getAnomalias(total);

    if (total == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + ALTO_BLOQUE_MONITOR + 10);
        tft.print("Nada detectado por ahora");
        dibujarPieAyuda("Pasivo: solo mira, no transmite nada");
        return;
    }

    // El canal del radio en este momento no se controla desde aca, asi
    // que esta lista solo ve lo que pasa en ESE canal - no es un
    // monitor de toda la banda 2.4GHz a la vez.
    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_BLOQUE_MONITOR - ALTO_PIE_AYUDA) / 34;
    int yOffset = top + ALTO_BLOQUE_MONITOR + 4;
    int mostradas = (total < maxVisibles) ? total : maxVisibles;

    for (int i = 0; i < mostradas; i++) {
        AnomaliaWiFi& a = lista[i];
        unsigned long segundos = a.momento / 1000;
        char horaBuf[16];
        sprintf(horaBuf, "T+%02lu:%02lu:%02lu", segundos / 3600, (segundos % 3600) / 60, segundos % 60);

        uint16_t colorFila = a.esDisassoc ? COLOR_WARN : COLOR_DANGER;
        tft.drawBitmap(5, yOffset + 2, ICONO_MONITOR, ICONO_ANCHO, ICONO_ALTO, colorFila, COLOR_BG);
        int xTexto = 5 + ICONO_ANCHO + 4;

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(xTexto, yOffset);
        tft.print(horaBuf);
        tft.print("  ch");
        tft.print(a.canal);

        tft.setTextColor(colorFila, COLOR_BG);
        tft.setCursor(xTexto, yOffset + 16);
        tft.print(a.esDisassoc ? "DISASSOC" : "DEAUTH");
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.print("  de ..");
        char macBuf[6];
        sprintf(macBuf, "%02X%02X", a.origen[4], a.origen[5]);
        tft.print(macBuf);

        yOffset += 34;
    }

    if (total > mostradas) {
        char masTexto[32];
        snprintf(masTexto, sizeof(masTexto), "+ %d mas", total - mostradas);
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("Pasivo: solo mira, no transmite nada");
    }
}

// Cuenta, canal por canal (1 a 13), cuantas redes del ULTIMO escaneo
// hay en cada uno, y lo muestra como un grafico de barras
// horizontales.
void dibujarPantallaCanales() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("CANALES", COLOR_CAT_YELLOW, ICONO_CANALES);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    if (numRedes == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Hace falta un escaneo primero");
        dibujarPieAyuda("Anda a ESCANEAR y volve aca");
        return;
    }

    int conteo[13] = {0};
    int maxConteo = 1; // nunca 0, para no dividir por 0 mas abajo
    for (int i = 0; i < numRedes; i++) {
        int ch = redes[i].canal;
        if (ch >= 1 && ch <= 13) {
            conteo[ch - 1]++;
            if (conteo[ch - 1] > maxConteo) maxConteo = conteo[ch - 1];
        }
    }

    const int FILAS = 13;
    int altoFila = (alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / FILAS;
    if (altoFila < 10) altoFila = 10; // las 13 filas entran apretadas igual en pantallas chicas
    int xBarra = 38;
    int anchoMaxBarra = TFT_PANEL_WIDTH - xBarra - 35; // deja lugar al numero de la derecha
    // Barra tipo medidor segmentado (estilo huntMeter de Fennec) en
    // vez de una barra de progreso lisa - mismo proposito, se nota
    // igual de bien cual canal esta mas saturado.
    const int SEGMENTOS_BARRA = 16;

    int y = top + 2;
    for (int ch = 1; ch <= 13; ch++) {
        int cant = conteo[ch - 1];
        int llenos = (cant * SEGMENTOS_BARRA) / maxConteo;
        if (cant > 0 && llenos < 1) llenos = 1; // que se note aunque sea una sola red

        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(2, y);
        tft.print("CH");
        tft.print(ch);

        dibujarBarraSegmentada(xBarra, y, anchoMaxBarra, 8, SEGMENTOS_BARRA, llenos, COLOR_ACCENT, COLOR_PANEL);

        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(xBarra + anchoMaxBarra + 4, y);
        tft.print(cant);

        y += altoFila;
    }

    // Estas barras son la foto del ULTIMO escaneo (manual o
    // automatico, ver AUTO_SCAN_MS en main.cpp). El pie de abajo en
    // cambio es el ACUMULADO de todos los escaneos de la sesion.
    if (escaneosAcumulados > 1) {
        int chMasUsado = 1;
        for (int ch = 2; ch <= 13; ch++) {
            if (conteoAcumuladoCanales[ch - 1] > conteoAcumuladoCanales[chMasUsado - 1]) {
                chMasUsado = ch;
            }
        }
        char pie[40];
        snprintf(pie, sizeof(pie), "Historico (%d escaneos): CH%d el mas usado", escaneosAcumulados, chMasUsado);
        dibujarPieAyuda(pie);
    } else {
        dibujarPieAyuda("Barra mas corta = canal menos saturado");
    }
}

// Alto de la franja de controles (Channel/Y/X +/-) de ANALIZADOR, de
// abajo del grafico - dibujarPantallaAnalizador() y manejarToqueWifi()
// (caso SCREEN_ANALIZADOR, mas abajo) comparten este mismo numero para
// que el toque siempre coincida con lo que se ve dibujado, mismo
// criterio que ALTO_BOTON_ORDEN/ALTO_BOTON_EXPORTAR.
const int ALTO_CONTROLES_ANALIZADOR = 34;

// Analizador de canales: calca la pantalla REAL de Marauder (foto que
// mando el usuario) en vez del grafico de barras por canal de la
// primera version de esto - un grafico de LINEA en el TIEMPO de un
// solo canal elegido a mano, con 3 series separadas por color
// (Beacons/Deauths/Probes, igual que el legend de Marauder) y
// controles de Channel, escala Y y escala X. Todo dibujado con el
// lenguaje visual ambar del resto del firmware (marco de solo
// esquinas, texto con letras separadas, controles tipo "caja con
// esquinas" en vez de botones rellenos). Pura recepcion: el radio
// nunca transmite, y los controles de esta pantalla solo cambian A
// QUE escucha (el canal) o COMO se dibuja (las escalas) - ver el
// comentario largo de ANALIZADOR_DWELL_MS en WiFiScanner.h. Arranca
// solo al entrar a esta pantalla y se apaga solo al salir - ver el
// bloque de cambioPantalla en loop(), main.cpp.
void dibujarPantallaAnalizador() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("ANALIZADOR", COLOR_CAT_YELLOW, ICONO_CANALES);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    uint16_t histBeacons[ANALIZADOR_MUESTRAS];
    uint16_t histDeauths[ANALIZADOR_MUESTRAS];
    uint16_t histProbes[ANALIZADOR_MUESTRAS];
    scanner.getHistorialAnalizador(histBeacons, histDeauths, histProbes, ANALIZADOR_MUESTRAS);

    int canal = scanner.getCanalActualAnalizador();

    // --- Leyenda: una chapita de color + nombre + ultima muestra de
    // cada serie, igual que el recuadro "Beacons/Deauths/Probes" de la
    // referencia. Deauths en COLOR_DANGER a proposito (no en la
    // paleta ambar): es la misma excepcion que ya usa Monitor para
    // marcar esa señal como la que de verdad importa vigilar, y aca
    // sigue siendo justo eso.
    int yLeyenda = top + 2;
    int xLeyenda = 10;
    // Nombres abreviados a proposito (BCN/DEA/PRB en vez de BEACONS/
    // DEAUTHS/PROBES): con 3 series + numero en la misma franja, el
    // nombre completo no entra en los 240px de las placas chicas sin
    // que la leyenda se corte - el color ya identifica cual es cual
    // igual que en la referencia, el texto es solo un recordatorio.
    struct { const char* nombre; uint16_t color; uint16_t ultimo; } series[3] = {
        {"BCN", COLOR_LINE,   histBeacons[ANALIZADOR_MUESTRAS - 1]},
        {"DEA", COLOR_DANGER, histDeauths[ANALIZADOR_MUESTRAS - 1]},
        {"PRB", COLOR_ACCENT, histProbes[ANALIZADOR_MUESTRAS - 1]},
    };
    for (int i = 0; i < 3; i++) {
        char etiqueta[12];
        snprintf(etiqueta, sizeof(etiqueta), "%s %u", series[i].nombre, series[i].ultimo);
        tft.fillRect(xLeyenda, yLeyenda + 2, 6, 6, series[i].color);
        dibujarTextoEspaciado(xLeyenda + 10, yLeyenda, etiqueta, series[i].color, COLOR_BG, 1);
        xLeyenda += 10 + anchoTextoEspaciado(etiqueta, 1) + 10;
    }

    // --- Marco del grafico: solo esquinas, estilo Fennec, igual que
    // el resto de las cajas de esta interfaz desde el restyle.
    int yMarco = yLeyenda + 14;
    int altoDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int altoMarco = altoDisponible - (yMarco - top) - ALTO_CONTROLES_ANALIZADOR - 4;
    if (altoMarco < 30) altoMarco = 30; // por si alguna placa queda con muy poco alto disponible
    int xMarco = 8;
    int anchoMarco = TFT_PANEL_WIDTH - xMarco * 2;
    dibujarEsquinas(xMarco, yMarco, anchoMarco, altoMarco, COLOR_ACCENT_DIM, 5);

    // --- Las 3 series, como lineas quebradas conectando cada muestra
    // del historial (en vez de un area rellena: con 3 series pisandose
    // encima, lineas finas se leen mejor que 3 manchas solidas
    // superpuestas). "escalaX" controla CUANTAS muestras recientes se
    // ven (mas escala = menos muestras = mas "zoom" en el tiempo
    // reciente); "escalaY" multiplica el alto en pixeles de cada
    // unidad de conteo (mas escala = picos chicos se ven mas grandes).
    int visibles = ANALIZADOR_MUESTRAS / analizadorEscalaX;
    if (visibles < 2) visibles = 2;
    if (visibles > ANALIZADOR_MUESTRAS) visibles = ANALIZADOR_MUESTRAS;
    int primerIndice = ANALIZADOR_MUESTRAS - visibles;

    int xGrafico = xMarco + 3;
    int anchoGrafico = anchoMarco - 6;
    int yBase = yMarco + altoMarco - 3;
    int altoUtil = altoMarco - 6;
    float pasoX = (visibles > 1) ? (float)anchoGrafico / (visibles - 1) : 0;

    uint16_t* hist[3] = {histBeacons, histDeauths, histProbes};
    for (int s = 0; s < 3; s++) {
        int xPrev = 0, yPrev = 0;
        for (int i = 0; i < visibles; i++) {
            uint16_t valor = hist[s][primerIndice + i];
            int alto = valor * analizadorEscalaY;
            if (alto > altoUtil) alto = altoUtil; // recorta picos que no entran, no deforma la escala del resto
            int x = xGrafico + (int)(i * pasoX);
            int y = yBase - alto;
            if (i > 0) {
                tft.drawLine(xPrev, yPrev, x, y, series[s].color);
            }
            xPrev = x;
            yPrev = y;
        }
    }

    // --- Controles: 3 grupos [-][valor][+] lado a lado (Channel / Y /
    // X), cajas de solo esquinas en vez de botones rellenos. Mismos
    // numeros que usa manejarToqueWifi() (caso SCREEN_ANALIZADOR) para
    // que el toque coincida siempre con esto.
    int yControles = yMarco + altoMarco + 4;
    int anchoGrupo = TFT_PANEL_WIDTH / 3;
    int anchoBtn = 22;
    int altoBtn = ALTO_CONTROLES_ANALIZADOR - 10;

    char valorCanal[4];
    snprintf(valorCanal, sizeof(valorCanal), "%d", canal);
    char valorY[6];
    snprintf(valorY, sizeof(valorY), "Yx%d", analizadorEscalaY);
    char valorX[6];
    snprintf(valorX, sizeof(valorX), "Xx%d", analizadorEscalaX);
    const char* etiquetas[3] = {"CH", valorY, valorX};
    const char* valores[3] = {valorCanal, "", ""};

    for (int g = 0; g < 3; g++) {
        int xGrupo = g * anchoGrupo;
        int xMenos = xGrupo + 4;
        int xMas = xGrupo + anchoGrupo - anchoBtn - 4;
        int yBtn = yControles + 5;

        dibujarEsquinas(xMenos, yBtn, anchoBtn, altoBtn, COLOR_ACCENT_DIM, 3);
        dibujarTextoEspaciado(xMenos + 7, yBtn + (altoBtn / 2) - 3, "-", COLOR_TEXT, COLOR_BG, 0);

        dibujarEsquinas(xMas, yBtn, anchoBtn, altoBtn, COLOR_ACCENT_DIM, 3);
        dibujarTextoEspaciado(xMas + 7, yBtn + (altoBtn / 2) - 3, "+", COLOR_TEXT, COLOR_BG, 0);

        // Etiqueta/valor centrado entre los dos botones del grupo.
        char medio[10];
        if (g == 0) {
            snprintf(medio, sizeof(medio), "%s%s", etiquetas[g], valores[g]);
        } else {
            snprintf(medio, sizeof(medio), "%s", etiquetas[g]);
        }
        int anchoMedio = anchoTextoEspaciado(medio, 1);
        int xMedio = xMenos + anchoBtn + ((xMas - (xMenos + anchoBtn)) - anchoMedio) / 2;
        dibujarTextoEspaciado(xMedio, yBtn + (altoBtn / 2) - 3, medio, COLOR_TEXT_DIM, COLOR_BG, 1);
    }

    dibujarPieAyuda("Toca - o + para ajustar Channel / escala Y / X");
}

// Compara las redes del ULTIMO escaneo entre si y marca que SSID
// aparece con mas de un BSSID distinto EN EL MISMO escaneo - la señal
// pasiva clasica de un posible "gemelo malvado". No es una certeza
// (una red mesh real da el mismo patron).
void dibujarPantallaDuplicados() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("DUPLICADO", COLOR_CAT_RED, ICONO_DUPLICADO);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    if (numRedes == 0) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Hace falta un escaneo primero");
        dibujarPieAyuda("Anda a ESCANEAR y volve aca");
        return;
    }

    // Las redes ocultas (SSID vacio) se ignoran a proposito: entre
    // ellas serian "duplicados" sin decir nada real.
    bool sospechosa[MAX_REDES_ESCANEO] = {false};
    int totalSospechosas = 0;
    for (int i = 0; i < numRedes; i++) {
        if (strlen(redes[i].ssid) == 0) continue;
        for (int j = i + 1; j < numRedes; j++) {
            if (strlen(redes[j].ssid) == 0) continue;
            if (strcmp(redes[i].ssid, redes[j].ssid) == 0 &&
                memcmp(redes[i].bssid, redes[j].bssid, 6) != 0) {
                if (!sospechosa[i]) { sospechosa[i] = true; totalSospechosas++; }
                if (!sospechosa[j]) { sospechosa[j] = true; totalSospechosas++; }
            }
        }
    }

    if (totalSospechosas == 0) {
        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setCursor(10, top + 10);
        tft.print("Ningun SSID repetido en este escaneo");
        dibujarPieAyuda("Puede cambiar en el proximo escaneo");
        return;
    }

    int maxVisibles = (alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / 34;
    int yOffset = top + 2;
    int mostradas = 0;

    for (int i = 0; i < numRedes && mostradas < maxVisibles; i++) {
        if (!sospechosa[i]) continue;

        tft.drawBitmap(5, yOffset + 2, ICONO_DUPLICADO, ICONO_ANCHO, ICONO_ALTO, COLOR_DANGER, COLOR_BG);
        int xTexto = 5 + ICONO_ANCHO + 4;

        tft.setTextColor(COLOR_DANGER, COLOR_BG);
        tft.setCursor(xTexto, yOffset);
        tft.print(redes[i].ssid);

        char macBuf[20];
        sprintf(macBuf, "..%02X%02X  ch%d", redes[i].bssid[4], redes[i].bssid[5], redes[i].canal);
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.setCursor(xTexto, yOffset + 16);
        tft.print(macBuf);

        yOffset += 34;
        mostradas++;
    }

    if (totalSospechosas > mostradas) {
        char masTexto[36];
        snprintf(masTexto, sizeof(masTexto), "+ %d mas con el mismo problema", totalSospechosas - mostradas);
        dibujarPieAyuda(masTexto);
    } else {
        dibujarPieAyuda("Puede ser mesh real - no es 100% certeza");
    }
}

// GAME: vacia a proposito, pedido explicito del usuario (ver el
// comentario junto a la fila "GAME" en herramientas[], en main.cpp) -
// nada de logica aca adentro, solo el cartel. Se completa en otra
// sesion.
void dibujarPantallaGame() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("GAME", COLOR_CAT_WHITE, ICONO_ESPADA);
    int top = CONTENT_TOP + ALTO_TITULO_SECCION;

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, top + 10);
    tft.print("Proximamente...");
    dibujarPieAyuda("Todavia no hay nada aca");
}

// Vuelca a /resumen.txt en la SD un reporte legible de TODA la sesion.
// Mismo patron de SD.end()/SD.begin() que ya usa Wardriving::
// exportKML(): abre, escribe y cierra en esta misma llamada, nunca
// deja un File colgado para la proxima. Devuelve cuantas lineas
// escribio, o -1 si la SD no estaba disponible.
int exportarResumenSesion() {
    SD.end();
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("No se pudo montar la SD para el resumen");
        return -1;
    }

    File f = SD.open("/resumen.txt", FILE_WRITE);
    if (!f) {
        Serial.println("No se pudo crear resumen.txt");
        return -1;
    }

    int lineas = 0;
    unsigned long seg = millis() / 1000;
    f.println("=== Resumen de sesion - deauth-chan ===");
    f.printf("Tiempo encendido: %02lu:%02lu:%02lu\n", seg / 3600, (seg % 3600) / 60, seg % 60);
    lineas += 2;

    f.println();
    f.println("-- WiFi --");
    f.printf("Redes vistas en el ultimo escaneo: %d\n", numRedes);
    f.printf("Redes distintas aprendidas (esta sesion y anteriores): %d\n", aprendizaje.getTotalRedesConocidas());
    f.printf("Redes nuevas descubiertas en esta sesion: %d\n", mascota.getRedesNuevas());
    lineas += 4;

    unsigned long deauth = 0, disassoc = 0;
    scanner.getAnomaliasStats(deauth, disassoc);
    f.println();
    f.println("-- Monitor pasivo --");
    f.printf("Paquetes deauth detectados: %lu\n", deauth);
    f.printf("Paquetes disassoc detectados: %lu\n", disassoc);
    lineas += 3;

    f.println();
    f.println("-- Wardriving --");
    f.printf("Redes anotadas en wardriving.csv: %d\n", wardriver.getContadorRedes());
    lineas += 2;

    f.println();
    f.println("-- Historial de eventos de esta sesion --");
    int totalEventos = 0;
    EventoLog* listaEventos = eventos.getEventos(totalEventos);
    if (totalEventos == 0) {
        f.println("(sin eventos)");
        lineas++;
    } else {
        // El mas reciente queda primero en el array (ver EventLog::
        // registrar) - lo volcamos en ese mismo orden.
        for (int i = 0; i < totalEventos; i++) {
            f.printf("%s  %s\n", listaEventos[i].timestamp, listaEventos[i].texto);
            lineas++;
        }
    }

    f.close();
    Serial.printf("resumen.txt generado (%d lineas)\n", lineas);
    return lineas;
}

// Toque sobre cualquiera de las pantallas de WIFI que tienen algo
// tocable (SCAN, CONOCIDAS, EVENTOS, MONITOR) - mismo cuerpo que
// tenian esos cases del switch de manejarToque() en main.cpp, movidos
// tal cual a su propia funcion por categoria.
void manejarToqueWifi(MenuScreen pantalla, int x, int y) {
    switch (pantalla) {
        case SCREEN_SCAN: {
            // Mientras hay un escaneo o un conteo de clientes en
            // curso, un toque no hace nada.
            if (escaneando || scanner.conteoEnProgreso()) break;

            if (redIndiceConteo >= 0) {
                // Se esta mostrando el resultado de un conteo: un
                // toque en cualquier lado vuelve a la lista de redes.
                redIndiceConteo = -1;
                contenidoSucio = true;
                break;
            }

            // Mismos numeros que usa dibujarListaRedes() para pintar
            // la lista (top + 6, filas de 20px) - si alguno de los dos
            // cambia sin el otro, el toque deja de coincidir con lo
            // que se ve en pantalla.
            int primeraFilaY = CONTENT_TOP + ALTO_TITULO_SECCION + 6;
            int filaAlto = 20;
            int indiceFila = (y - primeraFilaY) / filaAlto;
            bool filaValida = (y >= primeraFilaY) && indiceFila >= 0 &&
                indiceFila < numRedes &&
                indiceFila < ((alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA) / filaAlto);

            if (filaValida) {
                // Toco una red puntual: arranca el conteo pasivo de
                // clientes para esa red (no bloqueante).
                scanner.iniciarConteoClientes(redes[indiceFila].bssid, redes[indiceFila].canal);
                redIndiceConteo = indiceFila;
                contenidoSucio = true;
            } else if (y > CONTENT_TOP && y < CONTENT_TOP + alturaListado()) {
                // Toco el espacio vacio debajo de la lista: pide un
                // escaneo nuevo.
                mascota.setEstado(ESTADO_SCANNING);
                scanner.startScan();
                escaneando = true;
                contenidoSucio = true; // para que se vea "Escaneando..." ya mismo
            }
            break;
        }
        case SCREEN_CONOCIDAS: {
            // Mismos numeros que dibujarPantallaConocidas() para su
            // renglon fijo de "Orden: X" - ver ALTO_BOTON_ORDEN en
            // main.cpp. Tocar en cualquier lado de esa franja pasa al
            // siguiente criterio (ciclico: senial -> vistas -> nombre
            // -> de nuevo senial).
            int top = CONTENT_TOP + ALTO_TITULO_SECCION;
            if (y >= top && y <= top + ALTO_BOTON_ORDEN) {
                ordenConocidas = (ordenConocidas + 1) % 3;
                contenidoSucio = true;
            }
            break;
        }
        case SCREEN_EVENTOS: {
            // Mismos numeros que dibujarPantallaEventos() para su
            // boton fijo de "exportar resumen" - ver el comentario de
            // ALTO_BOTON_EXPORTAR en Menu.cpp.
            int top = CONTENT_TOP + ALTO_TITULO_SECCION;
            if (y >= top && y <= top + ALTO_BOTON_EXPORTAR) {
                ultimoExportResumen = exportarResumenSesion();
                #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
                    // exportarResumenSesion() remonta la SD, igual que
                    // exportKML() de Wardrive - mismo motivo para
                    // reclamar el bus aca.
                    touchDriver.reclaimBus();
                #endif
                contenidoSucio = true; // para mostrar el resultado ya mismo
            }
            break;
        }
        case SCREEN_MONITOR: {
            // Mismos numeros que dibujarPantallaMonitor() para su
            // franja de PCAP - ver el comentario de ALTO_BLOQUE_MONITOR
            // en Menu.cpp.
            int top = CONTENT_TOP + ALTO_TITULO_SECCION;
            if (y >= top + 18 && y <= top + ALTO_BLOQUE_MONITOR) {
                bool nuevoEstado = !scanner.pcapEstaActivo();
                scanner.activarPCAP(nuevoEstado);
                eventos.registrar(nuevoEstado ? "PCAP: grabacion iniciada" : "PCAP: grabacion detenida");
                #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
                    // registrar() escribe a la SD - mismo motivo de
                    // siempre para reclamar el bus del touch despues.
                    touchDriver.reclaimBus();
                #endif
                contenidoSucio = true;
            }
            break;
        }
        case SCREEN_ANALIZADOR: {
            // Mismos numeros que dibujarPantallaAnalizador() usa para
            // ubicar el marco del grafico y la franja de controles -
            // si alguno de los dos cambia sin el otro, el toque deja
            // de coincidir con lo que se ve dibujado.
            int top = CONTENT_TOP + ALTO_TITULO_SECCION;
            int yLeyenda = top + 2;
            int yMarco = yLeyenda + 14;
            int altoDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
            int altoMarco = altoDisponible - (yMarco - top) - ALTO_CONTROLES_ANALIZADOR - 4;
            if (altoMarco < 30) altoMarco = 30;
            int yControles = yMarco + altoMarco + 4;

            int anchoBtn = 22;
            int altoBtn = ALTO_CONTROLES_ANALIZADOR - 10;
            int yBtn = yControles + 5;
            if (y < yBtn || y > yBtn + altoBtn) break;

            int anchoGrupo = TFT_PANEL_WIDTH / 3;
            int grupo = x / anchoGrupo;
            if (grupo > 2) grupo = 2;
            int xGrupo = grupo * anchoGrupo;
            int xMenos = xGrupo + 4;
            int xMas = xGrupo + anchoGrupo - anchoBtn - 4;

            bool tocoMenos = (x >= xMenos && x <= xMenos + anchoBtn);
            bool tocoMas = (x >= xMas && x <= xMas + anchoBtn);
            if (!tocoMenos && !tocoMas) break;

            if (grupo == 0) {
                // Channel: ciclico entre 1 y 13 (no tiene sentido
                // "pasarse" del ultimo canal y quedar en un numero
                // invalido, mejor dar la vuelta).
                int canal = scanner.getCanalActualAnalizador();
                canal += tocoMas ? 1 : -1;
                if (canal < 1) canal = 13;
                if (canal > 13) canal = 1;
                scanner.setCanalAnalizador(canal);
            } else if (grupo == 1) {
                analizadorEscalaY += tocoMas ? 1 : -1;
                if (analizadorEscalaY < 1) analizadorEscalaY = 1;
                if (analizadorEscalaY > 20) analizadorEscalaY = 20;
            } else {
                analizadorEscalaX += tocoMas ? 1 : -1;
                if (analizadorEscalaX < 1) analizadorEscalaX = 1;
                if (analizadorEscalaX > 4) analizadorEscalaX = 4;
            }
            contenidoSucio = true;
            break;
        }
        default:
            break;
    }
}
