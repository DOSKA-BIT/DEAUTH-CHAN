# CYD Wardriver

Firmware para convertir una placa "Cheap Yellow Display" (familia
ESP32-2432S0xx / 3248S035x) en un compañero de wardriving con mascota
estilo tamagotchi: escanea redes WiFi de forma pasiva, cuenta
dispositivos por RSSI, guarda todo con posición GPS y muestra el
resultado en pantalla con una carita que reacciona a lo que va
encontrando.

## Qué hace y qué no hace

Este proyecto **solo escucha**. No transmite tramas de deautenticación,
no clona puntos de acceso, no captura handshakes para crackeo offline.
Todo lo que hace es:

- Escaneo WiFi estándar (`WiFi.scanNetworks`)
- Conteo pasivo de cuántos dispositivos distintos se escuchan hablando
  con un AP puntual (modo promiscuo de solo recepción)
- Registro de posición GPS + CSV/PCAP de lo que se ve en el aire
  (beacons y probe requests, nunca handshakes ni tráfico de datos)
- Una mascota en pantalla que cambia de humor según lo que va
  descubriendo

Si buscás una herramienta de auditoría con capacidad de ataque activo,
este no es el proyecto.

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
perfil de la placa.

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

## Compilar

Sin entorno local de desarrollo:

1. Hacé fork o subí este repo a GitHub
2. El workflow en `.github/workflows/build.yml` compila las 4 placas
   en paralelo
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
  Mascota.*                la carita y sus animaciones
  WiFiScanner.*            escaneo pasivo + conteo de clientes
  Learning.*               memoria de redes ya vistas
  Wardriving.*             log CSV con posición GPS
  PCAPWriter.*             log PCAP de beacons/probes
  GPSModule.*              lectura del NEO-6M
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
