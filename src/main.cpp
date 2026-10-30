#include "Globals.h"

// Este archivo quedo reducido a proposito (pedido del usuario: separar
// main.cpp en archivos por categoria sin cambiar como funciona nada).
// Lo que queda aca es lo que de verdad es "transversal" a todo el
// firmware y no pertenece a ninguna categoria en particular:
//   - Las definiciones de los objetos/variables globales (las
//     declaraciones extern viven en Globals.h, que todos los demas
//     archivos incluyen).
//   - setup() y loop().
//   - manejarToque(), que ahora es un dispatcher chico: resuelve los
//     dos botones fijos de abajo y, segun la pantalla activa, llama a
//     la funcion de toque de Menu.cpp o del Screen*.cpp que
//     corresponda.
// Todo el dibujado y la logica propia de cada categoria se movio a su
// propio archivo: Menu.cpp (chrome compartido + menu de 2 niveles),
// ScreenWifi.cpp, ScreenWardriving.cpp, ScreenRadar.cpp y
// ScreenAjustes.cpp.

TFT_eSPI tft = TFT_eSPI();

#if TOUCH_IS_CAPACITIVE
    TouchCapacitive touchDriver;
#elif TOUCH_SHARES_TFT_BUS
    TouchResistiveSharedBus touchDriver(&tft);
#else
    TouchResistive touchDriver;
#endif

Mascota mascota;
WiFiScanner scanner;
PCAPWriter pcap;
BLEScanner radar;
GPSModule gps;
Wardriving wardriver;
Learning aprendizaje;
EventLog eventos;
Settings ajustes;

RedInfo redes[MAX_REDES_ESCANEO];
int numRedes = 0;
bool escaneando = false;

// Escaneo automatico de fondo: antes WiFi solo escaneaba cuando
// tocabas el boton en ESCANEAR. Ahora, ademas, cada AUTO_SCAN_MS
// (mientras no haya un escaneo manual ni un conteo de clientes en
// curso, y la mascota no este dormida) se dispara un escaneo solo,
// sin que haga falta estar parado en esa pantalla - asi CANALES se ve
// "vivo" en vez de una foto fija, y "NUEVAS" en el encabezado puede
// subir mientras estas mirando otra herramienta. escaneoEsAutomatico
// le dice al bloque que procesa el resultado (ver loop() mas abajo) si
// este escaneo puntual vino de este timer o de que tocaste el boton,
// para no guardar en Wardriving un punto de ubicacion que vos no
// pediste.
const unsigned long AUTO_SCAN_MS = 45000;
unsigned long ultimoAutoScan = 0;
bool escaneoEsAutomatico = false;

// Cuantas veces aparecio cada canal (1-13) sumando TODOS los escaneos
// de la sesion (manuales + automaticos), no solo el ultimo - para
// poder mostrar en CANALES cual es el canal mas saturado "en
// general", no nada mas en la foto de este momento. Vive en RAM nomas
// (como los contadores de anomalias de WiFiScanner): se resetea en
// cada reinicio, y eso esta bien, no es un dato que haga falta
// recordar de una sesion a la otra.
int conteoAcumuladoCanales[13] = {0};
int escaneosAcumulados = 0;

// Orden de la lista de CONOCIDAS: 0=senial, 1=veces vista, 2=nombre.
// Se cambia tocando el renglon de arriba de esa pantalla (ver
// dibujarPantallaConocidas() en ScreenWifi.cpp y manejarToqueWifi(),
// caso SCREEN_CONOCIDAS). NO hay un orden por "ultima vez vista": ese
// dato se guarda con millis() de CADA sesion, que arranca de cero en
// cada reinicio, asi que comparar el "ultimaVez" de una red guardada
// hace rato contra el de una vista recien en ESTA sesion no diria
// nada real.
int ordenConocidas = 0;
const int ALTO_BOTON_ORDEN = 18;

// Analizador de canales: escala de X (cuantas muestras del historial
// entran en pantalla - mas alto = mas "zoom" en el tiempo reciente,
// menos muestras visibles pero cada una mas ancha) y de Y (cuanto
// vale en pixeles una unidad de conteo - mas alto = picos chicos se
// ven mas grandes). Arrancan en valores razonables para que el
// grafico se vea bien apenas se entra, se ajustan con los botones +/-
// de la pantalla (ver dibujarPantallaAnalizador() y manejarToqueWifi(),
// caso SCREEN_ANALIZADOR, ambas en ScreenWifi.cpp).
int analizadorEscalaX = 1;
int analizadorEscalaY = 2;

// Filtro de que se guarda en wardriving.csv: 0 = todas las redes
// (comportamiento de siempre), 1 = solo las abiertas, 2 = solo las
// que tienen clave.
int filtroWardrive = 0;

