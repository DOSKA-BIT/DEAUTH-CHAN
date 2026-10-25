#ifndef BLE_DB_H
#define BLE_DB_H

#include <Arduino.h>
#include <pgmspace.h>

// Fabricante por Company ID de BLE (los 2 bytes que vienen al
// principio de los "Manufacturer Specific Data" de un advertisement -
// ver BLEScanner.cpp). Son los numeros que asigna oficialmente el
// Bluetooth SIG (bluetooth.com/specifications/assigned-numbers),
// publicos para cualquiera que implemente el protocolo - no es nada
// sacado de otro proyecto ni de ingenieria inversa.
//
// Esta tabla es chica a proposito: solo sirve para MOSTRAR de que
// marca es un dispositivo BLE cercano. La deteccion puntual de
// "esto es una red Find My de Apple" (la unica que este firmware
// afirma con confianza) vive aparte, en BLEScanner.cpp, porque ahi
// si hace falta mirar el contenido del payload y no solo el Company
// ID.
struct BLECompanyRecord {
    uint16_t companyId;
    const char* nombre;
};

static const BLECompanyRecord bleCompanyDB[] PROGMEM = {
    {0x004C, "Apple"},
    {0x0075, "Samsung"},
    {0x00E0, "Google"},
    {0x0006, "Microsoft"},
    {0x0171, "Amazon"},
    {0x038F, "Xiaomi"},
    {0x0157, "Anker"},
    {0x0059, "Nordic Semi"},
    {0x004F, "Sony"},
};

inline const char* buscarFabricanteBLE(uint16_t companyId) {
    for (int i = 0; i < (int)(sizeof(bleCompanyDB) / sizeof(bleCompanyDB[0])); i++) {
        BLECompanyRecord reg;
        memcpy_P(&reg, &bleCompanyDB[i], sizeof(BLECompanyRecord));
        if (reg.companyId == companyId) {
            return reg.nombre;
        }
    }
    return "Desconocido";
}

#endif
