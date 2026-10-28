#!/usr/bin/env python3
"""
Genera las caras nuevas de la mascota (pixel art estilo pwnagotchi,
inspiradas en las caritas tipo kaomoji que mando el usuario) y escribe
src/MascotaCaras.h con los arrays listos para
TFT_eSPI::drawBitmap(x, y, bitmap, w, h, color, colorFondo) - el mismo
metodo que ya usa Iconos.h, nada mas que con un lienzo mas grande
porque esta es la cara PRINCIPAL (la de la pantalla de Mascota), no un
icono chico de lista.

Por que pixel art y no el texto kaomoji tal cual: las fuentes que usa
TFT_eSPI en esta pantalla son ASCII basico (el mismo motivo por el que
el proyecto evita la "ñ" en texto on-screen) - los caracteres unicode
de un kaomoji real (⇀‸↼, ♥, etc) no existen en esa fuente y saldrian
como basura. Estas caras son formas geometricas simples dibujadas a
mano via Pillow (circulos, arcos, lineas), pensadas para transmitir
la misma idea de "carita de emoji" que la referencia, no un recorte de
ninguna imagen existente.

Reemplaza a MascotaSprite.h (que generaba tools/convert_mascota_frames.py
desde assets/mascota_frames/*.png, arte a color). Esos dos quedan sin
uso despues de este cambio - se pueden borrar del repo si se quiere
(no hace falta para que compile: el firmware ya no los referencia).

Cada estado tiene 2 frames (igual que antes), para el mismo esquema de
animacion que ya tiene Mascota.cpp (frame2 segun el estado - ver
spriteParaEstado() alla).
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


# ---- IDLE: carita tranquila, ojitos curvos cerrados tipo ' ◡ ◡ ' ----
def cara_idle_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    # ojos: arco hacia arriba (sonrisa en los ojos, estilo kaomoji ^‿^)
    d.arc([12, 20, 24, 32], start=190, end=350, fill=1, width=3)
    d.arc([32, 20, 44, 32], start=190, end=350, fill=1, width=3)
    # sonrisa chica
    d.arc([20, 34, 36, 44], start=20, end=160, fill=1, width=2)
    return img


def cara_idle_2():
    # variante con un parpadeo leve: ojos un poco mas achicados
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.arc([13, 22, 23, 30], start=190, end=350, fill=1, width=2)
    d.arc([33, 22, 43, 30], start=190, end=350, fill=1, width=2)
    d.arc([20, 34, 36, 44], start=20, end=160, fill=1, width=2)
    return img


# ---- SCANNING: mirando/buscando, con una onda de "radar" arriba ----
def cara_scanning_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    # ojos mirando para un lado (circulo con el iris corrido)
    d.ellipse([12, 22, 25, 35], outline=1, width=2)
    d.ellipse([14, 25, 18, 29], fill=1)
    d.ellipse([31, 22, 44, 35], outline=1, width=2)
    d.ellipse([33, 25, 37, 29], fill=1)
    # boca chica neutra
    d.line([22, 44, 34, 44], fill=1, width=2)
    # onda de "radar" arriba de la cabeza, corrida a la izquierda
    d.arc([8, 1, 32, 17], start=195, end=345, fill=1, width=2)
    return img


def cara_scanning_2():
    # mismo ojo pero mirando para el otro lado, onda corrida a la derecha
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.ellipse([12, 22, 25, 35], outline=1, width=2)
    d.ellipse([19, 25, 23, 29], fill=1)
    d.ellipse([31, 22, 44, 35], outline=1, width=2)
    d.ellipse([38, 25, 42, 29], fill=1)
    d.line([22, 44, 34, 44], fill=1, width=2)
    d.arc([24, 1, 48, 17], start=195, end=345, fill=1, width=2)
    return img


# ---- HAPPY: ojos grandes con brillo + sonrisa ancha ----
def cara_happy_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.ellipse([11, 18, 25, 32], fill=1)
    d.ellipse([31, 18, 45, 32], fill=1)
    d.ellipse([14, 20, 18, 24], fill=0)  # brillito (hueco) ojo izq
    d.ellipse([34, 20, 38, 24], fill=0)  # brillito ojo der
    d.arc([16, 30, 40, 48], start=20, end=160, fill=1, width=3)
    return img


def cara_happy_2():
    # version "entrecerrados de la risa" (^ ^), para la segunda mitad
    # del estado HAPPY, igual que antes
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.arc([11, 22, 25, 36], start=200, end=340, fill=1, width=3)
    d.arc([31, 22, 45, 36], start=200, end=340, fill=1, width=3)
    d.arc([14, 28, 42, 50], start=15, end=165, fill=1, width=3)
    return img


# ---- CURIOSA: ceja levantada + ojos bien abiertos, boca en "o" ----
def cara_curiosa_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    # un ojo normal, el otro con la ceja levantada (asimetria = duda)
    d.ellipse([12, 20, 24, 32], outline=1, width=2)
    d.ellipse([16, 24, 20, 28], fill=1)
    d.ellipse([32, 18, 44, 30], outline=1, width=2)
    d.ellipse([36, 22, 40, 26], fill=1)
    d.line([33, 11, 43, 13], fill=1, width=2)  # ceja levantada
    d.ellipse([22, 38, 34, 48], outline=1, width=2)  # boca "o" de duda, centrada
    return img


def cara_curiosa_2():
    # la sorpresa: los dos ojos bien abiertos de golpe
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.ellipse([11, 18, 25, 32], outline=1, width=2)
    d.ellipse([15, 22, 21, 28], fill=1)
    d.ellipse([31, 18, 45, 32], outline=1, width=2)
    d.ellipse([35, 22, 41, 28], fill=1)
    d.ellipse([23, 36, 33, 46], outline=1, width=2)  # boca "o" mas grande
    return img


# ---- SLEEP: ojos cerrados (lineas) + "z" ----
def cara_sleep_1():
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.line([13, 26, 23, 26], fill=1, width=2)
    d.line([33, 26, 43, 26], fill=1, width=2)
    d.arc([22, 34, 34, 42], start=20, end=160, fill=1, width=2)  # boquita tranquila
    _dibujar_z(d, 42, 4, 6)
    return img


def cara_sleep_2():
    img = lienzo()
    d = ImageDraw.Draw(img)
    d.line([13, 26, 23, 26], fill=1, width=2)
    d.line([33, 26, 43, 26], fill=1, width=2)
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
out.append("// con la idea de las caritas estilo kaomoji que trajo el usuario -")
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
