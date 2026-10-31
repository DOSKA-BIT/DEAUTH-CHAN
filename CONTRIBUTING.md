# Contribuir a CYD Wardriver

Gracias por el interés. Esto es un proyecto hobby mantenido sin un
entorno local de desarrollo (todo se compila vía GitHub Actions), así
que el flujo de contribución está pensado alrededor de eso.

## Antes que nada: el alcance del proyecto

Este firmware es **estrictamente pasivo**: no transmite deauth, no
clona APs, no captura handshakes para crackeo offline, no tiene
ninguna funcionalidad de ataque activo. Un PR que agregue cualquiera
de eso (incluso "apagado por default", "con flag para habilitarlo", o
"solo para investigación") va a ser rechazado sin excepción. Si
estás buscando sumar capacidad de ataque activo, este no es el
proyecto para eso.

## Cómo reportar un bug

Abrí un issue con:

- Qué placa usás (una de la tabla del README, o el modelo exacto si
  es una no soportada todavía)
- Qué esperabas que pasara vs. qué pasó
- Si podés, el log de Serial (115200 baudios) del momento del problema

## Cómo proponer una herramienta nueva

Abrí un issue describiendo la idea antes de hacer el PR. Ayuda mucho
si incluís una foto de referencia si te inspiraste en otra interfaz
(Pwnagotchi, Marauder, Fennec, etc.) — varias herramientas de este
proyecto (el Analizador de canales, el restyle visual) pasaron por
varias vueltas hasta encontrarle la forma correcta, y una referencia
visual ahorra ida y vuelta.

## Cómo sumar soporte para una placa CYD nueva

La arquitectura ya está pensada para esto, no hace falta tocar el
resto del firmware:

1. Creá `src/boards/Board_TUPLACA.h` copiando la estructura de uno
   existente (`Board_2432S028.h` es el más simple, de touch
   resistivo con bus dedicado). Como mínimo necesita:
   - `TFT_PANEL_WIDTH` / `TFT_PANEL_HEIGHT` (resolución nativa)
   - `TOUCH_IS_CAPACITIVE` (0 o 1)
   - `TOUCH_SHARES_TFT_BUS` (si no es capacitivo: 1 si el touch usa
     el mismo bus SPI que la pantalla, 0 si tiene uno propio)
   - `TFT_BL_PIN`, `SD_CS_PIN`
   - Los pines de SPI de la pantalla van en su propio
     `lib/CustomTFTConfig/User_Setup_TUPLACA.h` para TFT_eSPI, no en
     el `Board_TUPLACA.h`
2. Agregá el `#elif defined(CYD_BOARD_TUPLACA)` correspondiente en
   `src/boards/BoardConfig.h`
3. Agregá un `[env:cyd-tuplaca]` en `platformio.ini` con su
   `-D CYD_BOARD_TUPLACA`
4. Agregalo a la matriz de `.github/workflows/build.yml` (y a
   `release.yml` si querés que también salga en los Releases)
5. Documentá de dónde sacaste el pinout en un comentario arriba de
   los `#define` — este proyecto no acepta pinouts "probé y anduvo"
   sin ninguna fuente, porque las placas CYD son clones chinos con
   variaciones entre lotes y un pinout sin origen es difícil de
   confiar o de debuggear después.

No hace falta que puedas compilar localmente para proponer esto: el
mismo workflow de CI que usa el mantenedor corre sobre tu PR y te va
a decir si compila en las 4 placas existentes.

## Convenciones de código

- Los nombres de variables, funciones y comentarios están en español
  en toda la base de código existente — mantené esa consistencia en
  el código que toques, aunque el inglés también se entiende sin
  problema en los PRs y discusiones.
- Nada de "ñ" en texto que se muestra en pantalla (la fuente de
  TFT_eSPI que usa este proyecto es ASCII básico).
- Patrón de redibujado: cada pantalla repinta solo lo que cambió
  (dirty-check), no redibuja todo en cada vuelta del loop. Mirá
  `contenidoSucio` en `Globals.h`/`main.cpp` y cómo lo usa cualquier
  `Screen*.cpp` antes de agregar una pantalla nueva.
- Si tu cambio toca texto que el usuario ve en pantalla y el
  firmware ya tiene el sistema de idiomas activo en esa pantalla
  (ver `Idioma.h`), agregá la clave nueva en las dos tablas (`ES[]` y
  `EN[]` de `Idioma.cpp`) en la misma posición relativa que en el
  enum - si ese archivo/pantalla todavía no migró al sistema de
  idiomas, no hace falta que lo migres vos solo para tu cambio.
- PRs grandes (que tocan muchos archivos a la vez) son más difíciles
  de revisar sin poder compilar localmente - si tu cambio se puede
  partir en pasos más chicos, mejor.

## Qué esperar del proceso de review

El mantenedor tampoco tiene un entorno local de PlatformIO armado para
este proyecto (todo pasa por GitHub Actions), así que el feedback de
un PR puede tardar a que termine de correr el CI. Un PR que falla el
build en alguna de las 4 placas no se mergea hasta que compile limpio
en todas.
