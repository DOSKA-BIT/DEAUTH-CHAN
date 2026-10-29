#include "WiFiScanner.h"
#include <esp_wifi.h>

WiFiScanner* WiFiScanner::instance = nullptr;

void WiFiScanner::begin() {
    instance = this;
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    Serial.println("Escaner en modo promiscuo (solo escucha)");
}

void WiFiScanner::startScan() {
    esp_wifi_set_promiscuous(false);
    WiFi.scanNetworks(true, false);
}

bool WiFiScanner::isScanDone() {
    return (WiFi.scanComplete() >= 0);
}

void WiFiScanner::getScanResults(RedInfo* resultados, int maxRedes, int& encontradas) {
    int n = WiFi.scanComplete();

    // BUG REAL que encontre repasando este archivo (no reportado, lo
    // encontre yo mirando el codigo): antes, si un escaneo terminaba
    // con CERO redes encontradas (perfectamente posible - un lugar
    // apantallado, un mal momento, lo que sea), esta funcion cortaba
    // aca con un "return" temprano SIN llegar nunca a la linea de mas
    // abajo que vuelve a prender el modo promiscuo. Resultado: el
    // radio se quedaba en modo escaneo activo PARA SIEMPRE, y el
    // Monitor de deauth/disassoc (ademas de PCAP) dejaba de detectar
    // absolutamente nada el resto de la sesion, sin ningun error
    // visible en pantalla - exactamente el mismo tipo de bug "silencioso"
    // que ya encontramos antes con los File de la SD. Con el escaneo
    // automatico de fondo corriendo cada 45s (ver AUTO_SCAN_MS en
    // main.cpp), bastaba con que UN SOLO escaneo de tantos diera cero
    // redes para que el Monitor dejara de funcionar sin que nadie se
    // diera cuenta. Ahora "cero redes" sigue el mismo camino que
    // "redes encontradas" (el for de abajo simplemente no itera
    // ninguna vez) y SIEMPRE llega a reactivar el modo promiscuo.
    //
    // n < 0 en cambio es otra cosa: significa que el escaneo todavia
    // no termino (no deberia pasar nunca aca, porque isScanDone() ya
    // filtro ese caso antes de llamar a esto, pero por las dudas no
    // tocamos el radio si de verdad no termino todavia).
    if (n < 0) {
        encontradas = 0;
        return;
    }

    encontradas = (n < maxRedes) ? n : maxRedes;
    for (int i = 0; i < encontradas; i++) {
        strncpy(resultados[i].ssid, WiFi.SSID(i).c_str(), 32);
        resultados[i].ssid[32] = '\0';
        memcpy(resultados[i].bssid, WiFi.BSSID(i), 6);
        resultados[i].rssi = WiFi.RSSI(i);
        resultados[i].canal = WiFi.channel(i);
        resultados[i].tieneClave = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    WiFi.scanDelete();
    esp_wifi_set_promiscuous(true);
    Serial.printf("%d redes encontradas\n", encontradas);
}

void WiFiScanner::iniciarConteoClientes(const uint8_t* bssid, int canal) {
    portENTER_CRITICAL(&mux);
    memcpy(bssidObjetivo, bssid, 6);
    canalObjetivo = canal;
    numClientes = 0;
    conteoInicio = millis();
    escuchandoClientes = true;
    portEXIT_CRITICAL(&mux);

    esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
    Serial.printf("Escuchando trafico en canal %d\n", canal);
    // Y listo, se vuelve ya mismo - nada de quedarse esperando aca.
    // El corte a los CONTEO_CLIENTES_MS lo hace update().
}

void WiFiScanner::update() {
    if (escuchandoClientes && millis() - conteoInicio >= CONTEO_CLIENTES_MS) {
        finalizarConteoClientes();
    }

    // Muestreo del analizador de canales (ver el comentario largo de
    // ANALIZADOR_DWELL_MS en el .h): cada tanto, congela lo que se
    // acumulo de cada tipo de frame desde la ultima muestra, lo
    // empuja al historial (shift simple, igual que EventLog/
    // registrarAnomalia - con 48 lugares nomas no vale la pena un
    // indice de buffer circular) y arranca de nuevo en 0. No bloquea
    // nada ni toca el radio - el canal lo fija setCanalAnalizador(),
    // esto solo lee contadores.
    if (analizadorActivo && millis() - analizadorUltimoMuestreo >= ANALIZADOR_DWELL_MS) {
        uint16_t b, d, p;
        portENTER_CRITICAL(&mux);
        b = accBeacons; accBeacons = 0;
        d = accDeauths; accDeauths = 0;
        p = accProbes;  accProbes = 0;
        portEXIT_CRITICAL(&mux);

        for (int i = 0; i < ANALIZADOR_MUESTRAS - 1; i++) {
            histBeacons[i] = histBeacons[i + 1];
            histDeauths[i] = histDeauths[i + 1];
            histProbes[i] = histProbes[i + 1];
        }
        histBeacons[ANALIZADOR_MUESTRAS - 1] = b;
        histDeauths[ANALIZADOR_MUESTRAS - 1] = d;
        histProbes[ANALIZADOR_MUESTRAS - 1] = p;

        analizadorUltimoMuestreo = millis();
    }
}

void WiFiScanner::iniciarAnalizadorCanales() {
    for (int i = 0; i < ANALIZADOR_MUESTRAS; i++) {
        histBeacons[i] = 0;
        histDeauths[i] = 0;
        histProbes[i] = 0;
    }
    portENTER_CRITICAL(&mux);
    accBeacons = 0;
    accDeauths = 0;
    accProbes = 0;
    portEXIT_CRITICAL(&mux);

    analizadorUltimoMuestreo = millis();
    esp_wifi_set_channel(canalActualAnalizador, WIFI_SECOND_CHAN_NONE);
    analizadorActivo = true;
    Serial.printf("Analizador de canales: arrancado en canal %d\n", canalActualAnalizador);
}

void WiFiScanner::detenerAnalizadorCanales() {
    // Solo baja la bandera - el radio se queda escuchando en el mismo
    // canal en el que estaba, mismo criterio que ya usa
    // iniciarConteoClientes()/finalizarConteoClientes() (tampoco
    // "devuelven" el canal a ningun lado en particular al terminar).
    analizadorActivo = false;
    Serial.println("Analizador de canales: detenido");
}

void WiFiScanner::setCanalAnalizador(int canal) {
    if (canal < 1) canal = 1;
    if (canal > 13) canal = 13;
    if (canal == canalActualAnalizador) return;

    canalActualAnalizador = canal;

    // Mezclar en el mismo grafico trafico de dos canales distintos no
    // diria nada real, asi que cambiar de canal a mano limpia el
    // historial igual que iniciarAnalizadorCanales().
    for (int i = 0; i < ANALIZADOR_MUESTRAS; i++) {
        histBeacons[i] = 0;
        histDeauths[i] = 0;
        histProbes[i] = 0;
    }
    portENTER_CRITICAL(&mux);
    accBeacons = 0;
    accDeauths = 0;
    accProbes = 0;
    portEXIT_CRITICAL(&mux);

    if (analizadorActivo) {
        esp_wifi_set_channel(canalActualAnalizador, WIFI_SECOND_CHAN_NONE);
    }
}

void WiFiScanner::getHistorialAnalizador(uint16_t* outBeacons, uint16_t* outDeauths, uint16_t* outProbes, int n) {
    if (n > ANALIZADOR_MUESTRAS) n = ANALIZADOR_MUESTRAS;
    memcpy(outBeacons, histBeacons, n * sizeof(uint16_t));
    memcpy(outDeauths, histDeauths, n * sizeof(uint16_t));
    memcpy(outProbes, histProbes, n * sizeof(uint16_t));
}

bool WiFiScanner::conteoEnProgreso() {
    return escuchandoClientes;
}

unsigned long WiFiScanner::tiempoRestanteConteo() {
    if (!escuchandoClientes) return 0;
    unsigned long transcurrido = millis() - conteoInicio;
    if (transcurrido >= CONTEO_CLIENTES_MS) return 0;
    return CONTEO_CLIENTES_MS - transcurrido;
}

void WiFiScanner::finalizarConteoClientes() {
    portENTER_CRITICAL(&mux);
    unsigned long ahora = millis();
    for (int i = 0; i < numClientes; i++) {
        if (ahora - clientes[i].ultimaVez > CLIENTE_TIMEOUT) {
            for (int j = i; j < numClientes - 1; j++) {
                memcpy(&clientes[j], &clientes[j + 1], sizeof(ClienteInfo));
            }
            numClientes--;
            i--;
        }
    }
    escuchandoClientes = false;
    int numClientesFinal = numClientes;
    portEXIT_CRITICAL(&mux);

    Serial.printf("Dispositivos distintos escuchados: %d\n", numClientesFinal);
}

ClienteInfo* WiFiScanner::getClientes(int& count) {
    // Antes esto devolvia un puntero directo a clientes[], el mismo
    // array que promiscuousCallback() puede estar modificando EN ESE
    // MISMO INSTANTE desde la tarea del driver WiFi (en paralelo con
    // loop()). dibujarListaRedes() podia terminar imprimiendo un MAC o
    // un RSSI a medio escribir, o leer mas alla de "count" si el
    // callback agregaba un cliente nuevo mientras se estaba
    // dibujando. Ahora se copia todo de una vez bajo lock a un
    // "snapshot" propio, y se devuelve ESE - lo que lee la pantalla
    // nunca cambia por debajo mientras lo esta leyendo.
    portENTER_CRITICAL(&mux);
    numClientesSnapshot = numClientes;
    memcpy(clientesSnapshot, clientes, sizeof(ClienteInfo) * numClientesSnapshot);
    portEXIT_CRITICAL(&mux);

    count = numClientesSnapshot;
    return clientesSnapshot;
}

void WiFiScanner::setLED(bool state) {
    digitalWrite(TFT_BL_PIN, state ? HIGH : LOW);
}

void WiFiScanner::registrarAnomalia(bool esDisassoc, uint8_t* origen, uint8_t* destino, int canal, int rssi) {
    portENTER_CRITICAL(&mux);
    if (esDisassoc) {
        contadorDisassoc++;
    } else {
        contadorDeauth++;
    }

    // Shift simple, igual que en EventLog: con 8 lugares nomas no
    // vale la pena un indice de buffer circular para esto.
    for (int i = MAX_ANOMALIAS_RAM - 1; i > 0; i--) {
        anomalias[i] = anomalias[i - 1];
    }
    anomalias[0].momento = millis();
    memcpy(anomalias[0].origen, origen, 6);
    memcpy(anomalias[0].destino, destino, 6);
    anomalias[0].canal = canal;
    anomalias[0].rssi = rssi;
    anomalias[0].esDisassoc = esDisassoc;

    if (numAnomalias < MAX_ANOMALIAS_RAM) numAnomalias++;
    portEXIT_CRITICAL(&mux);
}

void WiFiScanner::getAnomaliasStats(unsigned long& deauth, unsigned long& disassoc) {
    // Dos "unsigned long" sueltos no se leen atomicamente solos en
    // ningun micro de 8/16/32 bits en general (y en el ESP32 ya hay
    // dos nucleos de por medio), asi que el lock sigue haciendo falta
    // aca aunque parezca una lectura trivial.
    portENTER_CRITICAL(&mux);
    deauth = contadorDeauth;
    disassoc = contadorDisassoc;
    portEXIT_CRITICAL(&mux);
}

AnomaliaWiFi* WiFiScanner::getAnomalias(int& count) {
    // Mismo motivo que getClientes() de arriba: se copia a un
    // snapshot propio bajo lock en vez de devolver un puntero al
    // array que registrarAnomalia() puede estar modificando desde la
    // tarea del driver WiFi en paralelo.
    portENTER_CRITICAL(&mux);
    numAnomaliasSnapshot = numAnomalias;
    memcpy(anomaliasSnapshot, anomalias, sizeof(AnomaliaWiFi) * numAnomaliasSnapshot);
    portEXIT_CRITICAL(&mux);

    count = numAnomaliasSnapshot;
    return anomaliasSnapshot;
}

void WiFiScanner::empujarPCAP(const uint8_t* frame, uint16_t len) {
    // Corre dentro de promiscuousCallback() (tarea del driver WiFi).
    // Solo copia bytes a RAM bajo el mux - nada de SD aca, ver el
    // comentario largo de pcapBuffer en el .h de por que.
    portENTER_CRITICAL(&mux);
    if (pcapPendientes < PCAP_BUFFER_SIZE) {
        PaqueteCrudoWiFi& p = pcapBuffer[pcapEscribirIdx];
        uint16_t copiar = (len > PCAP_FRAME_MAX) ? PCAP_FRAME_MAX : len;
        memcpy(p.datos, frame, copiar);
        p.len = copiar;
        unsigned long ahora = millis();
        p.ts_sec = ahora / 1000;
        p.ts_usec = (ahora % 1000) * 1000;
        pcapEscribirIdx = (pcapEscribirIdx + 1) % PCAP_BUFFER_SIZE;
        pcapPendientes++;
        pcapCapturados++;
    }
    // Si el buffer esta lleno (loop() no llego a drenarlo a tiempo),
    // se descarta el paquete nuevo en vez de pisar uno viejo todavia
    // no escrito - preferible perder UN paquete de mas a complicar la
    // lectura con paquetes parcialmente sobreescritos. En uso normal
    // (loop() drena en cada vuelta, ver main.cpp) esto no deberia
    // pasar salvo una rafaga muy densa de beacons.
    portEXIT_CRITICAL(&mux);
}

bool WiFiScanner::sacarPaquetePCAP(PaqueteCrudoWiFi& out) {
    // Corre desde loop() (tarea principal) - ver el comentario largo
    // de pcapBuffer en el .h.
    portENTER_CRITICAL(&mux);
    bool hay = pcapPendientes > 0;
    if (hay) {
        out = pcapBuffer[pcapLeerIdx];
        pcapLeerIdx = (pcapLeerIdx + 1) % PCAP_BUFFER_SIZE;
        pcapPendientes--;
    }
    portEXIT_CRITICAL(&mux);
    return hay;
}

void WiFiScanner::promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!instance) return;

    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t* frame = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;
    int rssi = pkt->rx_ctrl.rssi;

    if (len < 24) return;

    // Monitor pasivo de deauth/disassoc: se fija en esto para TODO
    // paquete que pasa por aca, no solo mientras se esta contando
    // clientes de un BSSID puntual (por eso va antes del "return" de
    // abajo). El frame control son los primeros 2 bytes de cualquier
    // frame 802.11; el tipo esta en los bits 2-3 y el subtipo en los
    // bits 4-7. Tipo 0 = management, subtipo 0x0C = deauth, 0x0A =
    // disassoc.
    uint8_t tipoFrame = (frame[0] >> 2) & 0x3;
    uint8_t subtipoFrame = (frame[0] >> 4) & 0xF;
    if (tipoFrame == 0 && (subtipoFrame == 0x0C || subtipoFrame == 0x0A)) {
        // addr1 = destino (frame[4..9]), addr2 = origen/transmisor
        // (frame[10..15]) - mismos offsets que usa el conteo de
        // clientes de aca abajo.
        instance->registrarAnomalia(subtipoFrame == 0x0A, &frame[10], &frame[4],
            pkt->rx_ctrl.channel, rssi);
    }

    // Captura cruda para PCAP (si esta activada desde Monitor): mismo
    // chequeo de tipo/subtipo que el monitor de deauth/disassoc de
    // arriba, mas beacon (0x08) y probe request/response (0x04/0x05) -
    // todo frame de management que explica "que hay dando vueltas en
    // el aire", nada de datos ni de handshakes. Va antes del "return"
    // de aca abajo por el mismo motivo que el monitor: tiene que ver
    // TODO paquete, no solo mientras se cuenta clientes de un BSSID
    // puntual.
    if (instance->pcapActivo && tipoFrame == 0 &&
        (subtipoFrame == 0x08 || subtipoFrame == 0x04 || subtipoFrame == 0x05 ||
         subtipoFrame == 0x0C || subtipoFrame == 0x0A)) {
        instance->empujarPCAP(frame, len);
    }

    // Analizador de canales: clasifica cada frame del canal que se
    // esta analizando en una de las 3 series que calca de Marauder -
    // Beacons, Deauths (deauth+disassoc, igual que agrupa el legend
    // real) y Probes (request+response). El chequeo de canal es por
    // las dudas (el radio deberia estar siempre parado en
    // canalActualAnalizador mientras esto esta activo, pero evita
    // contar algo de otro canal en el instante exacto de un cambio).
    if (instance->analizadorActivo && pkt->rx_ctrl.channel == instance->canalActualAnalizador &&
        tipoFrame == 0) {
        if (subtipoFrame == 0x08) {
            portENTER_CRITICAL(&instance->mux);
            instance->accBeacons++;
            portEXIT_CRITICAL(&instance->mux);
        } else if (subtipoFrame == 0x0C || subtipoFrame == 0x0A) {
            portENTER_CRITICAL(&instance->mux);
            instance->accDeauths++;
            portEXIT_CRITICAL(&instance->mux);
        } else if (subtipoFrame == 0x04 || subtipoFrame == 0x05) {
            portENTER_CRITICAL(&instance->mux);
            instance->accProbes++;
            portEXIT_CRITICAL(&instance->mux);
        }
    }

    if (!instance->escuchandoClientes) return;

    uint8_t* addr1 = &frame[4];
    uint8_t* addr2 = &frame[10];
    uint8_t* addr3 = &frame[16];

    uint8_t* bssid = addr3;
    if (memcmp(bssid, instance->bssidObjetivo, 6) != 0) {
        if (memcmp(addr1, instance->bssidObjetivo, 6) != 0 &&
            memcmp(addr2, instance->bssidObjetivo, 6) != 0) {
            return;
        }
    }

    uint8_t* macCliente = nullptr;
    if (memcmp(addr1, instance->bssidObjetivo, 6) != 0) {
        macCliente = addr1;
    } else if (memcmp(addr2, instance->bssidObjetivo, 6) != 0) {
        macCliente = addr2;
    } else {
        return;
    }

    instance->actualizarCliente(macCliente, rssi, instance->bssidObjetivo, instance->canalObjetivo);
}

