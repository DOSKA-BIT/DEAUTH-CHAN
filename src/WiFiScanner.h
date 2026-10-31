#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <WiFi.h>
#include "config.h"
#include "OUI_DB.h"

#define MAX_CLIENTES 20
#define CLIENTE_TIMEOUT 10000
#define CONTEO_CLIENTES_MS 3000
#define MAX_ANOMALIAS_RAM 8
#define PCAP_BUFFER_SIZE 16
#define PCAP_FRAME_MAX 300
#define EAPOL_BUFFER_SIZE 8
#define EAPOL_FRAME_MAX 256

#define ANALIZADOR_DWELL_MS 300
#define ANALIZADOR_MUESTRAS 48

struct PaqueteCrudoWiFi {
    uint8_t datos[PCAP_FRAME_MAX];
    uint16_t len;
    uint32_t ts_sec;
    uint32_t ts_usec;
};

struct PaqueteEAPOL {
    uint8_t datos[EAPOL_FRAME_MAX];
    uint16_t len;
    uint8_t bssid[6];
    uint8_t stmac[6];
};

struct AnomaliaWiFi {
    unsigned long momento;
    uint8_t origen[6];
    uint8_t destino[6];
    int canal;
    int rssi;
    bool esDisassoc;
};

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

    void iniciarConteoClientes(const uint8_t* bssid, int canal);
    void update();
    bool conteoEnProgreso();
    unsigned long tiempoRestanteConteo();
    ClienteInfo* getClientes(int& count);

    void setLED(bool state);

    void getAnomaliasStats(unsigned long& deauth, unsigned long& disassoc);
    AnomaliaWiFi* getAnomalias(int& count);

    void activarPCAP(bool activo) { pcapActivo = activo; }
    bool pcapEstaActivo() const { return pcapActivo; }
    unsigned long getPCAPCapturados() const { return pcapCapturados; }
    bool sacarPaquetePCAP(PaqueteCrudoWiFi& out);

    void activarEAPOL(bool activo) { eapolActivo = activo; }
    bool eapolEstaActivo() const { return eapolActivo; }
    unsigned long getEAPOLCapturados() const { return eapolCapturados; }
    unsigned long getHandshakesArmados() const { return handshakesArmados; }
    bool sacarPaqueteEAPOL(PaqueteEAPOL& out);

    void iniciarAnalizadorCanales();
    void detenerAnalizadorCanales();
    void setCanalAnalizador(int canal);
    bool analizadorEstaActivo() const { return analizadorActivo; }
    int getCanalActualAnalizador() const { return canalActualAnalizador; }

    void getHistorialAnalizador(uint16_t* outBeacons, uint16_t* outDeauths, uint16_t* outProbes, int n);

    // Callback de promiscuo expuesto como publico para que Deauther
    // pueda re-armarlo si el driver lo pierde (ver
    // Deauther::rearmarCallbackPromiscuo). Sigue siendo static.
    static void promiscuousCallbackPublico(void* buf, wifi_promiscuous_pkt_type_t type);

private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void actualizarCliente(uint8_t* mac, int rssi, uint8_t* bssid, int canal);
    void registrarAnomalia(bool esDisassoc, uint8_t* origen, uint8_t* destino, int canal, int rssi);
    void empujarPCAP(const uint8_t* frame, uint16_t len);
    void empujarEAPOL(const uint8_t* frame, uint16_t len);

    void finalizarConteoClientes();

    ClienteInfo clientes[MAX_CLIENTES];
    int numClientes = 0;
    uint8_t bssidObjetivo[6];
    int canalObjetivo = 0;
    bool escuchandoClientes = false;
    unsigned long conteoInicio = 0;

    AnomaliaWiFi anomalias[MAX_ANOMALIAS_RAM];
    int numAnomalias = 0;
    unsigned long contadorDeauth = 0;
    unsigned long contadorDisassoc = 0;

    AnomaliaWiFi anomaliasSnapshot[MAX_ANOMALIAS_RAM];
    int numAnomaliasSnapshot = 0;
    ClienteInfo clientesSnapshot[MAX_CLIENTES];
    int numClientesSnapshot = 0;

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    PaqueteCrudoWiFi pcapBuffer[PCAP_BUFFER_SIZE];
    int pcapEscribirIdx = 0;
    int pcapLeerIdx = 0;
    int pcapPendientes = 0;
    bool pcapActivo = false;
    unsigned long pcapCapturados = 0;

    PaqueteEAPOL eapolBuffer[EAPOL_BUFFER_SIZE];
    int eapolEscribirIdx = 0;
    int eapolLeerIdx = 0;
    int eapolPendientes = 0;
    bool eapolActivo = false;
    unsigned long eapolCapturados = 0;
    unsigned long handshakesArmados = 0;

    bool analizadorActivo = false;
    int canalActualAnalizador = 6;
    unsigned long analizadorUltimoMuestreo = 0;

    uint16_t accBeacons = 0;
    uint16_t accDeauths = 0;
    uint16_t accProbes = 0;

    uint16_t histBeacons[ANALIZADOR_MUESTRAS] = {0};
    uint16_t histDeauths[ANALIZADOR_MUESTRAS] = {0};
    uint16_t histProbes[ANALIZADOR_MUESTRAS] = {0};

    static WiFiScanner* instance;
};

#endif
