#ifndef DEAUTHER_H
#define DEAUTHER_H

#include <Arduino.h>



class Deauther {
public:
    void begin();

    // broadcast=true manda a FF:FF:.. (tumba a todos los clientes del
    // AP). broadcast=false necesita un cliente concreto.
    bool iniciar(const uint8_t* bssid, int canal, const uint8_t* cliente, bool broadcast);
    void detener();
    void update();

    bool activo() const { return atacando; }
    unsigned long getEnviados() const { return enviados; }
    unsigned long getFallidos() const { return fallidos; }
    int getCanal() const { return canalObjetivo; }
    bool esBroadcast() const { return modoBroadcast; }

private:
    void construirTrama();
    bool asegurarPromiscuo();

    struct TramaDeauth {
        uint8_t  fc[2];
        uint16_t dur;
        uint8_t  dest[6];
        uint8_t  src[6];
        uint8_t  bssid[6];
        uint16_t seq;
        uint16_t reason;
    } __attribute__((packed));

    TramaDeauth trama;
    uint8_t bssidObjetivo[6] = {0};
    uint8_t clienteObjetivo[6] = {0};
    int canalObjetivo = 0;
    bool modoBroadcast = true;
    bool atacando = false;
    unsigned long ultimoEnvio = 0;
    unsigned long enviados = 0;
    unsigned long fallidos = 0;
};

#endif