void WiFiScanner::actualizarCliente(uint8_t* mac, int rssi, uint8_t* bssid, int canal) {
    unsigned long ahora = millis();

    // Corre dentro de promiscuousCallback() (tarea del driver WiFi),
    // al mismo tiempo que loop() puede estar en getClientes() o en
    // finalizarConteoClientes() tocando este mismo array - de ahi el
    // lock. buscarOUI() es solo lectura de una tabla fija (OUI_DB.h),
    // no hace falta meterla adentro de la seccion critica.
    portENTER_CRITICAL(&mux);
    bool encontrado = false;
    for (int i = 0; i < numClientes; i++) {
        if (memcmp(clientes[i].mac, mac, 6) == 0) {
            clientes[i].rssi = rssi;
            clientes[i].ultimaVez = ahora;
            encontrado = true;
            break;
        }
    }

    if (!encontrado && numClientes < MAX_CLIENTES) {
        memcpy(clientes[numClientes].mac, mac, 6);
        clientes[numClientes].rssi = rssi;
        clientes[numClientes].canal = canal;
        memcpy(clientes[numClientes].bssid, bssid, 6);
        clientes[numClientes].ultimaVez = ahora;
        const char* fab = buscarOUI(mac);
        strncpy(clientes[numClientes].fabricante, fab, 15);
        clientes[numClientes].fabricante[15] = '\0';
        numClientes++;
    }
    portEXIT_CRITICAL(&mux);
}
