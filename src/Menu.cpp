// Chrome compartido por todas las pantallas (titulo de seccion, pie de
// ayuda, barra de botones de abajo) + el menu de dos niveles
// (categorias -> herramientas dentro de esa categoria). Nada de esto
// es especifico de una categoria en particular - por eso quedo en su
// propio archivo en vez de ir adentro de ScreenWifi.cpp o cualquier
// otro Screen*.cpp. Movido tal cual desde main.cpp, sin cambiar
// ningun comportamiento.
#include "Globals.h"

// Alto que ocupa el titulo de seccion (dibujarTituloSeccion, definida
// mas abajo junto con alturaListado) incluyendo su linea separadora -
// las pantallas que lo usan arrancan su propio contenido despues de
// este offset.
const int ALTO_TITULO_SECCION = 30;

// Alto reservado abajo de CUALQUIER pantalla de Herramientas para el
// renglon de ayuda (texto chico tipo "TAP=SELEC | <=VOLVER") y la
// barra solida decorativa, calcado del layout de PORKCHOP/Marauder
// que mando el usuario de referencia.
const int ALTO_PIE_AYUDA = 34;

// Alto del boton fijo de "exportar resumen" arriba de la lista de
// Eventos (ver dibujarPantallaEventos() en ScreenWifi.cpp y
// manejarToqueWifi(), caso SCREEN_EVENTOS - comparten este mismo
// numero para que el toque coincida siempre con lo que se ve
// dibujado).
const int ALTO_BOTON_EXPORTAR = 22;

// Alto del bloque fijo de arriba de Monitor (contadores de deauth/
// disassoc + el toggle de captura PCAP), antes de que arranque la
// lista de anomalias - ver dibujarPantallaMonitor() y manejarToqueWifi(),
// caso SCREEN_MONITOR, ambas en ScreenWifi.cpp.
const int ALTO_BLOQUE_MONITOR = 54;

void calcularLayoutMenu() {
    int anchoBoton = TFT_PANEL_WIDTH / 2;
    int y = TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT;

    menuBotones[0] = {0, y, anchoBoton - 2, 34, "Mascota"};
    menuBotones[1] = {anchoBoton, y, anchoBoton - 2, 34, "Herramientas"};
}

// Layout del menu de categorias (SCREEN_TOOLS) - son siempre 4, fijas,
// asi que esto se calcula una sola vez en setup().
void calcularLayoutCategorias() {
    int espacioDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int gap = 6;
    int alto = (espacioDisponible - gap * (CANTIDAD_CATEGORIAS - 1)) / CANTIDAD_CATEGORIAS;
    if (alto > 48) alto = 48; // son pocas categorias, que se vean bien grandes
    if (alto < 24) alto = 24;

    int y = CONTENT_TOP + ALTO_TITULO_SECCION;
    for (int i = 0; i < CANTIDAD_CATEGORIAS; i++) {
        categorias[i].y = y;
        categorias[i].h = alto;
        y += alto + gap;
    }
}

// Arma, para SCREEN_TOOLS_SUB, la lista de herramientas que
// pertenecen a "categoria" (sus indices dentro de herramientas[], en
// "indicesOut") y calcula la posicion de cada una. Son pocos items
// como para que valga la pena cachear esto aparte; se llama tanto
// desde el dibujado como desde el toque, siempre con los mismos dos
// pasos, para que nunca queden desincronizados entre si.
int layoutHerramientasDeCategoria(int categoria, int* indicesOut) {
    int cantidad = 0;
    for (int i = 0; i < CANTIDAD_HERRAMIENTAS; i++) {
        if (herramientas[i].categoria == categoria) {
            indicesOut[cantidad++] = i;
        }
    }
    if (cantidad == 0) return 0; // no deberia pasar nunca, pero evita una division por 0

    int espacioDisponible = alturaListado() - ALTO_TITULO_SECCION - ALTO_PIE_AYUDA;
    int gap = 2;
    int alto = (espacioDisponible - gap * (cantidad - 1)) / cantidad;
    if (alto > 28) alto = 28;
    if (alto < 18) alto = 18;

    int y = CONTENT_TOP + ALTO_TITULO_SECCION;
    for (int k = 0; k < cantidad; k++) {
        int idx = indicesOut[k];
        herramientas[idx].y = y;
        herramientas[idx].h = alto;
        y += alto + gap;
    }
    return cantidad;
}

