#include "HandshakeWriter.h"
#include <SD.h>

void HandshakeWriter::hex(char* out, const uint8_t* in, uint16_t n) {
    static const char d[] = "0123456789abcdef";
    for (uint16_t i = 0; i < n; i++) {
        out[i * 2]     = d[in[i] >> 4];
        out[i * 2 + 1] = d[in[i] & 0xF];
    }
    out[n * 2] = '\0';
}

bool HandshakeWriter::begin() {
    // Sin SD.end(): el montaje ya lo hizo pcap.begin() en setup().
    sprintf(path, "/handshakes_%lu.txt", millis());
    listo = true;
    Serial.printf("Handshakes: %s\n", path);
    return true;
}

bool HandshakeWriter::write(const uint8_t* bssid, const uint8_t* stmac,
                            const uint8_t* anonce, const uint8_t* snonce,
                            const uint8_t* mic, const uint8_t* eapol, uint16_t eapolLen) {
    if (!listo) return false;

    // WPA*02*MIC*BSSID*STMAC*ESSID_HEX*ANONCE*SNONCE*EAPOL*AP_MAC
    // hashcat 22000: el ESSID puede ir vacio ("00") si no se conoce;
    // aca no lo tenemos a mano en el callback, va vacio y hashcat lo
    // resuelve con el diccionario igual.
    char micHex[33], bssidHex[13], stmacHex[13], anonceHex[65], snonceHex[65];
    hex(micHex, mic, 16);
    hex(bssidHex, bssid, 6);
    hex(stmacHex, stmac, 6);
    hex(anonceHex, anonce, 32);
    hex(snonceHex, snonce, 32);

    File f = SD.open(path, FILE_APPEND);
    if (!f) return false;

    f.printf("WPA*02*%s*%s*%s*00*%s*%s*", micHex, bssidHex, stmacHex, anonceHex, snonceHex);
    for (uint16_t i = 0; i < eapolLen; i++) f.printf("%02x", eapol[i]);
    f.printf("*%s\n", bssidHex);
    f.close();

    escritos++;
    return true;
}
