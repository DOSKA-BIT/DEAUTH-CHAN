#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <WiFi.h>
#include "config.h"
#include "OUI_DB.h"

#define MAX_CLIENTES 20
#define CLIENTE_TIMEOUT 10000
#define MAX_ANOMALIAS_RAM 8

// Un frame de deauth o disassoc suelto es normal (un router dando de
// baja a un cliente que se fue, por ejemplo). Esto no decide si un
// evento puntual es un ataque - eso hace falta mirarlo en contexto,
// no paquete por paquete. Lo que hace es dejar registrado cada uno
// para que la persona pueda notar una rafaga (muchos en pocos
// segundos = sospechoso) en vez de uno aislado cada tanto (normal).
// Pura recepcion: en ningun lado de este archivo, ni de todo el
// proyecto, se arma o se transmite un frame de este tipo.
struct AnomaliaWiFi {
    unsigned long momento;  // millis() de cuando se vio
    uint8_t origen[6];      // addr2: quien dice mandar el frame
    uint8_t destino[6];     // addr1: a quien se lo manda (o broadcast)
    int canal;
    int rssi;
    bool esDisassoc;        // false = deauth (subtipo 0x0C), true = disassoc (0x0A)
};

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

    // Monitor pasivo de deauth/disassoc: corre solo, todo el tiempo
    // que el radio esta en modo promiscuo (o sea, casi siempre salvo
    // el ratito de un scan activo), sin que haga falta prender nada
    // a mano. getAnomaliasStats da los totales acumulados desde el
    // arranque, getAnomalias da los ultimos vistos para listarlos.
    void getAnomaliasStats(unsigned long& deauth, unsigned long& disassoc);
    AnomaliaWiFi* getAnomalias(int& count);

private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void actualizarCliente(uint8_t* mac, int rssi, uint8_t* bssid, int canal);
    void registrarAnomalia(bool esDisassoc, uint8_t* origen, uint8_t* destino, int canal, int rssi);

    ClienteInfo clientes[MAX_CLIENTES];
    int numClientes = 0;
    uint8_t bssidObjetivo[6];
    int canalObjetivo = 0;
    bool escuchandoClientes = false;

    AnomaliaWiFi anomalias[MAX_ANOMALIAS_RAM];
    int numAnomalias = 0;
    unsigned long contadorDeauth = 0;
    unsigned long contadorDisassoc = 0;

    static WiFiScanner* instance;
};

#endif
