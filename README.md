# CYD Wardriver

Firmware para convertir una placa "Cheap Yellow Display" (ESP32-2432S02x
y variantes) en un compañero de wardriving con mascota estilo tamagotchi:
escanea redes WiFi de forma pasiva, cuenta dispositivos por RSSI, guarda
todo con posición GPS y muestra el resultado en pantalla con una carita
que reacciona a lo que va encontrando.

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

| Placa | Estado |
|---|---|
| ESP32-2432S028 (2.8", resistivo) | Andando |
| ESP32-2432S024 (2.4", resistivo) | Pendiente (fase 2) |
| ESP32-3248S035R (3.5", resistivo) | Pendiente (fase 2) |
| ESP32-3248S035C (3.5", capacitivo) | Pendiente (fase 2) |
| ESP32-2424S012 (1.28" redonda) | Pendiente (fase 2) |
| Variantes 4.3"/5"/7" (ESP32-S3, panel RGB) | Fuera de alcance por ahora, otra arquitectura de pantalla |

La arquitectura ya está preparada para sumar placas sin tocar el resto
del código: cada una vive en `src/boards/Board_XXXX.h` y se selecciona
con un `-D CYD_BOARD_XXXX` en su propio `[env:]` de `platformio.ini`.

## Compilar

Sin entorno local de desarrollo:

1. Hacé fork o subí este repo a GitHub
2. El workflow en `.github/workflows/build.yml` compila solo
3. Descargá `firmware.bin` / `bootloader.bin` / `partitions.bin` del
   artifact `firmware-cyd-2432s028` y flasheá con esptool o con la
   herramienta web de espressif

Con PlatformIO local (si en algún momento tenés el entorno armado):

```
pio run -e cyd-2432s028 -t upload
```

## Estructura

```
src/
  boards/          perfiles de pines por placa
  main.cpp         loop principal y manejo de pantallas
  Mascota.*         la carita y sus animaciones
  WiFiScanner.*     escaneo pasivo + conteo de clientes
  Learning.*        memoria de redes ya vistas
  Wardriving.*      log CSV con posición GPS
  PCAPWriter.*      log PCAP de beacons/probes
  GPSModule.*       lectura del NEO-6M
  TouchDriver.h      interfaz común para touch resistivo/capacitivo
  TouchResistive.h   implementación XPT2046
```

## Hardware de referencia (2432S028)

- ESP32-2432S028 (WROOM-32, 4MB flash)
- Pantalla ILI9341 240x320 SPI
- Touch resistivo XPT2046
- Módulo GPS NEO-6M por Serial2 (RX=17, TX=16), no viene integrado en
  la placa
- Ranura microSD en el mismo bus SPI que la pantalla
