#!/usr/bin/env python3
"""Genera el tema GRUB2 oficial de AetherBoot.

Uso: python3 generar_tema_aetherboot.py <logo_aetherboot.png> <carpeta_salida>

Diseñado para las capacidades reales de GRUB 2.04:
  * PNG RGBA de 8 bits sin entrelazar (lo único que GRUB sabe leer)
  * paneles semitransparentes con mezcla alfa (sí la soporta)
  * sin animaciones ni desenfoque en vivo (GRUB no los soporta): el efecto
    "cristal" está pre-renderizado en las imágenes
Todo el arte, salvo el logotipo oficial, es procedural y original.
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

LOGO, OUT = sys.argv[1], sys.argv[2]
ICONOS = os.path.join(OUT, "icons")
os.makedirs(ICONOS, exist_ok=True)

# Paleta oficial AetherBoot
AZUL_PROFUNDO = (5, 11, 24)
AZUL_NOCHE = (10, 26, 63)
AZUL_ELECTRICO = (30, 107, 255)
CIAN = (34, 211, 238)
BLANCO = (255, 255, 255)
GRIS_TEXTO = (203, 213, 225)


def guardar(img, nombre, carpeta=OUT):
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    img.save(os.path.join(carpeta, nombre), optimize=True)


def mezclar(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


# ------------------------------------------------------------------ fondo
def fondo(w=1920, h=1080):
    img = Image.new("RGB", (w, h))
    px = img.load()
    cx, cy = w * 0.5, h * 0.42
    rmax = math.hypot(w, h) * 0.62
    for y in range(h):
        for x in range(w):
            t = min(1.0, math.hypot(x - cx, (y - cy) * 1.25) / rmax)
            px[x, y] = mezclar(AZUL_NOCHE, AZUL_PROFUNDO, t ** 0.8)
    img = img.convert("RGBA")

    brillo = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(brillo)
    d.ellipse((-w * 0.20, -h * 0.45, w * 0.45, h * 0.45), fill=AZUL_ELECTRICO + (70,))
    d.ellipse((w * 0.62, h * 0.55, w * 1.25, h * 1.40), fill=CIAN + (48,))
    brillo = brillo.filter(ImageFilter.GaussianBlur(180))
    img = Image.alpha_composite(img, brillo)

    # rejilla en perspectiva en la parte inferior
    capa = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(capa)
    horizonte = int(h * 0.80)
    for i in range(-14, 15):
        x0 = w / 2 + i * 60
        x1 = w / 2 + i * 260
        d.line((x0, horizonte, x1, h), fill=AZUL_ELECTRICO + (26,), width=1)
    for k in range(1, 9):
        y = horizonte + (h - horizonte) * (k / 8) ** 1.8
        d.line((0, y, w, y), fill=AZUL_ELECTRICO + (22,), width=1)

    # trazas tipo circuito en las esquinas
    def traza(puntos, alfa=55):
        d.line(puntos, fill=CIAN + (alfa,), width=2, joint="curve")
        x, y = puntos[-1]
        d.ellipse((x - 4, y - 4, x + 4, y + 4), outline=CIAN + (alfa + 30,), width=2)

    traza([(0, 160), (120, 160), (170, 210), (330, 210)])
    traza([(0, 240), (80, 240), (120, 280), (220, 280)], 40)
    traza([(w, 120), (w - 140, 120), (w - 190, 170), (w - 360, 170)])
    traza([(w, h - 300), (w - 90, h - 300), (w - 130, h - 260), (w - 260, h - 260)], 40)
    capa = capa.filter(ImageFilter.GaussianBlur(0.6))
    img = Image.alpha_composite(img, capa)
    return img.convert("RGB")


# ----------------------------------------------------- piezas de 9 cortes
def nueve(prefijo, tam, radio, relleno, borde=None, ancho_borde=1, brillo_sup=None):
    s = tam
    k = 4
    big = Image.new("RGBA", (s * k, s * k), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.rounded_rectangle((0, 0, s * k - 1, s * k - 1), radius=radio * k, fill=relleno,
                        outline=borde, width=ancho_borde * k if borde else 0)
    if brillo_sup:
        d.rounded_rectangle((ancho_borde * k, ancho_borde * k, s * k - 1 - ancho_borde * k, radio * k),
                            radius=radio * k, fill=brillo_sup)
    img = big.resize((s, s), Image.LANCZOS)
    c = radio
    m = s - 2 * c
    cortes = {
        "nw": (0, 0, c, c), "n": (c, 0, c + m, c), "ne": (c + m, 0, s, c),
        "w": (0, c, c, c + m), "c": (c, c, c + m, c + m), "e": (c + m, c, s, c + m),
        "sw": (0, c + m, c, s), "s": (c, c + m, c + m, s), "se": (c + m, c + m, s, s),
    }
    for nombre, caja in cortes.items():
        guardar(img.crop(caja), f"{prefijo}_{nombre}.png")


def seleccion():
    """Barra de selección azul iluminada con acento cian a la izquierda.

    Las esquinas miden 6 px: GRUB agranda el resaltado por arriba y por abajo
    en el alto de los cortes n/s, así que con item_spacing = 2*6 los
    resaltados no se montan sobre las entradas vecinas."""
    s, c, k = 36, 6, 4
    big = Image.new("RGBA", (s * k, s * k), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    for y in range(s * k):
        t = y / (s * k)
        col = mezclar((52, 132, 255), (20, 84, 230), t)
        d.line((0, y, s * k, y), fill=col + (235,))
    mascara = Image.new("L", big.size, 0)
    ImageDraw.Draw(mascara).rounded_rectangle((0, 0, s * k - 1, s * k - 1), radius=c * k, fill=255)
    big.putalpha(mascara.point(lambda v: int(v * 0.92)))
    d = ImageDraw.Draw(big)
    d.rounded_rectangle((0, 0, s * k - 1, s * k - 1), radius=c * k, outline=CIAN + (200,), width=k)
    d.rounded_rectangle((2 * k, 8 * k, 5 * k, s * k - 8 * k), radius=2 * k, fill=CIAN + (255,))
    img = big.resize((s, s), Image.LANCZOS)
    m = s - 2 * c
    cortes = {
        "nw": (0, 0, c, c), "n": (c, 0, c + m, c), "ne": (c + m, 0, s, c),
        "w": (0, c, c, c + m), "c": (c, c, c + m, c + m), "e": (c + m, c, s, c + m),
        "sw": (0, c + m, c, s), "s": (c, c + m, c + m, s), "se": (c + m, c + m, s, s),
    }
    for nombre, caja in cortes.items():
        guardar(img.crop(caja), f"select_{nombre}.png")


def barra_inferior():
    w, h = 64, 40
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    for y in range(h):
        d.line((0, y, w, y), fill=(6, 14, 34, int(200 + 30 * y / h)))
    d.line((0, 0, w, 0), fill=CIAN + (150,), width=1)
    guardar(img, "barra_inferior.png")


def deslizador():
    t = Image.new("RGBA", (8 * 4, 24 * 4), (0, 0, 0, 0))
    ImageDraw.Draw(t).rounded_rectangle((0, 0, 31, 95), radius=16, fill=CIAN + (210,))
    t = t.resize((8, 24), Image.LANCZOS)
    guardar(t.crop((0, 0, 8, 4)), "slider_n.png")
    guardar(t.crop((0, 4, 8, 20)), "slider_c.png")
    guardar(t.crop((0, 20, 8, 24)), "slider_s.png")


# ---------------------------------------------------------------- iconos
def lienzo():
    return Image.new("RGBA", (128, 128), (0, 0, 0, 0))


def icono(im, nombre):
    sombra = Image.new("RGBA", im.size, (0, 0, 0, 0))
    a = im.split()[3].filter(ImageFilter.GaussianBlur(6))
    sombra.putalpha(a.point(lambda v: int(v * 0.55)))
    capa = Image.new("RGBA", im.size, AZUL_ELECTRICO + (0,))
    capa.putalpha(sombra.split()[3])
    final = Image.alpha_composite(capa, im)
    guardar(final.resize((32, 32), Image.LANCZOS), nombre + ".png", ICONOS)


def i_disco(c=CIAN):
    im = lienzo(); d = ImageDraw.Draw(im)
    d.ellipse((10, 10, 118, 118), fill=AZUL_ELECTRICO + (255,))
    d.ellipse((24, 24, 104, 104), outline=c + (160,), width=4)
    d.ellipse((48, 48, 80, 80), fill=AZUL_PROFUNDO + (255,))
    d.ellipse((58, 58, 70, 70), fill=c + (255,))
    d.arc((16, 16, 112, 112), 200, 250, fill=BLANCO + (200,), width=6)
    return im


def i_capas():
    im = lienzo(); d = ImageDraw.Draw(im)
    for i, a in enumerate((120, 190, 255)):
        y = 80 - i * 24
        d.polygon([(64, y - 26), (118, y), (64, y + 26), (10, y)], fill=mezclar(AZUL_ELECTRICO, CIAN, i / 2) + (a,))
    return im


def i_chip(c=CIAN):
    im = lienzo(); d = ImageDraw.Draw(im)
    d.rounded_rectangle((30, 30, 98, 98), radius=10, fill=AZUL_ELECTRICO + (255,))
    d.rounded_rectangle((46, 46, 82, 82), radius=4, fill=AZUL_PROFUNDO + (255,))
    d.rectangle((58, 58, 70, 70), fill=c + (255,))
    for p in range(40, 92, 16):
        for caja in ((p, 12, p + 6, 28), (p, 100, p + 6, 116), (12, p, 28, p + 6), (100, p, 116, p + 6)):
            d.rectangle(caja, fill=c + (255,))
    return im


def i_unidad(led=CIAN):
    im = lienzo(); d = ImageDraw.Draw(im)
    d.rounded_rectangle((10, 36, 118, 96), radius=12, fill=(148, 178, 230, 255))
    d.rounded_rectangle((10, 36, 118, 60), radius=12, fill=(190, 210, 245, 255))
    d.line((22, 78, 80, 78), fill=AZUL_PROFUNDO + (170,), width=5)
    d.ellipse((92, 70, 106, 84), fill=led + (255,))
    return im


def i_cubo():
    im = lienzo(); d = ImageDraw.Draw(im)
    d.polygon([(64, 12), (112, 38), (64, 64), (16, 38)], fill=CIAN + (255,))
    d.polygon([(16, 38), (64, 64), (64, 118), (16, 92)], fill=AZUL_ELECTRICO + (255,))
    d.polygon([(112, 38), (64, 64), (64, 118), (112, 92)], fill=(16, 64, 170, 255))
    return im


def i_carpeta():
    im = lienzo(); d = ImageDraw.Draw(im)
    d.rounded_rectangle((10, 26, 60, 50), radius=8, fill=(22, 150, 200, 255))
    d.rounded_rectangle((10, 38, 118, 108), radius=10, fill=CIAN + (255,))
    d.rounded_rectangle((10, 52, 118, 108), radius=10, fill=(56, 189, 248, 255))
    return im


def i_volver():
    im = lienzo(); d = ImageDraw.Draw(im)
    d.ellipse((8, 8, 120, 120), outline=GRIS_TEXTO + (255,), width=8)
    d.polygon([(34, 64), (66, 34), (66, 52), (94, 52), (94, 76), (66, 76), (66, 94)], fill=GRIS_TEXTO + (255,))
    return im


def i_engranaje():
    im = lienzo(); d = ImageDraw.Draw(im)
    pts = []
    for i in range(16):
        a = i * math.pi / 8
        r = 56 if i % 2 == 0 else 42
        for da in (-0.17, 0.17):
            pts.append((64 + r * math.cos(a + da), 64 + r * math.sin(a + da)))
    d.polygon(pts, fill=CIAN + (255,))
    d.ellipse((50, 50, 78, 78), fill=AZUL_PROFUNDO + (255,))
    return im


def i_encendido():
    im = lienzo(); d = ImageDraw.Draw(im)
    d.arc((14, 18, 114, 118), 300, 240, fill=(96, 165, 250, 255), width=12)
    d.rounded_rectangle((58, 6, 70, 64), radius=6, fill=(96, 165, 250, 255))
    return im


def i_escudo():
    im = lienzo(); d = ImageDraw.Draw(im)
    d.polygon([(64, 8), (112, 26), (106, 80), (64, 120), (22, 80), (16, 26)], fill=CIAN + (255,))
    d.line([(40, 64), (58, 82), (90, 46)], fill=AZUL_PROFUNDO + (255,), width=12, joint="curve")
    return im


def i_info():
    im = lienzo(); d = ImageDraw.Draw(im)
    d.ellipse((8, 8, 120, 120), fill=AZUL_ELECTRICO + (255,))
    d.ellipse((56, 26, 72, 42), fill=BLANCO + (255,))
    d.rounded_rectangle((56, 52, 72, 100), radius=5, fill=BLANCO + (255,))
    return im


def i_herramienta():
    im = lienzo(); d = ImageDraw.Draw(im)
    d.line((30, 98, 84, 44), fill=GRIS_TEXTO + (255,), width=18)
    d.ellipse((70, 14, 118, 62), fill=GRIS_TEXTO + (255,))
    ImageDraw.Draw(im).ellipse((88, 20, 112, 44), fill=(0, 0, 0, 0))
    return im


# ------------------------------------------------------------- construir
guardar(fondo(), "background.png")
nueve("menu", 48, 18, (24, 56, 130, 92), (120, 190, 255, 90), 1)
nueve("terminal_box", 48, 14, (5, 11, 24, 240), (56, 120, 220, 255))
seleccion()
barra_inferior()
deslizador()

logo = Image.open(LOGO).convert("RGBA")
ancho = 460
alto = round(logo.height * ancho / logo.width)
logo = logo.resize((ancho, alto), Image.LANCZOS)
guardar(logo, "logo.png")

iconos = {
    "vtoyiso": i_disco(), "vtoywim": i_capas(), "vtoyefi": i_chip(), "vtoyimg": i_unidad(),
    "vtoyvhd": i_unidad(AZUL_ELECTRICO), "vtoyvtoy": i_cubo(), "vtoydir": i_carpeta(),
    "vtoyret": i_volver(), "F5tool": i_engranaje(), "F4boot": i_encendido(),
    "boot_disk": i_unidad(), "boot_uefi": i_chip(), "boot_windows": i_unidad(AZUL_ELECTRICO),
    "boot_xorboot": i_unidad(), "debug_util": i_herramienta(), "debug_json": i_herramienta(),
    "debug_theme": i_engranaje(), "debug_power": i_encendido(), "debug_reboot": i_encendido(),
    "debug_hwinfo": i_info(), "custom": i_herramienta(), "customsub": i_carpeta(),
    "debug_info": i_info(), "debug_halt": i_encendido(), "debug_efisetup": i_chip(),
}
for k in ("checksum_md5", "checksum_sha1", "checksum_sha256", "checksum_sha512"):
    iconos[k] = i_escudo()
for nombre, im in iconos.items():
    icono(im, nombre)

print(f"tema generado en {OUT}; logo {ancho}x{alto}")
