#!/usr/bin/env python3
"""
Genera las caras de la mascota (pixel art estilo anime/pwnagotchi: ojos
grandes y redondos con un brillito, como pidio el usuario) y escribe
src/MascotaCaras.h con los arrays listos para
TFT_eSPI::drawBitmap(x, y, bitmap, w, h, color, colorFondo) - el mismo
metodo que ya usa Iconos.h, nada mas que con un lienzo mas grande
porque esta es la cara PRINCIPAL (la de la pantalla de Mascota), no un
icono chico de lista.

Por que pixel art y no un sprite importado: las fuentes que usa
TFT_eSPI en esta pantalla son ASCII basico (el mismo motivo por el que
el proyecto evita la "ñ" en texto on-screen) - caracteres unicode como
los de un kaomoji real no existen en esa fuente y saldrian como
basura. Estas caras son formas geometricas simples dibujadas a mano
via Pillow (circulos, arcos, lineas): un ojo "anime" generico (forma
redonda grande + un brillito) no es el diseño de ningun personaje en
particular, es un estilo de dibujo igual de generico que la carita
kaomoji de la version anterior.

Reemplaza a MascotaSprite.h (que generaba tools/convert_mascota_frames.py
desde assets/mascota_frames/*.png, arte a color). Esos dos quedan sin
uso despues de este cambio - se pueden borrar del repo si se quiere
(no hace falta para que compile: el firmware ya no los referencia).

Cada estado tiene 2 frames (igual que antes), para el mismo esquema de
animacion que ya tiene Mascota.cpp (frame2 segun el estado - ver
caraParaEstado() alla).
"""
from PIL import Image, ImageDraw

W, H = 56, 56


def lienzo():
    return Image.new("1", (W, H), 0)


def empaquetar(img):
    bytes_por_fila = (W + 7) // 8
    datos = []
    px = img.load()
    for y in range(H):
        for byte_idx in range(bytes_por_fila):
            b = 0
            for bit in range(8):
                x = byte_idx * 8 + bit
                valor = px[x, y] if x < W else 0
                b = (b << 1) | (1 if valor else 0)
            datos.append(b)
    return datos


# ---- Ojo "anime": forma redonda solida bien grande + 1-2 brillitos ----
# (el brillito es un agujero del color de fondo sobre el relleno negro,
# no un trazo aparte - asi se ve de verdad como un ojo con luz adentro
# en vez de un circulo plano).
def ojo_anime(d, cx, cy, rx=8, ry=9, brillo_grande=True, brillo_chico=True):
    d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=1)
    if brillo_grande:
        bx, by, br = cx - rx * 0.35, cy - ry * 0.4, max(2, rx * 0.3)
        d.ellipse([bx - br, by - br, bx + br, by + br], fill=0)
    if brillo_chico:
        bx, by, br = cx + rx * 0.3, cy + ry * 0.45, max(1, rx * 0.15)
        d.ellipse([bx - br, by - br, bx + br, by + br], fill=0)


# ---- IDLE: ojos grandes tranquilos con brillito, boquita chica ----
def cara_idle_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    ojo_anime(d, 18, 27, rx=7, ry=8)
    ojo_anime(d, 38, 27, rx=7, ry=8)
    d.arc([22, 38, 34, 46], start=20, end=160, fill=1, width=2)
    return img


def cara_idle_2():
    # parpadeo leve: los ojos se cierran un poco (mas bajos y mas finos)
    img = lienzo()
    d = ImageDraw.Draw(img)
    ojo_anime(d, 18, 28, rx=7, ry=5)
    ojo_anime(d, 38, 28, rx=7, ry=5)
    d.arc([22, 38, 34, 46], start=20, end=160, fill=1, width=2)
    return img


# ---- SCANNING: mirando/buscando, con una onda de "radar" arriba ----
def cara_scanning_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    # mismo ojo anime grande, pero el brillito corrido hacia un lado
    # para que se note que esta mirando para alla (en vez de mover todo
    # el ojo, que a este tamaño se veria raro).
    ojo_anime(d, 18, 27, rx=7, ry=8, brillo_chico=False)
    d.ellipse([12, 23, 15, 26], fill=0)
    ojo_anime(d, 38, 27, rx=7, ry=8, brillo_chico=False)
    d.ellipse([32, 23, 35, 26], fill=0)
    d.line([22, 44, 34, 44], fill=1, width=2)
    d.arc([8, 1, 32, 17], start=195, end=345, fill=1, width=2)
    return img


def cara_scanning_2():
    # mismo ojo pero el brillito (y la "mirada") corrido para el otro lado
    img = lienzo()
    d = ImageDraw.Draw(img)
    ojo_anime(d, 18, 27, rx=7, ry=8, brillo_chico=False)
    d.ellipse([21, 23, 24, 26], fill=0)
    ojo_anime(d, 38, 27, rx=7, ry=8, brillo_chico=False)
    d.ellipse([41, 23, 44, 26], fill=0)
    d.line([22, 44, 34, 44], fill=1, width=2)
    d.arc([24, 1, 48, 17], start=195, end=345, fill=1, width=2)
    return img


# ---- HAPPY: ojos anime bien grandes y sonrisa ancha ----
def cara_happy_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    ojo_anime(d, 18, 26, rx=8, ry=9)
    ojo_anime(d, 38, 26, rx=8, ry=9)
    d.arc([16, 30, 40, 48], start=20, end=160, fill=1, width=3)
    return img


