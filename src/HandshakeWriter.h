#ifndef HANDSHAKE_WRITER_H
#define HANDSHAKE_WRITER_H

#include <Arduino.h>

// Escribe handshakes WPA/WPA2 completos (M1+M2) en formato hashcat
// 22000 (linea WPA*02*...) a un .txt en la SD, para crackear en PC con
// hashcat -m 22000. No es un PCAPWriter: el .pcap de 802.11 crudo no
// lo lee hashcat, hace falta esta linea armada.
//
// A diferencia de PCAPWriter, este NO hace SD.end()/SD.begin() (el
// montaje ya lo dejo listo pcap.begin() en setup(), y un segundo
// SD.end() ahi desmontaria la tarjeta que el otro acaba de montar).
// Abre y cierra por linea escrita, que es una vez por handshake
// completo, no muchas por segundo.
class HandshakeWriter {
public:
    bool begin();
    // Escribe una linea WPA*02* completa. bssid/stmac en 6 bytes,
    // nonce en 32, mic en 16, eapol en crudo con su largo. Devuelve
    // false si no hay SD montada o falla la escritura.
    bool write(const uint8_t* bssid, const uint8_t* stmac,
               const uint8_t* anonce, const uint8_t* snonce,
               const uint8_t* mic, const uint8_t* eapol, uint16_t eapolLen);
    unsigned long getEscritos() const { return escritos; }

private:
    char path[64] = "";
    bool listo = false;
    unsigned long escritos = 0;
    void hex(char* out, const uint8_t* in, uint16_t n);
};

#endif
