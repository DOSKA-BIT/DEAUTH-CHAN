#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <Arduino.h>
#include <NimBLEDevice.h>

#define MAX_DISPOSITIVOS_BLE 12

// Un dispositivo BLE visto en algun advertisement. Igual que con WiFi,
// esto es pura recepcion: nunca nos conectamos a nada ni mandamos un
// scan request (ver setActiveScan(false) en BLEScanner::begin()), solo
// escuchamos lo que cualquier dispositivo con Bluetooth ya esta
// gritando al aire por su cuenta.
struct DispositivoBLE {
    uint8_t mac[6];
    int rssi;
    char nombre[24];     // nombre anunciado, o "" si no mando ninguno
    uint16_t companyId;  // 0xFFFF = no traia manufacturer data
    bool esFindMyApple;  // ver comentario largo en el .cpp
    unsigned long ultimaVez;
};

// RADAR: escucha pasiva de Bluetooth Low Energy para mostrar que
// dispositivos BLE hay alrededor, y marcar los que coinciden con el
// formato publico de "Find My" de Apple (la red que usan los AirTag y
// cualquier accesorio compatible para avisar su ubicacion via otros
// iPhones cercanos, documentada por Apple en su "Find My Network
// Accessory Specification" para fabricantes de accesorios).
//
// Lo que esto SI hace: contar y listar dispositivos BLE cercanos con
// su señal, y avisar cuando alguno esta anunciando ese formato
// especifico de Find My. Lo que esto NO hace (y no finge hacer):
// identificar con certeza que un dispositivo puntual es "un AirTag" -
// cualquier accesorio de la red Find My (no solo AirTags) usa el
// mismo formato de broadcast, asi que lo mas honesto es mostrarlo como
// "dispositivo en la red Find My de Apple", no como "es un AirTag".
// Tampoco detecta camaras ni microfonos ocultos via Bluetooth (la
// gran mayoria ni siquiera usan BLE) ni nada de gafas tipo Ray-Ban
// Meta en particular - esas se anuncian como cualquier otro telefono/
// accesorio generico, no hay forma de distinguirlas de un vistazo al
// advertisement.
class BLEScanner {
public:
    void begin();

    DispositivoBLE* getDispositivos(int& count);
    void getStats(unsigned long& total, unsigned long& findMyApple);

    // Llamado desde el callback interno de NimBLE (otra tarea, ver
    // comentario largo en el .cpp) - publico porque NimBLE necesita
    // invocarlo desde una clase libre, no porque haya que llamarlo a
    // mano desde ningun otro lado del firmware.
    void onDispositivoVisto(const uint8_t* mac, int rssi, const char* nombre,
                             uint16_t companyId, bool esFindMyApple);

private:
    DispositivoBLE dispositivos[MAX_DISPOSITIVOS_BLE];
    int numDispositivos = 0;
    unsigned long totalVistos = 0;
    unsigned long totalFindMyApple = 0;

    DispositivoBLE dispositivosSnapshot[MAX_DISPOSITIVOS_BLE];
    int numDispositivosSnapshot = 0;

    // Mismo patron que el "mux" de WiFiScanner: el callback de NimBLE
    // corre en la tarea del host Bluetooth, en paralelo con loop(), asi
    // que cualquier lectura/escritura de dispositivos[] necesita esto.
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
};

#endif