def cara_happy_2():
    # version "entrecerrados de la risa" (^ ^): mismos ojos pero
    # cerrados en arco de felicidad, igual que antes
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.arc([11, 22, 25, 36], start=200, end=340, fill=1, width=3)
    d.arc([31, 22, 45, 36], start=200, end=340, fill=1, width=3)
    d.arc([14, 28, 42, 50], start=15, end=165, fill=1, width=3)
    return img


# ---- CURIOSA: ceja levantada + ojo sorprendido mas grande, boca en "o" ----
def cara_curiosa_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    # un ojo normal, el otro mas grande y con la ceja levantada (la
    # asimetria es lo que transmite la duda)
    ojo_anime(d, 18, 27, rx=6, ry=7)
    ojo_anime(d, 38, 25, rx=8, ry=9)
    d.line([33, 11, 45, 13], fill=1, width=2)  # ceja levantada
    d.ellipse([22, 38, 34, 48], outline=1, width=2)  # boca "o" de duda, centrada
    return img


def cara_curiosa_2():
    # la sorpresa: los dos ojos bien abiertos de golpe (grandes, sin brillito
    # chico para que se vean mas "platos" que los de IDLE/HAPPY)
    img = lienzo()
    d = ImageDraw.Draw(img)
    ojo_anime(d, 18, 26, rx=8, ry=10, brillo_chico=False)
    ojo_anime(d, 38, 26, rx=8, ry=10, brillo_chico=False)
    d.ellipse([23, 36, 33, 46], outline=1, width=2)  # boca "o" mas grande
    return img


# ---- SLEEP: ojos cerrados (lineas curvas) + "z" ----
def cara_sleep_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.arc([12, 22, 24, 30], start=10, end=170, fill=1, width=2)
    d.arc([32, 22, 44, 30], start=10, end=170, fill=1, width=2)
    d.arc([22, 34, 34, 42], start=20, end=160, fill=1, width=2)  # boquita tranquila
    _dibujar_z(d, 42, 4, 6)
    return img


def cara_sleep_2():
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.arc([12, 22, 24, 30], start=10, end=170, fill=1, width=2)
    d.arc([32, 22, 44, 30], start=10, end=170, fill=1, width=2)
    d.arc([22, 34, 34, 42], start=20, end=160, fill=1, width=2)
    _dibujar_z(d, 40, 2, 8)
    _dibujar_z(d, 48, -6, 5)
    return img


def _dibujar_z(d, x, y, tam):
    # una "Z" chiquita dibujada a mano con 3 lineas (no hay texto real
    # en este bitmap, es parte del dibujo).
    d.line([x, y, x + tam, y], fill=1, width=1)
    d.line([x + tam, y, x, y + tam], fill=1, width=1)
    d.line([x, y + tam, x + tam, y + tam], fill=1, width=1)


CARAS = [
    ("CARA_IDLE_1", cara_idle_1()),
    ("CARA_IDLE_2", cara_idle_2()),
    ("CARA_SCANNING_1", cara_scanning_1()),
    ("CARA_SCANNING_2", cara_scanning_2()),
    ("CARA_HAPPY_1", cara_happy_1()),
    ("CARA_HAPPY_2", cara_happy_2()),
    ("CARA_CURIOSA_1", cara_curiosa_1()),
    ("CARA_CURIOSA_2", cara_curiosa_2()),
    ("CARA_SLEEP_1", cara_sleep_1()),
    ("CARA_SLEEP_2", cara_sleep_2()),
]

out = []
out.append("#ifndef MASCOTA_CARAS_H")
out.append("#define MASCOTA_CARAS_H")
out.append("")
out.append("#include <Arduino.h>")
out.append("")
out.append("// Caras de la mascota, generadas con tools/generar_caras.py.")
out.append("// Pixel art monocromo (1 bit por pixel) dibujado a mano con Pillow,")
out.append("// estilo anime/pwnagotchi (ojos grandes redondos con brillito) -")
out.append("// no son una copia de ninguna imagen existente, ni texto unicode")
out.append("// (la fuente de TFT_eSPI que usa esta pantalla es ASCII basico,")
out.append("// esos caracteres saldrian como basura - ver el comentario largo")
out.append("// arriba de este archivo, en el script que lo genero).")
out.append("//")
out.append("// Reemplaza a MascotaSprite.h (arte a color, RGB565). Se usan con")
out.append("// TFT_eSPI::drawBitmap(x, y, bitmap, CARA_ANCHO, CARA_ALTO, color,")
out.append("// colorFondo), igual que los iconos de Iconos.h.")
out.append("#define CARA_ANCHO 56")
out.append("#define CARA_ALTO  56")
out.append("")

for nombre, img in CARAS:
    datos = empaquetar(img)
    out.append(f"static const uint8_t {nombre}[] PROGMEM = {{")
    linea = "    "
    for i, b in enumerate(datos):
        linea += f"0x{b:02X}, "
        if (i + 1) % 12 == 0:
            out.append(linea)
            linea = "    "
    if linea.strip():
        out.append(linea)
    out.append("};")
    out.append("")

out.append("#endif")

with open("/home/claude/work/deauth-chan/src/MascotaCaras.h", "w") as f:
    f.write("\n".join(out) + "\n")

print("MascotaCaras.h generado con", len(CARAS), "caras")
