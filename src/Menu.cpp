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

void dibujarMenuBotones() {
    // Linea fina arriba de la barra, para separarla del contenido en
    // vez de que los botones queden flotando sin mas.
    tft.drawFastHLine(0, TFT_PANEL_HEIGHT - MENU_BAR_HEIGHT - 1, TFT_PANEL_WIDTH, COLOR_ACCENT_DIM);

    for (int i = 0; i < 2; i++) {
        // El boton de Mascota se marca activo solo en la pantalla
        // principal; el de Herramientas se marca activo en cualquier
        // otra pantalla.
        bool activo = (i == 0) ? (pantallaActual == SCREEN_MAIN) : (pantallaActual != SCREEN_MAIN);
        uint16_t colorFondo = activo ? COLOR_ACCENT : COLOR_PANEL;
        uint16_t colorTexto = activo ? COLOR_BG : COLOR_TEXT_DIM;
        tft.fillRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, colorFondo);
        if (!activo) {
            tft.drawRect(menuBotones[i].x, menuBotones[i].y, menuBotones[i].w, menuBotones[i].h, COLOR_ACCENT_DIM);
        }
        tft.setTextColor(colorTexto, colorFondo);
        tft.setTextFont(1);
        tft.setTextSize(1);
        int anchoTexto = strlen(menuBotones[i].label) * 6;
        int xCentrado = menuBotones[i].x + (menuBotones[i].w - anchoTexto) / 2;
        tft.setCursor(xCentrado, menuBotones[i].y + 13);
        tft.print(menuBotones[i].label);
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
    tft.setTextColor(color, COLOR_BG);
    tft.setTextFont(2);
    tft.setCursor(x, CONTENT_TOP + 6);
    tft.print(titulo);
    tft.setTextFont(1);
    // La linea separadora se queda neutra (no de categoria) a
    // proposito: es chrome compartido de toda pantalla.
    tft.drawFastHLine(10, CONTENT_TOP + 24, TFT_PANEL_WIDTH - 20, COLOR_ACCENT_DIM);
}

// Pie de ayuda: un renglon de texto chico (dim) + una barra solida de
// cierre. Las pantallas que lo usan tienen que restarse
// ALTO_PIE_AYUDA de su propio alto disponible para que el contenido
// no se les superponga con esto.
void dibujarPieAyuda(const char* texto) {
    int yAyuda = CONTENT_TOP + alturaListado() - ALTO_PIE_AYUDA + 6;
    tft.fillRect(0, yAyuda - 4, TFT_PANEL_WIDTH, 16, COLOR_BG);
    tft.setTextFont(1);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(10, yAyuda);
    tft.print(texto);

    int yBarra = CONTENT_TOP + alturaListado() - 8;
    tft.fillRect(6, yBarra, TFT_PANEL_WIDTH - 12, 5, COLOR_ACCENT);
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

        bool destellada = (i == filaDestellada);
        uint16_t colorFondoFila = destellada ? c.color : COLOR_BG;
        uint16_t colorFrenteFila = destellada ? COLOR_BG : c.color;

        if (destellada) {
            tft.fillRect(0, c.y, TFT_PANEL_WIDTH, c.h, c.color);
        }

        int yIcono = c.y + (c.h - ICONO_ALTO) / 2;
        tft.drawBitmap(10, yIcono, c.icono, ICONO_ANCHO, ICONO_ALTO, colorFrenteFila, colorFondoFila);

        tft.setTextColor(colorFrenteFila, colorFondoFila);
        tft.setTextFont(2);
        tft.setCursor(10 + ICONO_ANCHO + 8, c.y + (c.h / 2) - 7);
        tft.print(c.nombre);
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
            tft.fillRect(0, h.y, TFT_PANEL_WIDTH, h.h, h.color);
        }

        int yIcono = h.y + (h.h - ICONO_ALTO) / 2;
        tft.drawBitmap(10, yIcono, h.icono, ICONO_ANCHO, ICONO_ALTO, colorFrenteFila, colorFondoFila);

        tft.setTextColor(colorFrenteFila, colorFondoFila);
        tft.setTextFont(2);
        tft.setCursor(10 + ICONO_ANCHO + 8, h.y + (h.h / 2) - 7);
        tft.print(h.nombre);
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
