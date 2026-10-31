# Política de seguridad

## Alcance

CYD Wardriver es un firmware de solo lectura: escanea WiFi y BLE en
modo pasivo, guarda lo que ve en una SD local y lo muestra en
pantalla. No expone ningún servicio de red (no hay servidor web, no
hay OTA, no hay ningún puerto escuchando), así que la superficie de
ataque remota es, en principio, nula - para explotar algo acá hacés
falta acceso físico al dispositivo (el puerto USB/Serial o la tarjeta
SD) o estar transmitiendo tramas 802.11/BLE maliciosas que el
firmware llegue a parsear.

Lo que SÍ es parte del alcance de seguridad de este proyecto:

- Parseo de tramas 802.11 (beacons, deauth, disassoc, probe) y de
  advertisements BLE recibidas por el aire - un bug acá podría
  crashear el dispositivo con una trama armada a propósito
  (denegación de servicio local), aunque dado que es firmware de
  hobby corriendo en un ESP32 sin aislamiento de procesos, no hay
  "escalada de privilegios" tradicional más allá de eso
- Escritura a la tarjeta SD (nombres de archivo, contenido de los
  CSV/PCAP) - un SSID armado a mano con caracteres raros no debería
  poder corromper esos archivos ni causar un buffer overflow

Lo que NO es parte del alcance (y no se van a tratar como
vulnerabilidad):

- Que el dispositivo pueda detectar/recibir tráfico ajeno - es
  exactamente su función, es pasivo por diseño
- Que alguien con acceso físico al dispositivo pueda reflashear otro
  firmware - cualquier ESP32 permite esto por USB sin protección
  adicional, no es un problema de este proyecto en particular

## Cómo reportar una vulnerabilidad

Abrí un [issue](https://github.com/DOSKA-BIT/deauth-chan/issues) en
este repositorio describiendo el problema. Como es un proyecto hobby
sin información personal de usuarios ni infraestructura propia
corriendo (todo vive en la placa de cada quien), no hay necesidad de
un canal privado separado - un issue público alcanza, salvo que el
reporte incluya un PoC que pueda usarse para hacer daño a terceros
antes de que exista un fix, en cuyo caso mejor avisar primero sin el
detalle completo y coordinar el timing de la publicación.

No hay programa de recompensas (bug bounty) - esto es un proyecto sin
fines de lucro.
