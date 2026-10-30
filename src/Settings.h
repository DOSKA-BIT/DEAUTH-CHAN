#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>
#include "Idioma.h"

// Ajustes persistentes del usuario (brillo de pantalla, tiempo de
// autosleep, idioma de la interfaz), guardados en NVS via la libreria
// Preferences que ya trae el core de Arduino-ESP32 - no es una
// dependencia nueva, es la misma flash donde cualquier proyecto de
// ESP32 guarda cosas como credenciales WiFi. No toca la SD para nada,
// asi que estos valores se leen bien aunque la tarjeta no este
// puesta.
//
// Antes el tiempo de autosleep era un #define fijo en config.h
// (SLEEP_TIMEOUT_MS) y el brillo ni existia como concepto (el
// backlight era HIGH/LOW nomas, ver digitalWrite(TFT_BL_PIN, ...) en
// la version vieja de main.cpp) - ahora los dos (y el idioma) se
// eligen desde AJUSTES (ver ScreenAjustes.cpp) y sobreviven a un
// reinicio o un corte de bateria.
class Settings {
public:
    void begin();

    // 10-100 en pasos de 10. No se permite bajar de 10: un brillo de
    // 0 apagaria el backlight del todo, y sin nada en pantalla el
    // usuario no tendria forma de volver a subirlo con el dedo.
    int getBrillo() const { return brillo; }
    void setBrillo(int valor);

    // Minutos de inactividad antes de apagar la pantalla. 0 = nunca
    // (deshabilita el autosleep por completo). Se mueve entre una
    // lista fija de valores (ver PRESETS_AUTOSLEEP en el .cpp) en vez
    // de sumar/restar de a 1, para no necesitar 30 toques para llegar
    // de "nunca" a "30 minutos".
    int getAutosleepMinutos() const { return autosleepMinutos; }
    void subirAutosleep();
    void bajarAutosleep();

    // Idioma de la interfaz (ver Idioma.h/tr()). Default ESPANOL, el
    // mismo que tenia siempre el firmware antes de que existiera esto.
    IdiomaId getIdioma() const { return idioma; }
    void setIdioma(IdiomaId valor);

private:
    Preferences prefs;
    int brillo = 100;
    int autosleepMinutos = 1;
    IdiomaId idioma = IDIOMA_ES;

    void guardarAutosleep();
};

#endif
