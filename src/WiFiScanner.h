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
#define PCAP_FRAME_MAX 128

// Copia cruda de UN frame de management (beacon, probe req/resp,
// deauth o disassoc - ver el filtro en promiscuousCallback()) para
// pasarla de la tarea del driver WiFi al loop() principal, que es
// quien de verdad la escribe a la SD via PCAPWriter. Si el frame es
// mas largo que PCAP_FRAME_MAX se trunca (quedan intactos el header
// 802.11 y el SSID/parametros del principio, que es lo que importa
// para abrir el pcap en Wireshark - el resto de un beacon largo son
// tags opcionales que no hacen falta para esto).
struct PaqueteCrudoWiFi {
    uint8_t datos[PCAP_FRAME_MAX];
    uint16_t len;
    uint32_t ts_sec;
    uint32_t ts_usec;
};

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
    // BSSID puntual. Es pura recepcion (modo promiscuo), en ningun
    // momento se transmite nada.
    //
    // Antes esto era una sola funcion (contarClientes) que se quedaba
    // 3 segundos enteros en un "while + delay(10)" bloqueando TODO el
    // firmware mientras tanto - ni el touch, ni la mascota, ni el
    // escaneo normal respondian en ese rato. Nunca llego a usarse
    // desde ningun boton por esa misma razon. Ahora son 3 pasos no
    // bloqueantes: iniciarConteoClientes() arranca y vuelve al toque,
    // update() hay que llamarlo en cada vuelta del loop() (corta solo
    // a los 3 segundos), y conteoEnProgreso()/tiempoRestanteConteo()
    // son para que la pantalla pueda mostrar un "Escuchando... 2s".
    void iniciarConteoClientes(const uint8_t* bssid, int canal);
    void update();
    bool conteoEnProgreso();
    unsigned long tiempoRestanteConteo();
    ClienteInfo* getClientes(int& count);

    void setLED(bool state);

    // Monitor pasivo de deauth/disassoc: corre solo, todo el tiempo
    // que el radio esta en modo promiscuo (o sea, casi siempre salvo
    // el ratito de un scan activo), sin que haga falta prender nada
    // a mano. getAnomaliasStats da los totales acumulados desde el
    // arranque, getAnomalias da los ultimos vistos para listarlos.
    void getAnomaliasStats(unsigned long& deauth, unsigned long& disassoc);
    AnomaliaWiFi* getAnomalias(int& count);

    // Captura cruda para PCAP (beacons, probe req/resp, deauth y
    // disassoc - frames de management, nada de datos ni EAPOL/
    // handshakes). Apagada por defecto: activarPCAP(true) la prende,
    // pero quien de verdad escribe a la SD es main.cpp (loop()), no
    // este archivo - ver el comentario largo de pcapBuffer mas abajo
    // de por que escribir a la SD desde ACA seria peligroso.
    //
    // pcapActivo es el UNICO campo de esta clase que se lee/escribe
    // SIN el mux (a proposito, a diferencia de todo lo demas de aca
    // abajo): es un simple bool, un solo lugar lo escribe (el toque en
    // Monitor, via esta funcion) y un solo lugar lo lee
    // (promiscuousCallback), y en el peor caso de carrera se captura o
    // se descarta UN frame de mas justo en el instante del toque - no
    // hay forma de que quede un valor "a medio escribir" como si
    // pasaria con un struct de varios campos.
    void activarPCAP(bool activo) { pcapActivo = activo; }
    bool pcapEstaActivo() const { return pcapActivo; }
    unsigned long getPCAPCapturados() const { return pcapCapturados; }
    bool sacarPaquetePCAP(PaqueteCrudoWiFi& out);

private:
    static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void actualizarCliente(uint8_t* mac, int rssi, uint8_t* bssid, int canal);
    void registrarAnomalia(bool esDisassoc, uint8_t* origen, uint8_t* destino, int canal, int rssi);
    void empujarPCAP(const uint8_t* frame, uint16_t len);

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

    // Copias "congeladas" que devuelven getAnomalias()/getClientes(),
    // tomadas bajo lock - ver el comentario largo en el .cpp (encima
    // de "mux") de por que hace falta esto y no alcanza con devolver
    // un puntero a clientes[]/anomalias[] directamente.
    AnomaliaWiFi anomaliasSnapshot[MAX_ANOMALIAS_RAM];
    int numAnomaliasSnapshot = 0;
    ClienteInfo clientesSnapshot[MAX_CLIENTES];
    int numClientesSnapshot = 0;

    // Protege todo lo de arriba (clientes/numClientes/anomalias/
    // contadores/escuchandoClientes): promiscuousCallback() corre en
    // la tarea interna del driver WiFi, en paralelo con loop() - sin
    // esto, una pantalla podria leer un MAC o un contador a medio
    // escribir justo cuando llega un paquete.
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    // Buffer circular chico para la captura cruda de PCAP.
    // empujarPCAP() (llamado desde promiscuousCallback(), tarea del
    // driver WiFi) escribe en pcapEscribirIdx bajo el mux de arriba;
    // sacarPaquetePCAP() (llamado desde loop(), tarea principal) lee
    // de pcapLeerIdx, tambien bajo el mux. Esto es justo el mismo
    // patron "snapshot bajo lock" que ya usan getClientes()/
    // getAnomalias() mas arriba, nada mas que en vez de copiar TODO el
    // array de una, como son muchos paquetes chiquitos por segundo,
    // se van sacando de a uno.
    //
    // El motivo de que este buffer exista: escribir a la SD
    // DIRECTAMENTE desde promiscuousCallback() (la tarea del driver
    // WiFi) podria chocar con una escritura a la SD que este haciendo
    // el loop() principal en simultaneo (wardriving, eventos,
    // aprendizaje) - dos tareas distintas escribiendole a la misma
    // tarjeta SPI al mismo tiempo es tierra de corrupcion de
    // filesystem, no solo de un dato mal leido. Por eso
    // promiscuousCallback() solo copia bytes a RAM (rapido, no toca
    // SPI), y es loop() el que drena este buffer y llama a
    // PCAPWriter::writePacket() de a un paquete por vuelta, en el
    // mismo lugar (la tarea principal) donde ya se hacen todas las
    // demas escrituras a la SD del firmware.
    PaqueteCrudoWiFi pcapBuffer[PCAP_BUFFER_SIZE];
    int pcapEscribirIdx = 0;
    int pcapLeerIdx = 0;
    int pcapPendientes = 0;
    bool pcapActivo = false;
    unsigned long pcapCapturados = 0; // total historico, para mostrar en pantalla

    static WiFiScanner* instance;
};

#endif
