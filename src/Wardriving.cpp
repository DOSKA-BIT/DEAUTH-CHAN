#include "Wardriving.h"
#include "boards/BoardConfig.h"

void Wardriving::begin() {
    // Antes esto dejaba "csvFile" abierto toda la sesion y
    // saveNetwork() escribia directamente ahi. El problema:
    // dibujarPantallaConfig() y dibujarPantallaWardrive() hacen
    // SD.end()/SD.begin() cada vez que se dibujan (para chequear
    // "SD: OK" o contar lineas del CSV), y exportKML() tambien - ese
    // SD.end() desmonta la tarjeta y invalida cualquier File que haya
    // quedado abierto de antes. Resultado: apenas se visitaba Config
    // o Wardrive una vez, las redes de los escaneos siguientes
    // dejaban de anotarse en wardriving.csv, sin ningun error visible
    // (escribir en un File invalido no crashea, simplemente no
    // escribe nada). Ahora cada saveNetwork() abre, escribe y cierra
    // de nuevo (ver mas abajo), igual que ya hace
    // Learning::guardarEnSD() - mas remontajes de SD por escaneo, pero
    // ninguno queda "viejo" esperando a romperse con el proximo toque
    // a otra pantalla.
    SD.end();
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD no disponible para wardriving");
        return;
    }

    if (!SD.exists("/wardriving.csv")) {
        File f = SD.open("/wardriving.csv", FILE_WRITE);
        if (f) {
            f.println("timestamp,ssid,bssid,rssi,channel,encryption,lat,lng,altitude");
            f.close();
        }
    }
    Serial.println("Wardriving iniciado");
}

void Wardriving::setFilterByEncryption(bool enabled, int authMode) {
    filterEnabled = enabled;
    filterAuthMode = authMode;
}

void Wardriving::saveNetwork(const RedInfo& red, const GPSData& pos) {
    if (filterEnabled) {
        // RedInfo solo guarda si la red tiene clave o no (un bool), no
        // el modo exacto (WPA/WPA2/etc) - asi que el filtro tampoco
        // puede distinguir mas que eso. filterAuthMode == WIFI_AUTH_OPEN
        // significa "quiero solo las abiertas"; cualquier otro valor
        // significa "quiero solo las que tienen clave, no importa cual".
        bool quiereAbiertas = (filterAuthMode == WIFI_AUTH_OPEN);
        bool esAbierta = !red.tieneClave;
        if (quiereAbiertas != esAbierta) return; // no cumple el filtro, se descarta
    }

    // Se remonta, abre, escribe y cierra en cada llamada (ver el
    // comentario largo en begin()) en vez de reusar un File que haya
    // quedado abierto de antes - asi nunca importa si otra pantalla
    // desmonto la SD entre medio. Esto se llama una vez por red
    // encontrada al terminar un escaneo (no en cada vuelta del loop),
    // asi que el costo extra de remontar es chico comparado con
    // perder datos en silencio.
    SD.end();
    if (!SD.begin(SD_CS_PIN)) return;
    File f = SD.open("/wardriving.csv", FILE_APPEND);
    if (!f) return;

    f.printf("%s,%s,%02X:%02X:%02X:%02X:%02X:%02X,%d,%d,%d,%.6f,%.6f,%.1f\n",
        pos.valid ? pos.timestamp : "NO_FIX",
        red.ssid,
        red.bssid[0], red.bssid[1], red.bssid[2],
        red.bssid[3], red.bssid[4], red.bssid[5],
        red.rssi,
        red.canal,
        red.tieneClave ? 1 : 0,
        pos.lat,
        pos.lng,
        pos.altitude
    );
    f.flush();
    f.close();
}

// XML no permite & < > " ' sueltos adentro de un texto. Un SSID real
// puede tener cualquiera de estos (por ejemplo un WiFi llamado
// "Tom&Jerry"), asi que hay que escaparlos antes de escribirlos o el
// .kml queda invalido y no abre en Google Earth/Maps.
static void escribirTextoEscapado(File& f, const String& texto) {
    for (size_t i = 0; i < texto.length(); i++) {
        char c = texto[i];
        switch (c) {
            case '&':  f.print("&amp;");  break;
            case '<':  f.print("&lt;");   break;
            case '>':  f.print("&gt;");   break;
            case '"':  f.print("&quot;"); break;
            case '\'': f.print("&apos;"); break;
            default:   f.print(c);        break;
        }
    }
}

