#include "HandshakeWriter.h"
#include "boards/BoardConfig.h"
#include <SD.h>

// Tamaño maximo de un frame EAPOL que vamos a recibir. Coincide con
// el buffer que usa WiFiScanner para PaqueteEAPOL (EAPOL_FRAME_MAX =
// 256). Si algun dia cambia ahi, cambiar aca tambien.
#define HANDHAKE_EAPOL_MAX 256

bool HandshakeWriter::begin() {
    SD.end();
    if (!SD.begin(SD_CS_PIN)) return false;
    sprintf(path, "/handshakes_%lu.hc22000", millis());
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    f.close();
    listo = true;
    return true;
}

void HandshakeWriter::hex(char* out, const uint8_t* in, uint16_t n) {
    static const char* d = "0123456789abcdef";
    for (uint16_t i = 0; i < n; i++) {
        out[i * 2]     = d[in[i] >> 4];
        out[i * 2 + 1] = d[in[i] & 0x0F];
    }
    out[n * 2] = '\0';
}

bool HandshakeWriter::write(const uint8_t* bssid, const uint8_t* stmac,
                             const uint8_t* anonce, const uint8_t* snonce,
                             const uint8_t* mic, const uint8_t* eapol, uint16_t eapolLen) {
    if (!listo) return false;
    if (eapolLen > HANDHAKE_EAPOL_MAX) return false;  // no entra en el buffer
    File f = SD.open(path, FILE_APPEND);
    if (!f) return false;

    char b[13], s[13], an[65], sn[65], m[33], e[HANDHAKE_EAPOL_MAX * 2 + 1];
    hex(b, bssid, 6);
    hex(s, stmac, 6);
    hex(an, anonce, 32);
    hex(sn, snonce, 32);
    hex(m, mic, 16);
    hex(e, eapol, eapolLen);

    f.printf("WPA*02*%s*%s*%s*%s*%s*%s*%s\n", m, b, s, sn, an, e, "ESSID");
    f.close();
    escritos++;
    return true;
}