// Dice si "pantalla" es una herramienta propiamente dicha (ESCANEAR,
// STATS, WARDRIVE, RADAR, CONFIG, etc - cualquier pantalla final, no
// un nivel del menu). Se usa tanto para decidir el texto del boton
// derecho de abajo (dibujarMenuBotones(), aca mismo) como para que
// ese boton funcione como "Atras" en manejarToque() (main.cpp).
bool esPantallaHoja(MenuScreen pantalla) {
    return pantalla != SCREEN_MAIN && pantalla != SCREEN_TOOLS && pantalla != SCREEN_TOOLS_SUB;
}

// ---- Vocabulario visual Fennec (fase 3 del rediseño) --------------
//
// Calcado de cifertech/fennec (Fennec/DisplayUI.cpp): esa interfaz no
// usa bloques rellenos ni lineas solidas para separar secciones, usa
// texto con las letras separadas (drawSpaced/drawSpacedLeft), lineas
// punteadas (dashHLine) y cajas marcadas solo en las esquinas en vez
// de un rectangulo completo. Estos 5 helpers son la version de eso
// para este firmware - los usa TODO el chrome compartido de aca
// abajo, y los Screen*.cpp de cada categoria para sus propias barras
// y separadores.

int anchoTextoEspaciado(const char* texto, int espacio) {
    int n = strlen(texto);
    if (n == 0) return 0;
    // Fuente 1 de TFT_eSPI a tamaño 1: 6px de ancho por caracter.
    return n * 6 + (n - 1) * espacio;
}

void dibujarTextoEspaciado(int x, int y, const char* texto, uint16_t color, uint16_t fondo, int espacio) {
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(color, fondo);
    int cx = x;
    char uno[2] = {0, 0};
    for (int i = 0; texto[i] != '\0'; i++) {
        uno[0] = texto[i];
        tft.setCursor(cx, y);
        tft.print(uno);
        cx += 6 + espacio;
    }
}

void dibujarLineaPunteada(int x, int y, int w, uint16_t color) {
    // 2px pintados + 2px salteados, igual que dashHLine de Fennec.
    for (int px = x; px < x + w; px += 4) {
        int largoSegmento = 2;
        int restante = (x + w) - px;
        if (restante < largoSegmento) largoSegmento = restante;
        if (largoSegmento > 0) tft.drawFastHLine(px, y, largoSegmento, color);
    }
}

void dibujarEsquinas(int x, int y, int w, int h, uint16_t color, int largo) {
    tft.drawFastHLine(x, y, largo, color);
    tft.drawFastVLine(x, y, largo, color);
    tft.drawFastHLine(x + w - largo, y, largo, color);
    tft.drawFastVLine(x + w - 1, y, largo, color);
    tft.drawFastHLine(x, y + h - 1, largo, color);
    tft.drawFastVLine(x, y + h - largo, largo, color);
    tft.drawFastHLine(x + w - largo, y + h - 1, largo, color);
    tft.drawFastVLine(x + w - 1, y + h - largo, largo, color);
}

void dibujarBarraSegmentada(int x, int y, int w, int h, int segmentos, int llenos, uint16_t colorLleno, uint16_t colorVacio) {
    if (segmentos <= 0) return;
    int gap = 2;
    int anchoSeg = (w - gap * (segmentos - 1)) / segmentos;
    if (anchoSeg < 1) anchoSeg = 1;
    int cx = x;
    for (int i = 0; i < segmentos; i++) {
        uint16_t c = (i < llenos) ? colorLleno : colorVacio;
        tft.fillRect(cx, y, anchoSeg, h, c);
        cx += anchoSeg + gap;
    }
}