int Wardriving::exportKML() {
    SD.end();
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("No se pudo montar la SD para exportar KML");
        return -1;
    }

    if (!SD.exists("/wardriving.csv")) {
        Serial.println("No hay wardriving.csv todavia, nada que exportar");
        return -1;
    }

    File csvRead = SD.open("/wardriving.csv", FILE_READ);
    if (!csvRead) {
        Serial.println("No se pudo abrir wardriving.csv para exportar");
        return -1;
    }

    File kml = SD.open("/wardriving.kml", FILE_WRITE);
    if (!kml) {
        Serial.println("No se pudo crear wardriving.kml");
        csvRead.close();
        return -1;
    }

    kml.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    kml.println("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
    kml.println("<Document>");
    kml.println("<name>DEAUTH-CHAN wardriving</name>");
    // El color en KML va en formato ABGR (alfa-azul-verde-rojo), no
    // RGB como de costumbre - por eso los pares de hex estan al reves
    // de lo que uno esperaria para verde/amarillo/rojo.
    kml.println("<Style id=\"fuerte\"><IconStyle><color>ff00ff00</color></IconStyle></Style>");
    kml.println("<Style id=\"media\"><IconStyle><color>ff00ffff</color></IconStyle></Style>");
    kml.println("<Style id=\"debil\"><IconStyle><color>ff0000ff</color></IconStyle></Style>");

    bool esEncabezado = true;
    int puntosEscritos = 0;
    int puntosSinFix = 0;

    while (csvRead.available()) {
        String linea = csvRead.readStringUntil('\n');
        linea.trim();
        if (linea.length() == 0) continue;
        if (esEncabezado) {
            esEncabezado = false;
            continue;
        }

        // Columnas, en el orden que las escribe saveNetwork():
        // timestamp,ssid,bssid,rssi,channel,encryption,lat,lng,altitude
        // Si un SSID tuviera una coma adentro esto se desalinearia -
        // es una limitacion que ya tenia el CSV (no escapa comas), no
        // algo nuevo de este exportador.
        int c1 = linea.indexOf(',');
        int c2 = linea.indexOf(',', c1 + 1);
        int c3 = linea.indexOf(',', c2 + 1);
        int c4 = linea.indexOf(',', c3 + 1);
        int c5 = linea.indexOf(',', c4 + 1);
        int c6 = linea.indexOf(',', c5 + 1);
        int c7 = linea.indexOf(',', c6 + 1);
        int c8 = linea.indexOf(',', c7 + 1);
        if (c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0 || c5 < 0 || c6 < 0 || c7 < 0 || c8 < 0) {
            continue; // linea incompleta o corrupta, se salta
        }

        String ssid = linea.substring(c1 + 1, c2);
        int rssi = linea.substring(c3 + 1, c4).toInt();
        String lat = linea.substring(c6 + 1, c7);
        String lng = linea.substring(c7 + 1, c8);

        if (lat.toFloat() == 0.0 && lng.toFloat() == 0.0) {
            // Se guardo sin fix de GPS en su momento (NO_FIX): no hay
            // donde ponerla en el mapa, mejor omitirla que clavar un
            // pin en 0,0 en medio del oceano.
            puntosSinFix++;
            continue;
        }

        if (ssid.length() == 0) ssid = "*OCULTA*";
        const char* estilo = (rssi >= -60) ? "fuerte" : (rssi >= -75) ? "media" : "debil";

        kml.print("<Placemark><name>");
        escribirTextoEscapado(kml, ssid);
        kml.print("</name><description>");
        kml.print(rssi);
        kml.print(" dBm</description><styleUrl>#");
        kml.print(estilo);
        kml.print("</styleUrl><Point><coordinates>");
        kml.print(lng);
        kml.print(",");
        kml.print(lat);
        kml.println(",0</coordinates></Point></Placemark>");

        puntosEscritos++;
    }

    kml.println("</Document>");
    kml.println("</kml>");
    kml.flush();
    kml.close();
    csvRead.close();

    Serial.printf("KML exportado: %d puntos (%d sin GPS, omitidos)\n", puntosEscritos, puntosSinFix);
    return puntosEscritos;
}
