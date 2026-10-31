# CYD Wardriver

![Build](https://github.com/DOSKA-BIT/deauth-chan/actions/workflows/build.yml/badge.svg)
![Release](https://github.com/DOSKA-BIT/deauth-chan/actions/workflows/release.yml/badge.svg)
![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)

Firmware para convertir una placa "Cheap Yellow Display" (familia
ESP32-2432S0xx / 3248S035x) en un compañero de wardriving con mascota
estilo pwnagotchi: escanea redes WiFi y dispositivos BLE de forma
pasiva, cuenta dispositivos por RSSI, guarda todo con posición GPS y
muestra el resultado en pantalla con una carita de ojos grandes
estilo anime que reacciona a lo que va encontrando.

## Qué hace y qué no hace

Este proyecto **solo escucha**. No transmite tramas de deautenticación,
no clona puntos de acceso, no captura handshakes para crackeo offline,
y no tiene ninguna funcionalidad de ataque activo en ningún menú ni
modo oculto. Todo lo que hace es:

- Escaneo WiFi estándar (`WiFi.scanNetworks`) y modo promiscuo de
  **solo recepción** para contar dispositivos y detectar
  deauth/disassoc de terceros (para avisarte, no para generarlos)
- Escaneo BLE pasivo (nunca se conecta a nada ni manda un scan
  request) para listar dispositivos cercanos y avisar cuando alguno
  anuncia el formato público de "Find My" de Apple
- Registro de posición GPS + CSV/PCAP de lo que se ve en el aire
  (beacons y probe requests, nunca handshakes ni tráfico de datos)
- Una mascota en pantalla que cambia de humor según lo que va
  descubriendo

Si buscás una herramienta de auditoría con capacidad de ataque activo,
este no es el proyecto, y no lo va a ser en ningún agregado futuro.

## Herramientas disponibles

| Categoría | Herramienta | Qué hace |
|---|---|---|
| WIFI | Escanear | Busca redes WiFi cerca |
| WIFI | Stats | Memoria de redes ya vistas, con cuántas veces se vieron |
| WIFI | Conocidas | Redes guardadas, ordenables por señal |
| WIFI | Monitor | Detecta deauth/disassoc ajenos + captura PCAP opcional |
| WIFI | Eventos | Historial de la sesión actual |
| WIFI | Canales | Uso del espectro, canal por canal (barras) |
| WIFI | Analizador | Tráfico en vivo (beacons/deauths/probes) en un canal elegido a mano, gráfico de línea en el tiempo |
| WIFI | Duplicado | Mismo SSID con más de un BSSID en el último escaneo (posible gemelo malvado) |
| WARDRIVING | Wardrive | Guarda ubicación GPS + redes vistas en CSV, exportable a KML |
| RADAR | Radar | Dispositivos BLE cerca por señal, marca los que están en la red Find My de Apple |
| AJUSTES | Config | Datos de la placa, estado de la SD, heap/uptime |
| AJUSTES | Idioma | Español / English (en progreso: el chrome del menú ya traduce, el resto de las pantallas se está migrando) |
| AJUSTES | Brillo / Autosleep / Borrar redes | Brillo de pantalla por PWM, apagado automático configurable, borrado del historial de redes con doble confirmación |

## Estado del soporte multiplaca

| Placa | Pantalla | Touch | Estado |
|---|---|---|---|
| ESP32-2432S028 | 2.8" ILI9341 240x320 | Resistivo XPT2046, bus dedicado | Andando, pinout muy verificado |
| ESP32-2432S024 | 2.4" ILI9341 240x320 | Resistivo XPT2046, bus dedicado | Andando, mismo pinout que la 2432S028 (ver aviso en `Board_2432S024.h` sobre el backlight) |
| ESP32-3248S035R | 3.5" ST7796 320x480 | Resistivo XPT2046, bus compartido con la pantalla | Andando |
| ESP32-3248S035C | 3.5" ST7796 320x480 | Capacitivo GT911 (I2C) | Andando, con nota sobre el bug de fábrica del pin INT (ver `Board_3248S035C.h`) |
| ESP32-2424S012 (1.28" redonda) | GC9A01 | Capacitivo CST816 | Pendiente, sin pinout verificado todavía |
| Variantes 4.3"/5"/7" (ESP32-S3, panel RGB) | - | - | Fuera de alcance, otra arquitectura de pantalla (bus paralelo, no SPI) |

La arquitectura está pensada para sumar placas sin tocar el resto del
código: cada una vive en `src/boards/Board_XXXX.h`, se selecciona con
un `-D CYD_BOARD_XXXX` en su propio `[env:]` de `platformio.ini`, y el
touch se resuelve solo entre tres implementaciones (`TouchResistive`,
`TouchResistiveSharedBus`, `TouchCapacitive`) según lo que declare el
perfil de la placa. Ver [CONTRIBUTING.md](CONTRIBUTING.md) si querés
sumar una placa nueva.

### Sobre las fuentes de los pinouts

Los pines de cada placa salen de documentación pública de la
comunidad CYD (Random Nerd Tutorials, el repo `rzeldent/esp32-smartdisplay`,
hilos de `Bodmer/TFT_eSPI` en GitHub y `esp3d.io`), no son inventados.
Aun así, estas son placas clonadas por distintos fabricantes chinos y
existen revisiones con variaciones menores (backlight en otro pin,
etc.) — si algo no prende a la primera, revisá el comentario en el
`Board_XXXX.h` correspondiente antes de asumir que hay un bug de
software.

### Aviso importante: SD + touch en las placas de bus dedicado

En la 2432S028 y la 2432S024, el touch tiene su propio bus SPI (VSPI)
separado del de la pantalla, pero la SD también usa ese mismo
periférico VSPI con otros pines. El ESP32 clásico solo tiene dos
buses SPI de uso general y la pantalla ya ocupa uno, así que SD y
touch se turnan el otro por necesidad de hardware. El código ya
resuelve esto (cada escritura a SD reclama el bus, y se lo devuelve al
touch después), pero si notás que el touch se queda sin responder justo
después de guardar algo, es ese reparto, no un bug.

## Instalar sin compilar nada (flasheo desde el navegador)

Si tu placa ya está soportada (ver tabla arriba) y solo querés
probarla, no hace falta instalar PlatformIO ni nada: conectá la placa
por USB a una compu con Chrome o Edge (necesita Web Serial, Firefox y
Safari no andan) y entrá a:

**[Página de instalación →](https://doska-bit.github.io/deauth-chan/)**

Elegí el botón de tu placa y seguí los pasos; el navegador se encarga
de todo (borra y flashea bootloader + particiones + firmware). Esa
página siempre instala el último [Release](https://github.com/DOSKA-BIT/deauth-chan/releases)
publicado.

## Compilar vos mismo

Sin entorno local de desarrollo:

1. Hacé fork o subí este repo a GitHub
2. El workflow en `.github/workflows/build.yml` compila las 4 placas
   en paralelo en cada push
3. Descargá el artifact `firmware-cyd-XXXX` que corresponda a tu placa
   y flasheá `firmware.bin` / `bootloader.bin` / `partitions.bin` con
   esptool o con la herramienta web de espressif

Con PlatformIO local (si en algún momento tenés el entorno armado):

```
pio run -e cyd-2432s028 -t upload
pio run -e cyd-2432s024 -t upload
pio run -e cyd-3248s035r -t upload
pio run -e cyd-3248s035c -t upload
```

## Estructura

```
src/
  boards/                  perfiles de pines por placa
  main.cpp                 loop principal y manejo de pantallas
  Menu.cpp                 chrome compartido + menu de categorias/herramientas
  Screen*.cpp              pantallas de cada categoria (WIFI, WARDRIVING, RADAR, AJUSTES)
  Mascota.*                la carita y sus animaciones
  MascotaCaras.h           bitmaps de la carita (generados por tools/generar_caras.py)
  WiFiScanner.*            escaneo pasivo + conteo de clientes + Analizador de canales
  BLEScanner.*             escaneo BLE pasivo (RADAR + deteccion de Find My)
  Learning.*               memoria de redes ya vistas
  Wardriving.*             log CSV con posición GPS
  PCAPWriter.*             log PCAP de beacons/probes
  GPSModule.*              lectura del NEO-6M
  Settings.*               ajustes persistentes (brillo, autosleep, idioma) en NVS
  Idioma.*                 sistema de traducciones (ES/EN)
  TouchDriver.h            interfaz común de touch
  TouchResistive.h         XPT2046 en bus dedicado (2432S028/S024)
  TouchResistiveSharedBus.h XPT2046 compartiendo bus con la pantalla (3248S035R)
  TouchCapacitive.h        GT911 por I2C (3248S035C)
```

## Hardware de referencia

- ESP32-WROOM-32 (4MB flash) en todas las variantes soportadas
- Módulo GPS NEO-6M por Serial2, no viene integrado en ninguna CYD —
  los pines exactos dependen de la placa, ver `Board_XXXX.h`
- Ranura microSD en el mismo bus SPI que ocupa el touch (ver aviso
  arriba)

## Contribuir

Ver [CONTRIBUTING.md](CONTRIBUTING.md) para sumar soporte de una placa
nueva o proponer una herramienta. Para reportar un problema de
seguridad, ver [SECURITY.md](SECURITY.md).

## Licencia

MIT - ver [LICENSE](LICENSE).