// Resultado del ultimo intento de exportar el KML: -2 = todavia no se
// toco el boton en esta sesion, -1 = fallo (sin SD o sin CSV todavia),
// 0 o mas = cantidad de puntos que escribio.
int ultimoExportKML = -2;

// Mismo significado que ultimoExportKML de arriba, pero para el
// resumen de sesion que se exporta desde la pantalla de Eventos (ver
// exportarResumenSesion() en ScreenWifi.cpp): -2 = no se toco el boton
// todavia, -1 = fallo (sin SD), 0 o mas = cuantas lineas escribio.
int ultimoExportResumen = -2;

// Indice (dentro de redes[]) de la red cuyo conteo de clientes se
// esta mostrando/mostro ultimo en la pantalla de Escaneo. -1 = no hay
// ningun resultado de conteo para mostrar, se ve la lista de redes
// normal.
int redIndiceConteo = -1;

// Momento del ultimo toque real en la pantalla (no cuenta el toque
// que la despierta, ver mas abajo). Arranca en 0 y se pisa recien en
// setup(), asi el reloj de inactividad no empieza a correr hasta que
// el equipo ya esta andando.
unsigned long ultimoToque = 0;

// Avisa que el contenido de la pantalla activa (lista, stats,
// wardriving, config) quedo desactualizado y hay que redibujarlo.
// Antes esto se disparaba con cada parpadeo de la mascota sin
// importar que pantalla estuviera activa; ahora solo se prende
// cuando de verdad cambio algo que vale la pena mostrar.
bool contenidoSucio = true;

MenuScreen pantallaActual = SCREEN_MAIN;

// Destello al tocar una fila de la lista de Herramientas: como esto es
// tactil (no hay joystick ni cursor que se mueva solo), la unica forma
// de que la barra de "seleccionado" signifique algo real es mostrarla
// un instante justo donde tocaste, antes de entrar a esa pantalla - en
// vez de fingir un cursor fijo que no se mueve nunca. -1 = no hay
// ningun destello activo.
int filaDestellada = -1;
unsigned long momentoDestello = 0;
const unsigned long DESTELLO_MS = 120; // bien cortito, solo para que se note el toque
MenuScreen destinoPendiente = SCREEN_MAIN;

// Que categoria se esta mostrando en SCREEN_TOOLS_SUB (indice dentro
// de categorias[]). Se fija apenas se toca una fila en SCREEN_TOOLS,
// antes de que termine el destello - asi la pantalla ya sabe que
// lista filtrada dibujar en cuanto arranca la navegacion.
int categoriaSeleccionada = 0;

// Solo dos botones fijos abajo: uno para volver a la mascota y otro
// que es "Herramientas" o "Atras" segun donde estemos parados (ver
// esPantallaHoja() en Menu.cpp y el comentario largo en
// manejarToque(), mas abajo). Agregar una herramienta nueva el dia de
// manana es sumar una fila en la lista (herramientas[] mas abajo), no
// pelear por lugar en la barra de abajo.
Boton menuBotones[2];

// Las 4 categorias del submenu de Herramientas. Son pocas y fijas,
// asi que van en su propio array chico en vez de inventarse aparte de
// herramientas[] - cada Herramienta de mas abajo apunta a una de
// estas por indice (campo "categoria"). El icono de cada categoria es
// el de su primera herramienta (no hay un set de iconos "de
// categoria" aparte - no hacia falta duplicar arte para esto).
// Los campos nombre/descripcion ahora son claves de traduccion
// (TextoId, ver Idioma.h) en vez del texto en si - ver el comentario
// largo junto a "struct Categoria" en Globals.h.
Categoria categorias[CANTIDAD_CATEGORIAS] = {
    {TXT_CAT_WIFI_NOMBRE,       TXT_CAT_WIFI_DESC,       COLOR_CAT_CYAN,   ICONO_ESCANEAR, 0, 0},
    {TXT_CAT_WARDRIVING_NOMBRE, TXT_CAT_WARDRIVING_DESC, COLOR_CAT_VIOLET, ICONO_WARDRIVE, 0, 0},
    {TXT_CAT_RADAR_NOMBRE,      TXT_CAT_RADAR_DESC,      COLOR_CAT_GREEN,  ICONO_RADAR,    0, 0},
    {TXT_CAT_AJUSTES_NOMBRE,    TXT_CAT_AJUSTES_DESC,    COLOR_CAT_BLUE,   ICONO_CONFIG,   0, 0},
};

