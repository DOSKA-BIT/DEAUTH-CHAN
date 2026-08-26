#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Cada placa soportada define su propio archivo Board_XXXX.h con estas
// macros. El selector vive en platformio.ini, una entre CYD_BOARD_2432S028,
// CYD_BOARD_2432S024, CYD_BOARD_3248S035R, CYD_BOARD_3248S035C o
// CYD_BOARD_2424S012 (build_flags -D).
//
// Lo que cada perfil tiene que declarar como mínimo:
//   TFT_PANEL_WIDTH / TFT_PANEL_HEIGHT   -> resolución en orientación nativa
//   TOUCH_IS_CAPACITIVE                  -> 0 = XPT2046 resistivo, 1 = capacitivo (GT911/CST816)
//   TFT_BL_PIN                            -> pin de backlight
//   SD_CS_PIN
//   (los pines de SPI del TFT quedan en el User_Setup.h de TFT_eSPI,
//    no acá, porque esa librería los necesita como defines de compilación)

#if defined(CYD_BOARD_2432S028)
    #include "Board_2432S028.h"
#elif defined(CYD_BOARD_2432S024)
    #include "Board_2432S024.h"
#elif defined(CYD_BOARD_3248S035R)
    #include "Board_3248S035R.h"
#elif defined(CYD_BOARD_3248S035C)
    #include "Board_3248S035C.h"
#elif defined(CYD_BOARD_2424S012)
    #include "Board_2424S012.h"
#else
    #error "No se definio ninguna placa CYD. Agrega -D CYD_BOARD_XXXX en platformio.ini"
#endif

#ifndef TOUCH_IS_CAPACITIVE
    #error "El perfil de placa tiene que definir TOUCH_IS_CAPACITIVE (0 o 1)"
#endif

#endif