// Misma idea que dibujarBarraSegmentada() de arriba, pero apilando los
// segmentos de abajo hacia arriba en vez de de izquierda a derecha -
// hace falta para el Analizador de canales, que calca el grafico de
// barras VERTICALES de Marauder (una columna por canal) en vez de la
// barra horizontal que usa CANALES.
void dibujarBarraSegmentadaV(int x, int y, int w, int h, int segmentos, int llenos, uint16_t colorLleno, uint16_t colorVacio) {
    if (segmentos <= 0) return;
    int gap = 1;
    int altoSeg = (h - gap * (segmentos - 1)) / segmentos;
    if (altoSeg < 1) altoSeg = 1;
    int cy = y + h - altoSeg;
    for (int i = 0; i < segmentos; i++) {
        uint16_t c = (i < llenos) ? colorLleno : colorVacio;
        tft.fillRect(x, cy, w, altoSeg, c);
        cy -= (altoSeg + gap);
    }
}

void dibujarMenuBotones() {
    // Linea punteada arriba de la barra en vez de solida, para
    // separarla del contenido con el mismo trazo que el resto del
    // chrome nuevo.
    dibujarLineaPunteada(0, TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - 1, TFT_PANEL_WIDTH, COLOR_ACCENT_DIM);

    for (int i = 0; i < 2; i++) {
        // El boton de Mascota se marca activo solo en la pantalla
        // principal; el de Herramientas/Atras se marca activo en
        // cualquier otra pantalla.
        bool activo = (i == 0) ? (pantallaActual == SCREEN_MAIN) : (pantallaActual != SCREEN_MAIN);

        // Fennec no rellena sus botones/tabs: son cajas vacias con el
        // borde en "line" (inactivo) o "sand" (activo), mas una rayita
        // debajo marcando cual esta prendido (drawTab). Calcado aca:
        // nada de fillRect de bloque solido, solo el marco + la marca
        // de activo.
        tft.fillRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, COLOR_BG);
        uint16_t colorBorde = activo ? COLOR_ACCENT : COLOR_ACCENT_DIM;
        tft.drawRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, colorBorde);
        if (activo) {
            // Rayita de "activo" pegada abajo del borde, adentro del boton.
            tft.drawFastHLine(menuBotones[i].x + 3, menuBotones[i].y + menuBotones[i].h - 3,
                               menuBotones[i].w - 6, COLOR_ACCENT);
        }

        // El boton derecho cambia de texto segun donde estamos
        // parados: "Atras" dentro de una herramienta o de la lista de
        // herramientas de una categoria (ahi vuelve un nivel, ver
        // manejarToque() en main.cpp), "Herramientas" en cualquier
        // otro lado (Mascota o la lista de categorias) - mismo
        // comportamiento que tenia antes en esos dos casos.
        const char* etiqueta = menuBotones[i].label;
        if (i == 1 && (pantallaActual == SCREEN_TOOLS_SUB || esPantallaHoja(pantallaActual))) {
            etiqueta = "Atras";
        }

        uint16_t colorTexto = activo ? COLOR_ACCENT : COLOR_TEXT_DIM;
        int anchoTexto = anchoTextoEspaciado(etiqueta);
        int xCentrado = menuBotones[i].x + (menuBotones[i].w - anchoTexto) / 2;
        dibujarTextoEspaciado(xCentrado, menuBotones[i].y + 13, etiqueta, colorTexto, COLOR_BG);
    }
}

int alturaListado() {
    // Lo que queda del alto de pantalla descontando la franja de
    // arriba (donde va el resumen de la mascota) y la barra de botones
    // de abajo. Es la zona util para listas, stats, etc en cada
    // pantalla que no sea la principal.
    return TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - CONTENT_TOP;
}

