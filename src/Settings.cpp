#include "Settings.h"

// Namespace propio de NVS para estos ajustes - distinto del nombre
// "ajustes" que usa el objeto global (ver Globals.h) para no
// confundir a quien lea el log de Preferences ("cydwd_cfg" identifica
// el proyecto, no hace falta mas que eso).
static const char* NVS_NAMESPACE = "cydwd_cfg";

// Valores posibles de autosleep, en minutos. 0 = nunca. El default de
// fabrica (1 minuto) es el mismo que tenia el viejo SLEEP_TIMEOUT_MS
// fijo de config.h.
static const int PRESETS_AUTOSLEEP[] = {0, 1, 2, 5, 10, 15, 30};
static const int CANT_PRESETS_AUTOSLEEP = sizeof(PRESETS_AUTOSLEEP) / sizeof(PRESETS_AUTOSLEEP[0]);

// Busca en que posicion de la lista de presets cae un valor guardado.
// Si no matchea ninguno (valor viejo de una version anterior, flash
// corrupta, etc) cae al indice 1 (1 minuto) en vez de romperse.
static int indicePreset(int minutos) {
    for (int i = 0; i < CANT_PRESETS_AUTOSLEEP; i++) {
        if (PRESETS_AUTOSLEEP[i] == minutos) return i;
    }
    return 1;
}

void Settings::begin() {
    prefs.begin(NVS_NAMESPACE, false);

    brillo = prefs.getInt("brillo", 100);
    if (brillo < 10 || brillo > 100) brillo = 100;

    int guardado = prefs.getInt("autosleep", 1);
    autosleepMinutos = PRESETS_AUTOSLEEP[indicePreset(guardado)];
}

void Settings::setBrillo(int valor) {
    if (valor < 10) valor = 10;
    if (valor > 100) valor = 100;
    if (valor == brillo) return;
    brillo = valor;
    prefs.putInt("brillo", brillo);
}

void Settings::subirAutosleep() {
    int i = indicePreset(autosleepMinutos);
    if (i < CANT_PRESETS_AUTOSLEEP - 1) i++;
    autosleepMinutos = PRESETS_AUTOSLEEP[i];
    guardarAutosleep();
}

void Settings::bajarAutosleep() {
    int i = indicePreset(autosleepMinutos);
    if (i > 0) i--;
    autosleepMinutos = PRESETS_AUTOSLEEP[i];
    guardarAutosleep();
}

void Settings::guardarAutosleep() {
    prefs.putInt("autosleep", autosleepMinutos);
}