// Una fila por herramienta en la pantalla de Herramientas: nombre,
// descripcion corta, a que categoria pertenece (para el submenu de
// arriba), a que pantalla lleva al tocarla, y su color + icono de
// categoria. Agregar una herramienta nueva es agregar una linea aca
// (y su pantalla propia en el Screen*.cpp de su categoria, y su icono
// en Iconos.h) - no hay que tocar la barra de botones para nada.
//
// Nombres en mayuscula a proposito: es parte de la estetica de lista
// de terminal que reemplaza a las tarjetas redondeadas de antes. El
// color de cada una es su categoria: cian = redes/descubrimiento,
// amarillo = stats/destacado, violeta = analisis/ubicacion, azul =
// configuracion (neutro), blanco = listas simples/informativas, rojo
// = alertas.
Herramienta herramientas[] = {
    {TXT_HERR_ESCANEAR_NOMBRE,   TXT_HERR_ESCANEAR_DESC,   CAT_WIFI,       SCREEN_SCAN,      COLOR_CAT_CYAN,   ICONO_ESCANEAR,  0, 0},
    {TXT_HERR_STATS_NOMBRE,      TXT_HERR_STATS_DESC,      CAT_WIFI,       SCREEN_STATS,     COLOR_CAT_YELLOW, ICONO_STATS,     0, 0},
    {TXT_HERR_CONOCIDAS_NOMBRE,  TXT_HERR_CONOCIDAS_DESC,  CAT_WIFI,       SCREEN_CONOCIDAS, COLOR_CAT_WHITE,  ICONO_CONOCIDAS, 0, 0},
    {TXT_HERR_MONITOR_NOMBRE,    TXT_HERR_MONITOR_DESC,    CAT_WIFI,       SCREEN_MONITOR,   COLOR_CAT_RED,    ICONO_MONITOR,   0, 0},
    {TXT_HERR_EVENTOS_NOMBRE,    TXT_HERR_EVENTOS_DESC,    CAT_WIFI,       SCREEN_EVENTOS,   COLOR_CAT_BLUE,   ICONO_EVENTOS,   0, 0},
    {TXT_HERR_CANALES_NOMBRE,    TXT_HERR_CANALES_DESC,    CAT_WIFI,       SCREEN_CANALES,   COLOR_CAT_YELLOW, ICONO_CANALES,   0, 0},
    {TXT_HERR_ANALIZADOR_NOMBRE, TXT_HERR_ANALIZADOR_DESC, CAT_WIFI,       SCREEN_ANALIZADOR,COLOR_CAT_YELLOW, ICONO_CANALES,   0, 0},
    {TXT_HERR_DUPLICADO_NOMBRE,  TXT_HERR_DUPLICADO_DESC,  CAT_WIFI,       SCREEN_DUPLICADOS,COLOR_CAT_RED,    ICONO_DUPLICADO, 0, 0},
    // Vacio a proposito (pedido explicito del usuario): sin logica
    // adentro, nada mas el boton, el icono y una pantalla en blanco -
    // ver dibujarPantallaGame() en ScreenWifi.cpp.
    {TXT_HERR_GAME_NOMBRE,       TXT_HERR_GAME_DESC,       CAT_WIFI,       SCREEN_GAME,      COLOR_CAT_WHITE,  ICONO_ESPADA,    0, 0},
    {TXT_HERR_WARDRIVE_NOMBRE,   TXT_HERR_WARDRIVE_DESC,   CAT_WARDRIVING, SCREEN_WARDRIVE,  COLOR_CAT_VIOLET, ICONO_WARDRIVE,  0, 0},
    {TXT_HERR_RADAR_NOMBRE,      TXT_HERR_RADAR_DESC,      CAT_RADAR,      SCREEN_RADAR,     COLOR_CAT_GREEN,  ICONO_RADAR,     0, 0},
    {TXT_HERR_CONFIG_NOMBRE,     TXT_HERR_CONFIG_DESC,     CAT_AJUSTES,    SCREEN_CONFIG,    COLOR_CAT_BLUE,   ICONO_CONFIG,    0, 0},
    // Pedido del usuario: el selector de idioma vive al lado de CONFIG,
    // dentro de la misma categoria AJUSTES (ver dibujarPantallaIdioma()/
    // manejarToqueIdioma() en ScreenAjustes.cpp). Reusa el icono de
    // CONFIG a proposito (mismo criterio pragmatico que ANALIZADOR
    // reusando el icono de CANALES) en vez de dibujar uno nuevo.
    {TXT_HERR_IDIOMA_NOMBRE,     TXT_HERR_IDIOMA_DESC,     CAT_AJUSTES,    SCREEN_IDIOMA,    COLOR_CAT_BLUE,   ICONO_CONFIG,    0, 0},
};
const int CANTIDAD_HERRAMIENTAS = sizeof(herramientas) / sizeof(herramientas[0]);