// Encabezado de seccion: nombre de la pantalla en grande + una linea
// fina debajo. "color" es el color de categoria de la herramienta;
// "icono" es opcional (la lista de Herramientas no necesita uno junto
// a su titulo).
void dibujarTituloSeccion(const char* titulo, uint16_t color, const uint8_t* icono) {
    int x = 10;
    if (icono) {
        tft.drawBitmap(x, CONTENT_TOP + 4, icono, ICONO_ANCHO, ICONO_ALTO, color, COLOR_BG);
        x += ICONO_ANCHO + 6;
    }
    // Titulo con las letras separadas en vez de la fuente 2 pegada de
    // siempre - es el detalle que mas se nota del look "CRT retro" de
    // Fennec (drawSpacedLeft en sus titulos de seccion).
    tft.fillRect(x, CONTENT_TOP + 2, TFT_PANEL_WIDTH - x - 10, 18, COLOR_BG);
    dibujarTextoEspaciado(x, CONTENT_TOP + 6, titulo, color, COLOR_BG, 3);
    // Linea separadora punteada en vez de solida - chrome compartido
    // de toda pantalla, se queda neutra (no de categoria) a proposito.
    dibujarLineaPunteada(10, CONTENT_TOP + 24, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);
}

// Pie de ayuda: un renglon de texto chico (dim, letras separadas) +
// una linea punteada de cierre en vez de la barra solida de antes.
// Las pantallas que lo usan tienen que restarse ALTO_PIE_AYUDA de su
// propio alto disponible para que el contenido no se les superponga
// con esto.
void dibujarPieAyuda(const char* texto) {
    int yAyuda = CONTENT_TOP + alturaListado() - ALTO_PIE_AYUDA + 6;
    tft.fillRect(0, yAyuda - 4, TFT_PANEL_WIDTH, 16, COLOR_BG);
    dibujarTextoEspaciado(10, yAyuda, texto, COLOR_TEXT_DIM, COLOR_BG, 1);

    int yBarra = CONTENT_TOP + alturaListado() - 8;
    dibujarLineaPunteada(6, yBarra, TFT_PANEL_WIDTH - 12, COLOR_ACCENT);
}

// Pantalla de arriba de todo del submenu: las 4 categorias (WIFI,
// WARDRIVING, RADAR, AJUSTES). Tocar una lleva a
// dibujarPantallaHerramientasSub() con esa categoria - ver
// manejarToqueCategorias() mas abajo.
void dibujarPantallaCategorias() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("HERRAMIENTAS");

    for (int i = 0; i < CANTIDAD_CATEGORIAS; i++) {
        Categoria& c = categorias[i];

        // Fila normal: caja marcada solo en las 4 esquinas (estilo
        // Fennec) en vez de un rectangulo relleno o con borde
        // completo. El destello (feedback de que el toque se
        // registro) SI se sigue viendo como bloque lleno un instante
        // - eso es intencional, igual que un boton "lit" en Fennec.
        bool destellada = (i == filaDestellada);
        uint16_t colorFondoFila = destellada ? c.color : COLOR_BG;
        uint16_t colorFrenteFila = destellada ? COLOR_BG : c.color;

        if (destellada) {
            tft.fillRect(2, c.y, TFT_PANEL_WIDTH - 4, c.h, c.color);
        } else {
            dibujarEsquinas(2, c.y, TFT_PANEL_WIDTH - 4, c.h, COLOR_ACCENT_DIM);
        }

        int yIcono = c.y + (c.h - ICONO_ALTO) / 2;
        tft.drawBitmap(10, yIcono, c.icono, ICONO_ANCHO, ICONO_ALTO, colorFrenteFila, colorFondoFila);

        dibujarTextoEspaciado(10 + ICONO_ANCHO + 8, c.y + (c.h / 2) - 3, c.nombre, colorFrenteFila, colorFondoFila, 2);
    }

    if (filaDestellada >= 0 && filaDestellada < CANTIDAD_CATEGORIAS) {
        dibujarPieAyuda(categorias[filaDestellada].descripcion);
    } else {
        dibujarPieAyuda("Toca una categoria para ver sus herramientas");
    }
}

