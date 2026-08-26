#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <WiFi.h>
#include "config.h"
#include "OUI_DB.h"

#define MAX_CLIENTES 20
#define CLIENTE_TIMEOUT 10000

// Info de un dispositivo visto asociado a un BSSID durante el conteo
// pasivo. No hacemos nada con esto mas que mostrarlo en pantalla y
// guardarlo en el log de wardriving - es telemetria, no un blanco.
struct ClienteInfo {
    uint8_t mac[6];
    int rssi;
    int canal;
    uint8_t bssid[6];
    unsigned long ultimaVez;
    char fabricante[16];
};

class WiFiScanner {
public:
    void begin();

    void startScan();
    bool isScanDone();
    void getScanResults(RedInfo* resultados, int maxRedes, int& encontradas);

    // Cuenta cuantos dispositivos distintos se escuchan hablando con un
    // BSSID puntual, quedandose escuchando en su canal unos segundos.
    // Es pura recepcion (modo promiscuo), en ningun momento se transmite.
    void contarClientes(const uint8_t* bssid, int canal);
    ClienteInfo* getClientes(int& count);

    void setLED(bool state);

private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void actualizarCliente(uint8_t* mac, int rssi, uint8_t* bssid, int canal);

    ClienteInfo clientes[MAX_CLIENTES];
    int numClientes = 0;
    uint8_t bssidObjetivo[6];
    int canalObjetivo = 0;
    bool escuchandoClientes = false;

    static WiFiScanner* instance;
};

#endif