// Dispatcher de toque: primero resuelve los dos botones fijos de
// abajo (siempre activos, en cualquier pantalla), y despues deriva a
// la funcion de toque que corresponde segun la pantalla activa. Las
// pantallas sin nada tocable (STATS, CANALES, DUPLICADOS, RADAR,
// GAME) caen en el "default" y no hacen nada, igual que antes. CONFIG
// dejo de estar en ese grupo: ahora tiene sus propios controles
// (brillo, autosleep, borrar redes guardadas - ver ScreenAjustes.cpp).
void manejarToque(int x, int y) {
    for (int i = 0; i < 2; i++) {
        if (x >= menuBotones[i].x && x <= menuBotones[i].x + menuBotones[i].w &&
            y >= menuBotones[i].y && y <= menuBotones[i].y + menuBotones[i].h) {
            if (i == 0) {
                // Boton 0 = volver siempre a la mascota.
                pantallaActual = SCREEN_MAIN;
            } else if (esPantallaHoja(pantallaActual)) {
                // Boton 1 estando DENTRO de una herramienta (ESCANEAR,
                // STATS, etc): ahora funciona como "Atras" y vuelve un
                // solo nivel, a la lista de herramientas de ESA
                // categoria (categoriaSeleccionada no se toca, sigue
                // siendo la misma) - antes saltaba directo a la lista
                // de categorias y habia que buscar de nuevo donde
                // estabas. El texto del boton cambia a "Atras" en este
                // caso - ver dibujarMenuBotones() en Menu.cpp.
                pantallaActual = SCREEN_TOOLS_SUB;
            } else {
                // Desde Mascota, desde la lista de categorias o desde
                // la lista de herramientas de una categoria: un nivel
                // mas arriba siempre es la lista de categorias (no la
                // ultima herramienta que estaba abierta - asi es
                // predecible).
                pantallaActual = SCREEN_TOOLS;
            }
            return;
        }
    }

    switch (pantallaActual) {
        case SCREEN_TOOLS:
            manejarToqueCategorias(y);
            break;
        case SCREEN_TOOLS_SUB:
            manejarToqueHerramientasSub(y);
            break;
        case SCREEN_SCAN:
        case SCREEN_CONOCIDAS:
        case SCREEN_EVENTOS:
        case SCREEN_MONITOR:
        case SCREEN_ANALIZADOR:
            manejarToqueWifi(pantallaActual, x, y);
            break;
        case SCREEN_WARDRIVE:
            manejarToqueWardriving(x, y);
            break;
        case SCREEN_CONFIG:
            manejarToqueConfig(x, y);
            break;
        case SCREEN_IDIOMA:
            manejarToqueIdioma(x, y);
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("CYD Wardriver iniciando en %s\n", BOARD_NAME);

    // Backlight por PWM (antes era pinMode+digitalWrite a secas) para
    // poder variar el brillo desde AJUSTES. ajustes.begin() va ANTES
    // de prender el backlight para arrancar directo en el brillo que
    // el usuario dejo guardado la ultima vez, en vez de un flash a
    // full brillo seguido de un salto al valor guardado.
    ajustes.begin();
    ledcSetup(BACKLIGHT_LEDC_CHANNEL, BACKLIGHT_LEDC_FREQ_HZ, BACKLIGHT_LEDC_RES_BITS);
    ledcAttachPin(TFT_BL_PIN, BACKLIGHT_LEDC_CHANNEL);
    ledcWrite(BACKLIGHT_LEDC_CHANNEL, map(ajustes.getBrillo(), 0, 100, 0, 255));

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(COLOR_BG);

    touchDriver.begin();
    calcularLayoutMenu();
    calcularLayoutCategorias();
    mascota.init(&tft);
    ultimoToque = millis();

    if (pcap.begin("wardrive")) {
        Serial.println("PCAP listo");
    }

    // radar.begin() (Bluetooth) va ANTES que scanner.begin() (WiFi) a
    // proposito: los dos comparten el mismo radio 2.4GHz del ESP32, y
    // si WiFi ya lo prendio en modo promiscuo cuando Bluetooth intenta
    // inicializarse, esp_bt_controller_init() lo encuentra en un
    // estado que no puede manejar y aborta (ESP_ERR_INVALID_STATE,
    // visto en el log serie: "abort() was called" justo en
    // NimBLEDevice::init). Inicializar Bluetooth primero, con el radio
    // todavia "limpio", evita ese choque.
    radar.begin(); // RADAR: escaneo BLE pasivo, corre de fondo desde el arranque (ver BLEScanner.h)
    gps.begin();
    scanner.begin();
    wardriver.begin();
    aprendizaje.begin();
    eventos.begin();
    eventos.registrar("Inicio del sistema");

    #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
        // pcap.begin(), wardriver.begin() y aprendizaje.begin() tocan la
        // SD, que comparte el periferico VSPI con el touch en esta
        // placa (ver el comentario largo en Board_2432S028.h). Eso deja
        // el bus apuntando a los pines de la SD en vez de los del
        // touch, asi que hay que reclamarlo antes de que arranque el
        // loop() o el touch queda leyendo basura desde el primer toque.
        touchDriver.reclaimBus();
    #endif

    Serial.println("Setup completado");
}

void loop() {
    gps.update();
    mascota.update();
    scanner.update(); // corta solo el conteo de clientes a los 3s (ver WiFiScanner::update)

    // Drena el buffer de paquetes crudos de PCAP (si hay algo activado
    // en Monitor) y recien ACA los escribe a la SD, via
    // PCAPWriter::writePacket() - nunca desde la tarea de WiFi, que es
    // donde se llenan (ver el comentario largo de pcapBuffer en
    // WiFiScanner.h sobre por que). Un paquete por vuelta de loop()
    // alcanza de sobra para no acumular atraso en uso normal.
    PaqueteCrudoWiFi paquetePCAP;
    if (scanner.sacarPaquetePCAP(paquetePCAP)) {
        pcap.writePacket(paquetePCAP.datos, paquetePCAP.len, paquetePCAP.ts_sec, paquetePCAP.ts_usec);
    }

    // Modo sleep: si nadie toco la pantalla en el tiempo elegido desde
    // AJUSTES (0 = nunca, deshabilitado) y no hay un escaneo ni un
    // conteo de clientes activo, apagamos el backlight. No tocamos
    // nada de WiFi/SD/GPS -- el resto de los modulos sigue
    // funcionando igual, lo unico que cambia es que la pantalla deja
    // de consumir. Antes el tiempo era SLEEP_TIMEOUT_MS fijo en
    // config.h; ahora sale de ajustes.getAutosleepMinutos().
    unsigned long autosleepMs = (unsigned long)ajustes.getAutosleepMinutos() * 60000UL;
    if (autosleepMs > 0 && !escaneando && !scanner.conteoEnProgreso() && mascota.getEstado() != ESTADO_SLEEP &&
        millis() - ultimoToque > autosleepMs) {
        mascota.setEstado(ESTADO_SLEEP);
        ledcWrite(BACKLIGHT_LEDC_CHANNEL, 0);
        eventos.registrar("Durmiendo por inactividad", nullptr);
        #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
            // registrar() acaba de escribir en la SD, que en esta
            // placa remapea el bus VSPI hacia sus propios pines (igual
            // que cualquier otra escritura a SD). Si no se reclama aca
            // el proximo touchDriver.read() de este mismo loop() lee
            // basura del touch.
            touchDriver.reclaimBus();
        #endif
    }

    // ANTES esto pedia ademas "pantallaActual == SCREEN_SCAN" - bug
    // real que encontre repasando el codigo: si tocabas "Herramientas"
    // (o cualquier otra pantalla) MIENTRAS un escaneo estaba en curso,
    // esta linea nunca se ejecutaba (porque ya no estabas en
    // SCREEN_SCAN), asi que "escaneando" se quedaba en true PARA
    // SIEMPRE - y el toque de "Anda a ESCANEAR y volve aca" quedaba
    // mudo de ahi en mas, porque manejarToque() no dispara un escaneo
    // nuevo mientras escaneando ya esta en true. Sacar la condicion de
    // pantalla de aca tambien es lo que hace falta para que el
    // escaneo automatico de mas abajo funcione estando en CUALQUIER
    // pantalla, no solo en ESCANEAR.
    if (escaneando && scanner.isScanDone()) {
        scanner.getScanResults(redes, MAX_REDES_ESCANEO, numRedes);
        mascota.setRedesEncontradas(numRedes);
        escaneando = false;
        // El escaneo automatico no muestra el estado "ESCANEANDO" (el
        // usuario no lo pidio, no hace falta avisarle que esta pasando
        // salvo que encuentre algo nuevo - eso ya lo hace
        // incrementarRedesNuevas() mas abajo, poniendo CURIOSA).
        if (!escaneoEsAutomatico) {
            mascota.setEstado(ESTADO_IDLE);
        }

        GPSData pos = gps.getData();
        int nuevasEnEsteScan = 0;
        for (int i = 0; i < numRedes; i++) {
            // Wardriving (guardar ubicacion+red en la SD) sigue siendo
            // una accion deliberada del usuario - un escaneo de fondo
            // cada 45s no deberia llenarle la SD de puntos que nunca
            // pidio. Aprendizaje (Conocidas/redes nuevas) SI se
            // actualiza siempre: ese historial tiene que reflejar todo
            // lo que el radio escucho, sea de un escaneo manual o
            // automatico.
            if (!escaneoEsAutomatico) {
                wardriver.saveNetwork(redes[i], pos);
            }
            bool esNueva = aprendizaje.registrarAvistamiento(redes[i]);
            if (esNueva) {
                mascota.incrementarRedesNuevas();
                nuevasEnEsteScan++;
            }

            int ch = redes[i].canal;
            if (ch >= 1 && ch <= 13) conteoAcumuladoCanales[ch - 1]++;
        }
        escaneosAcumulados++;

        // Una sola escritura a la SD aca afuera del for: antes
        // registrarAvistamiento() guardaba en SD por cada red
        // individual, asi que un escaneo con 15 redes eran 15
        // remontajes de SD seguidos. Guardando una vez al final se
        // llega al mismo resultado sin la espera.
        if (numRedes > 0) {
            aprendizaje.guardarEnSD();
        }

        // Un solo registro de evento por escaneo (no uno por red, por
        // la misma razon de arriba): deja el resumen que de verdad
        // importa - cuantas redes vio y cuantas eran nuevas - sin
        // volver a escribir la SD veinte veces seguidas. Se marca
        // "Auto" para distinguirlo de un escaneo que de verdad tocaste
        // vos, al mirar el historial despues.
        char resumenEvento[48];
        snprintf(resumenEvento, sizeof(resumenEvento), "%s: %d redes (%d nuevas)",
                 escaneoEsAutomatico ? "Auto-escaneo" : "Escaneo", numRedes, nuevasEnEsteScan);
        eventos.registrar(resumenEvento, &pos);

        Serial.printf("Escaneo completado: %d redes\n", numRedes);

        #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
            // La SD y el touch comparten el periferico VSPI en esta
            // placa (ver el comentario largo en Board_2432S028.h). Cada
            // escritura a SD de arriba remapea el bus hacia sus propios
            // pines, asi que hay que recuperarlo para el touch antes de
            // seguir escuchando toques.
            touchDriver.reclaimBus();
        #endif

        escaneoEsAutomatico = false;
        contenidoSucio = true; // hay resultados nuevos para mostrar (en SCAN, CANALES o DUPLICADO)
    }

    // Dispara el escaneo de fondo (ver comentario largo arriba de
    // AUTO_SCAN_MS): solo si no hay ya un escaneo o un conteo de
    // clientes usando el radio, no estamos parados en ESCANEAR (ahi
    // el usuario tiene el control manual) y la mascota no esta
    // dormida (no tiene sentido gastar radio con la pantalla apagada
    // por inactividad).
    if (!escaneando && !scanner.conteoEnProgreso() && !scanner.analizadorEstaActivo() &&
        pantallaActual != SCREEN_SCAN && pantallaActual != SCREEN_ANALIZADOR &&
        mascota.getEstado() != ESTADO_SLEEP &&
        millis() - ultimoAutoScan >= AUTO_SCAN_MS) {
        ultimoAutoScan = millis();
        scanner.startScan();
        escaneando = true;
        escaneoEsAutomatico = true;
    }

    // Conteo de clientes no bloqueante: mientras dura, se repinta la
    // cuenta regresiva cada medio segundo; apenas termina (deja de
    // estar en progreso de una vuelta del loop a la otra) se marca
    // contenidoSucio una sola vez para que se vea el resultado final.
    {
        static bool conteoEnProgresoAnterior = false;
        bool conteoAhora = scanner.conteoEnProgreso();
        if (conteoEnProgresoAnterior && !conteoAhora) {
            contenidoSucio = true;
        }
        conteoEnProgresoAnterior = conteoAhora;

        if (pantallaActual == SCREEN_SCAN && conteoAhora) {
            static unsigned long ultimoRefrescoConteo = 0;
            unsigned long ahoraConteo = millis();
            if (ahoraConteo - ultimoRefrescoConteo >= 500) {
                ultimoRefrescoConteo = ahoraConteo;
                dibujarListaRedes();
            }
        }
    }

    // Si hay un destello de toque pendiente en Herramientas, se espera
    // sin bloquear (nada de delay()) a que pasen los DESTELLO_MS antes
    // de entrar de verdad a la pantalla elegida - asi da tiempo a que
    // se vea la fila invertida en vez de saltar directo.
    if (filaDestellada >= 0 && millis() - momentoDestello >= DESTELLO_MS) {
        filaDestellada = -1;
        pantallaActual = destinoPendiente;
        contenidoSucio = true;
    }

    static MenuScreen pantallaAnterior = (MenuScreen)-1;
    bool cambioPantalla = (pantallaActual != pantallaAnterior);
    if (cambioPantalla) {
        contenidoSucio = true;

        // El Analizador de canales prende/apaga su propia rueda de
        // canales justo al entrar/salir de esta pantalla puntual (ver
        // el comentario largo de ANALIZADOR_DWELL_MS en WiFiScanner.h)
        // - ni arranca solo de fondo ni se queda girando canales
        // despues de que te fuiste a otra pantalla.
        if (pantallaAnterior == SCREEN_ANALIZADOR) {
            scanner.detenerAnalizadorCanales();
        }
        if (pantallaActual == SCREEN_ANALIZADOR) {
            scanner.iniciarAnalizadorCanales();
        }
    }

    // La mascota se dibuja en la misma franja central que usan las
    // demas pantallas para su contenido (lista de redes, stats,
    // wardriving, config): no son dos capas separadas, es el mismo
    // pedazo de pantalla. Por eso antes, cuando a la mascota le tocaba
    // parpadear, se volvia a disparar TODO el bloque de abajo sin
    // importar en que pantalla estuvieramos - y en Wardrive/Config eso
    // significaba remontar la SD en cada parpadeo (de ahi la lentitud
    // y el parpadeo en esas pantallas, no solo en la principal). Ahora
    // la mascota solo se dibuja a si misma, y solo mientras estamos
    // parados en la pantalla principal; el resto de las pantallas se
    // redibuja una sola vez, cuando entras a ellas o cuando cambia
    // algo que de verdad hay que mostrar (como un escaneo terminado).
    bool enPantallaPrincipal = (pantallaActual == SCREEN_MAIN);
    // Dos chequeos separados a proposito: la cara grande anima sola
    // (bobbing, cambio de frame) y necesita redibujarse cada ~200ms
    // mientras estamos en la pantalla principal, pero el encabezado NO
    // tiene nada que se mueva solo - antes usaba el mismo chequeo que
    // la cara grande, y por eso la franja de arriba parpadeaba en
    // TODAS las pantallas cada vez que a la mascota le tocaba animar,
    // no solo cuando el texto que muestra cambiaba de verdad.
    bool mascotaCambio = mascota.necesitaRedibujar();
    bool encabezadoCambio = mascota.necesitaRedibujarEncabezado();

    if (cambioPantalla) {
        dibujarMenuBotones();
    }

    if (enPantallaPrincipal) {
        // Esta pantalla ES la mascota, no hay texto de bienvenida
        // compitiendo por el mismo espacio: antes se dibujaban los
        // dos en la misma franja, uno tapando al otro.
        if (cambioPantalla || mascotaCambio || encabezadoCambio) {
            // El borrado completo de la franja solo hace falta al
            // entrar a esta pantalla; en los redibujados de animacion
            // (mascotaCambio/encabezadoCambio solos) alcanza con que
            // la mascota limpie su propia zona, si no la pantalla
            // parpadea de mas.
            mascota.dibujar(cambioPantalla);
        }
    } else {
        // En el resto de las pantallas la mascota no tiene su cara
        // grande (ese lugar lo usa el contenido de la pantalla), pero
        // la franja de arriba con su estado se mantiene igual en
        // todos lados para no perderle la pista mientras mirás otra
        // cosa. Ojo: acá se chequea SOLO encabezadoCambio, no
        // mascotaCambio - si no, esta franja parpadearia en Escaneo,
        // Stats, Wardrive, etc cada vez que la cara grande (que ni se
        // ve en estas pantallas) tiene un tick de animacion.
        if (cambioPantalla || encabezadoCambio) {
            mascota.dibujarEncabezado();
        }
        if (contenidoSucio) {
            switch (pantallaActual) {
                case SCREEN_TOOLS:
                    dibujarPantallaCategorias();
                    break;
                case SCREEN_TOOLS_SUB:
                    dibujarPantallaHerramientasSub();
                    break;
                case SCREEN_SCAN:
                    dibujarListaRedes();
                    break;
                case SCREEN_STATS:
                    dibujarPantallaStats();
                    break;
                case SCREEN_WARDRIVE:
                    dibujarPantallaWardrive();
                    break;
                case SCREEN_CONFIG:
                    dibujarPantallaConfig();
                    break;
                case SCREEN_CONOCIDAS:
                    dibujarPantallaConocidas();
                    break;
                case SCREEN_EVENTOS:
                    dibujarPantallaEventos();
                    break;
                case SCREEN_MONITOR:
                    dibujarPantallaMonitor();
                    break;
                case SCREEN_CANALES:
                    dibujarPantallaCanales();
                    break;
                case SCREEN_ANALIZADOR:
                    dibujarPantallaAnalizador();
                    break;
                case SCREEN_DUPLICADOS:
                    dibujarPantallaDuplicados();
                    break;
                case SCREEN_RADAR:
                    dibujarPantallaRadar();
                    break;
                case SCREEN_GAME:
                    dibujarPantallaGame();
                    break;
                case SCREEN_IDIOMA:
                    dibujarPantallaIdioma();
                    break;
                default:
                    break;
            }
        }

        // Config muestra heap/uptime en vivo: se refresca solo ese
        // bloque de numeros cada 1s, sin pasar por contenidoSucio (que
        // volveria a dibujar todo el texto fijo de arriba) y sin tocar
        // la SD para nada - por eso es una funcion aparte y no un
        // vuelta a dibujarPantallaConfig() completa.
        if (pantallaActual == SCREEN_CONFIG) {
            static unsigned long ultimoRefrescoConfig = 0;
            unsigned long ahoraConfig = millis();
            if (cambioPantalla || ahoraConfig - ultimoRefrescoConfig >= 1000) {
                ultimoRefrescoConfig = ahoraConfig;
                if (!cambioPantalla) {
                    // Si acabamos de entrar a la pantalla ya se pinto
                    // completa arriba (contenidoSucio); aca solo evito
                    // un segundo redibujado inmediato del mismo cuadro.
                    actualizarDiagnosticoHardware();
                    // Mismo refresco de 1s: si "Borrar redes guardadas"
                    // quedo armado o mostrando "listo" y se vencio el
                    // tiempo, lo vuelve a su estado normal.
                    actualizarEstadoBorrado();
                }
            }
        }

        // Igual que en Config: el Monitor de deauth/disassoc se
        // repinta solo cada 1s mientras estamos parados en el, para
        // que una rafaga se note sin tener que salir y volver a
        // entrar a la pantalla. Esta pantalla no toca la SD para
        // nada (los contadores viven en RAM, dentro de WiFiScanner),
        // asi que ademas es mucho mas liviano que el refresco de
        // Config.
        if (pantallaActual == SCREEN_MONITOR) {
            static unsigned long ultimoRefrescoMonitor = 0;
            unsigned long ahoraMonitor = millis();
            if (cambioPantalla || ahoraMonitor - ultimoRefrescoMonitor >= 1000) {
                ultimoRefrescoMonitor = ahoraMonitor;
                if (!cambioPantalla) {
                    dibujarPantallaMonitor();
                }
            }
        }

        // Mismo patron que Monitor: RADAR tambien vive de datos que se
        // acumulan solos de fondo (el callback de BLE), asi que sin
        // esto la lista se quedaria congelada en lo que habia apenas
        // entraste a la pantalla hasta que la tocaras o salieras y
        // volvieras a entrar.
        if (pantallaActual == SCREEN_RADAR) {
            static unsigned long ultimoRefrescoRadar = 0;
            unsigned long ahoraRadar = millis();
            if (cambioPantalla || ahoraRadar - ultimoRefrescoRadar >= RADAR_REFRESCO_MS) {
                ultimoRefrescoRadar = ahoraRadar;
                if (!cambioPantalla) {
                    dibujarPantallaRadar();
                }
            }
        }

        // Mismo patron que RADAR/MONITOR: el Analizador vive de datos
        // que se acumulan solos de fondo (el callback de WiFi contando
        // trafico por canal) mas la rueda de canales que gira sola, asi
        // que sin esto la pantalla se quedaria congelada en la primera
        // foto hasta que la tocaras.
        if (pantallaActual == SCREEN_ANALIZADOR) {
            static unsigned long ultimoRefrescoAnalizador = 0;
            unsigned long ahoraAnalizador = millis();
            if (cambioPantalla || ahoraAnalizador - ultimoRefrescoAnalizador >= ANALIZADOR_DWELL_MS) {
                ultimoRefrescoAnalizador = ahoraAnalizador;
                if (!cambioPantalla) {
                    dibujarPantallaAnalizador();
                }
            }
        }
    }

    contenidoSucio = false;
    pantallaAnterior = pantallaActual;

    TouchPoint toque = touchDriver.read();
    if (toque.pressed) {
        ultimoToque = millis();

        if (mascota.getEstado() == ESTADO_SLEEP) {
            // El primer toque despues de dormir solo despierta: no
            // dispara ademas el boton o la zona de la pantalla que
            // haya debajo del dedo, para no activar algo sin querer
            // apenas se prende de nuevo la pantalla.
            ledcWrite(BACKLIGHT_LEDC_CHANNEL, map(ajustes.getBrillo(), 0, 100, 0, 255));
            mascota.setEstado(ESTADO_IDLE);
            contenidoSucio = true;
            eventos.registrar("Se desperto", nullptr);
            #if !TOUCH_IS_CAPACITIVE && !TOUCH_SHARES_TFT_BUS
                // Mismo motivo que en el bloque de "Durmiendo": esto
                // acaba de escribir en la SD, hay que reclamar el bus
                // antes de que se lea el touch de vuelta en la proxima
                // vuelta del loop().
                touchDriver.reclaimBus();
            #endif
        } else if (toque.x >= TFT_PANEL_WIDTH / 3 && toque.y >= CONTENT_TOP && toque.y <= CONTENT_TOP + alturaListado()
            && pantallaActual == SCREEN_MAIN) {
            mascota.tocar(toque.x, toque.y);
        } else {
            manejarToque(toque.x, toque.y);
        }
    }

    delay(10);
}
