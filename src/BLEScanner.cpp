#include "BLEScanner.h"
#include "BLE_DB.h"

// La propia clase de NimBLE que recibe cada advertisement -
// NimBLEAdvertisedDeviceCallbacks::onResult() corre en la tarea del
// host Bluetooth (FreeRTOS aparte, igual que promiscuousCallback() de
// WiFiScanner corre en la tarea del driver WiFi), nunca en loop(). Por
// eso lo unico que hace es copiar unos pocos bytes y pasarselos a
// BLEScanner::onDispositivoVisto() (que SI toma el mux antes de tocar
// el array compartido) - nada de trabajo pesado ni de tocar la SD
// desde aca.
class RadarCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
    explicit RadarCallbacks(BLEScanner* duenio) : duenio(duenio) {}

    void onResult(NimBLEAdvertisedDevice* dispositivo) override {
        uint8_t mac[6] = {0};
        // NimBLEAddress::toString() da "xx:xx:xx:xx:xx:xx" en
        // minuscula - parsearlo con sscanf es mas simple y mas estable
        // entre versiones de la libreria que ir a buscar el puntero
        // nativo de la direccion.
        sscanf(dispositivo->getAddress().toString().c_str(),
               "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

        char nombre[24] = "";
        if (dispositivo->haveName()) {
            std::string n = dispositivo->getName();
            strncpy(nombre, n.c_str(), sizeof(nombre) - 1);
            // strncpy no garantiza el '\0' final si el nombre ocupa
            // los 23 bytes enteros - hoy no pasa nada porque este
            // buffer arranca en cero y nunca se reusa entre llamadas,
            // pero lo dejamos explicito para no depender de eso.
            nombre[sizeof(nombre) - 1] = '\0';
        }

        // Manufacturer Specific Data: los primeros 2 bytes son el
        // Company ID (little-endian) que asigna el Bluetooth SIG -
        // formato publico, el mismo que usa cualquier implementacion
        // de BLE (ver BLE_DB.h). El resto del contenido es especifico
        // de cada fabricante; lo unico que este firmware interpreta
        // es el formato de "Find My" de Apple (ver comentario largo
        // en BLEScanner.h): Company ID 0x004C seguido de un byte de
        // tipo 0x12, documentado publicamente por Apple en su "Find My
        // Network Accessory Specification" para quien fabrique
        // accesorios compatibles - no es un formato privado ni
        // obtenido por ingenieria inversa de otro proyecto.
        uint16_t companyId = 0xFFFF;
        bool esFindMyApple = false;
        if (dispositivo->haveManufacturerData()) {
            std::string datos = dispositivo->getManufacturerData();
            if (datos.length() >= 2) {
                companyId = (uint8_t)datos[0] | ((uint8_t)datos[1] << 8);
                if (companyId == 0x004C && datos.length() >= 3 &&
                    (uint8_t)datos[2] == 0x12) {
                    esFindMyApple = true;
                }
            }
        }

        duenio->onDispositivoVisto(mac, dispositivo->getRSSI(), nombre,
                                    companyId, esFindMyApple);
    }

private:
    BLEScanner* duenio;
};

// NimBLEScan::start() sin esta funcion se queda esperando, bloqueado,
// a que el escaneo termine solo para recien ahi devolver el control -
// con duracion 0 (continuo) eso nunca pasa, asi que begin() se
// colgaba para siempre aca (confirmado en el log: se congelaba justo
// despues de "PCAP listo", antes de GPS o WiFi). Pasandole ESTA
// funcion como "scanCompleteCB", start() toma el otro camino interno
// (no bloqueante) y vuelve enseguida - en uso normal ni se llega a
// invocar, porque un escaneo continuo no "termina" nunca por su
// cuenta.
static void onScanCompleto(NimBLEScanResults resultados) {}

void BLEScanner::begin() {
    NimBLEDevice::init("");

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new RadarCallbacks(this));

    // Pasivo: nunca se manda un "scan request" pidiendole mas datos a
    // quien anuncia - solo se escucha lo que ya sale al aire por su
    // cuenta. Mismo espiritu que el escaneo WiFi: recepcion pura, cero
    // transmision de este lado.
    pScan->setActiveScan(false);
    pScan->setInterval(100);
    pScan->setWindow(99);

    // Duracion 0 = escaneo continuo, para que RADAR funcione igual que
    // el monitor de deauth/disassoc de WiFi: corre solo de fondo desde
    // el arranque, sin que haga falta prender nada a mano para que
    // empiece a sumar dispositivos.
    pScan->start(0, onScanCompleto, false);
}

void BLEScanner::onDispositivoVisto(const uint8_t* mac, int rssi, const char* nombre,
                                     uint16_t companyId, bool esFindMyApple) {
    portENTER_CRITICAL(&mux);

    int idx = -1;
    for (int i = 0; i < numDispositivos; i++) {
        if (memcmp(dispositivos[i].mac, mac, 6) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1 && numDispositivos < MAX_DISPOSITIVOS_BLE) {
        idx = numDispositivos++;
    } else if (idx == -1) {
        // Array lleno: en vez de descartar el dispositivo nuevo,
        // reemplaza al que hace mas tiempo no se escucha - asi la
        // lista no se queda pegada para siempre con los primeros 12
        // que aparecieron apenas se prendio el radar.
        unsigned long masViejo = dispositivos[0].ultimaVez;
        idx = 0;
        for (int i = 1; i < numDispositivos; i++) {
            if (dispositivos[i].ultimaVez < masViejo) {
                masViejo = dispositivos[i].ultimaVez;
                idx = i;
            }
        }
        memset(&dispositivos[idx], 0, sizeof(DispositivoBLE));
    }

    bool esNuevo = !dispositivos[idx].esFindMyApple && esFindMyApple;
    bool esPrimeraVez = (dispositivos[idx].ultimaVez == 0);

    memcpy(dispositivos[idx].mac, mac, 6);
    dispositivos[idx].rssi = rssi;
    strncpy(dispositivos[idx].nombre, nombre, sizeof(dispositivos[idx].nombre) - 1);
    // Mismo motivo que en RadarCallbacks::onResult(): este slot es
    // persistente (se reusa dispositivo tras dispositivo cuando el
    // array se llena), asi que aca si vale la pena el explicito en
    // vez de confiar en que quedo en cero de una vez anterior.
    dispositivos[idx].nombre[sizeof(dispositivos[idx].nombre) - 1] = '\0';
    dispositivos[idx].companyId = companyId;
    dispositivos[idx].esFindMyApple = dispositivos[idx].esFindMyApple || esFindMyApple;
    dispositivos[idx].ultimaVez = millis();

    if (esPrimeraVez) totalVistos++;
    if (esNuevo) totalFindMyApple++;

    portEXIT_CRITICAL(&mux);
}

DispositivoBLE* BLEScanner::getDispositivos(int& count) {
    portENTER_CRITICAL(&mux);
    numDispositivosSnapshot = numDispositivos;
    memcpy(dispositivosSnapshot, dispositivos, sizeof(DispositivoBLE) * numDispositivos);
    portEXIT_CRITICAL(&mux);
    count = numDispositivosSnapshot;
    return dispositivosSnapshot;
}

void BLEScanner::getStats(unsigned long& total, unsigned long& findMyApple) {
    portENTER_CRITICAL(&mux);
    total = totalVistos;
    findMyApple = totalFindMyApple;
    portEXIT_CRITICAL(&mux);
}
