// Pantalla de la categoria WARDRIVING + su toque asociado. Movido tal
// cual desde main.cpp, sin cambiar ningun comportamiento.
#include "Globals.h"

void dibujarPantallaWardrive() {
    tft.fillRect(0, CONTENT_TOP, TFT_PANEL_WIDTH, alturaListado(), COLOR_BG);
    dibujarTituloSeccion("WARDRIVE", COLOR_CAT_VIOLET, ICONO_WARDRIVE);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 35);
    tft.print("GPS: ");
    GPSData pos = gps.getData();
    tft.setTextColor(pos.valid ? COLOR_CAT_VIOLET : COLOR_WARN, COLOR_BG);
    tft.print(pos.valid ? "FIX" : "SIN FIX");

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(10, CONTENT_TOP + 55);
    tft.print("Satelites: ");
    tft.print(pos.satellites);

    tft.setCursor(10, CONTENT_TOP + 75);
    tft.print("Redes guardadas: ");
    // Wardriving ya lleva la cuenta el solo, sin releer wardriving.csv
    // ni remontar la SD en cada redibujado (ver Wardriving::
    // getContadorRedes()).
    tft.print(wardriver.getContadorRedes());

    // Filtro de que se guarda: se cicla tocando esta linea. Se guarda
    // en RAM nomas (no en la SD), asi que vuelve a "Todas" en cada
    // reinicio.
    tft.setCursor(10, CONTENT_TOP + 95);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.print("Filtro [toca]: ");
    tft.setTextColor(COLOR_CAT_VIOLET, COLOR_BG);
    if (filtroWardrive == 0) {
        tft.print("Todas");
    } else if (filtroWardrive == 1) {
        tft.print("Solo abiertas");
    } else {
        tft.print("Solo con clave");
    }

    tft.setCursor(10, CONTENT_TOP + 120);
    tft.setTextColor(COLOR_WARN, COLOR_BG);
    tft.print("[Toca para exportar KML]");

    // Resultado del ultimo export, para que tocar el boton de arriba
    // no se sienta como que no paso nada.
    tft.setCursor(10, CONTENT_TOP + 145);
    if (ultimoExportKML == -2) {
        tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        tft.print("(todavia no exportaste nada)");
    } else if (ultimoExportKML < 0) {
        tft.setTextColor(COLOR_DANGER, COLOR_BG);
        tft.print("Error: sin SD o sin datos");
    } else {
        tft.setTextColor(ultimoExportKML > 0 ? COLOR_CAT_VIOLET : COLOR_WARN, COLOR_BG);
        tft.print("wardriving.kml: ");
        tft.print(ultimoExportKML);
        tft.print(" puntos");
    }

    dibujarPieAyuda("Guarda cada red vista, con GPS si hay fix");

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        touchDriver.reclaimBus();
    #endif
}

// Toque sobre SCREEN_WARDRIVE - mismo cuerpo que tenia ese case del
// switch de manejarToque() en main.cpp, movido tal cual.
void manejarToqueWardriving(int x, int y) {
    if (y > CONTENT_TOP + 90 && y < CONTENT_TOP + 110) {
        // Cicla entre los 3 modos. El filtro solo distingue "tiene
        // clave si o no" porque es lo unico que guarda RedInfo
        // (tieneClave es un bool).
        filtroWardrive = (filtroWardrive + 1) % 3;
        if (filtroWardrive == 0) {
            wardriver.setFilterByEncryption(false, WIFI_AUTH_OPEN);
        } else if (filtroWardrive == 1) {
            wardriver.setFilterByEncryption(true, WIFI_AUTH_OPEN);
        } else {
            wardriver.setFilterByEncryption(true, WIFI_AUTH_WPA2_PSK);
        }
        contenidoSucio = true;
    } else if (y > CONTENT_TOP + 118 && y < CONTENT_TOP + 140) {
        ultimoExportKML = wardriver.exportKML();
        #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
            // exportKML() remonta la SD (lee el csv y escribe el kml),
            // asi que hay que reclamar el bus para el touch antes de
            // que se lea el proximo toque.
            touchDriver.reclaimBus();
        #endif
        contenidoSucio = true; // para mostrar el resultado ya mismo
    }
}