// Segundo nivel del submenu: las herramientas DENTRO de
// categoriaSeleccionada (fijada en manejarToqueCategorias() al tocar
// una fila de dibujarPantallaCategorias()).
void dibujarPantallaHerramientasSub() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    Categoria& cat = categorias[categoriaSeleccionada];
    dibujarTituloSeccion(cat.nombre, cat.color, cat.icono);

    int indices[CANTIDAD_HERRAMIENTAS];
    int cantidad = layoutHerramientasDeCategoria(categoriaSeleccionada, indices);

    // "filaDestellada" aca es la POSICION dentro de esta lista
    // filtrada (0..cantidad-1), no el indice global en herramientas[]
    // - ver manejarToqueHerramientasSub().
    for (int k = 0; k < cantidad; k++) {
        Herramienta& h = herramientas[indices[k]];

        bool destellada = (k == filaDestellada);
        uint16_t colorFondoFila = destellada ? h.color : COLOR_BG;
        uint16_t colorFrenteFila = destellada ? COLOR_BG : h.color;

        if (destellada) {
            tft.fillRect(2, h.y, TFT_PANEL_WIDTH - 4, h.h, h.color);
        } else {
            dibujarEsquinas(2, h.y, TFT_PANEL_WIDTH - 4, h.h, COLOR_ACCENT_DIM, 3);
        }

        int yIcono = h.y + (h.h - ICONO_ALTO) / 2;
        tft.drawBitmap(10, yIcono, h.icono, ICONO_ANCHO, ICONO_ALTO, colorFrenteFila, colorFondoFila);

        dibujarTextoEspaciado(10 + ICONO_ANCHO + 8, h.y + (h.h / 2) - 3, h.nombre, colorFrenteFila, colorFondoFila, 2);
    }

    if (filaDestellada >= 0 && filaDestellada < cantidad) {
        dibujarPieAyuda(herramientas[indices[filaDestellada]].descripcion);
    } else {
        dibujarPieAyuda("Toca una herramienta para entrar");
    }
}

// Toque sobre SCREEN_TOOLS: mismo cuerpo que tenia el case
// SCREEN_TOOLS del switch de manejarToque() en main.cpp, movido tal
// cual.
void manejarToqueCategorias(int y) {
    for (int i = 0; i < CANTIDAD_CATEGORIAS; i++) {
        Categoria& c = categorias[i];
        if (y >= c.y && y <= c.y + c.h) {
            // No navegamos todavia: primero se ve el destello (fila
            // invertida) como feedback de que el toque se registro, y
            // recien cuando pasa DESTELLO_MS (ver loop() en main.cpp)
            // se entra de verdad al submenu de esta categoria.
            filaDestellada = i;
            momentoDestello = millis();
            destinoPendiente = SCREEN_TOOLS_SUB;
            categoriaSeleccionada = i;
            contenidoSucio = true;
            return;
        }
    }
}

// Toque sobre SCREEN_TOOLS_SUB: mismo cuerpo que tenia el case
// SCREEN_TOOLS_SUB del switch de manejarToque() en main.cpp, movido
// tal cual.
void manejarToqueHerramientasSub(int y) {
    int indices[CANTIDAD_HERRAMIENTAS];
    int cantidad = layoutHerramientasDeCategoria(categoriaSeleccionada, indices);
    for (int k = 0; k < cantidad; k++) {
        Herramienta& h = herramientas[indices[k]];
        if (y >= h.y && y <= h.y + h.h) {
            filaDestellada = k;
            momentoDestello = millis();
            destinoPendiente = h.destino;
            contenidoSucio = true;
            return;
        }
    }
}
