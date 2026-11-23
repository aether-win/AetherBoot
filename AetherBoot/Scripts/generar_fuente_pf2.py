#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
generar_fuente_pf2.py — convierte una fuente TrueType al formato PF2 de GRUB.

Sustituye a grub-mkfont para el tema de AetherBoot. Solo incluye los
caracteres latinos (ASCII, Latin-1, Latin Extended-A y signos comunes); para
cualquier otro carácter GRUB usa automáticamente la fuente unicode.pf2 que ya
carga el menú.

Uso:
  generar_fuente_pf2.py fuente.ttf salida.pf2 "Nombre Familia" tamaño_px [bold]

Formato PF2 (grub-core/font/font.c): secciones [nombre 4 bytes][longitud BE]
[datos]; CHIX = índice ordenado (código, banderas, desplazamiento); cada glifo
= ancho, alto, desplazamiento x, desplazamiento y (borde inferior respecto a
la línea base), avance, y el mapa de bits de 1 bit por píxel sin relleno.
"""
import struct
import sys

from PIL import Image, ImageDraw, ImageFont


def rangos():
    cods = list(range(0x20, 0x7F)) + list(range(0xA0, 0x180))
    cods += [0x2013, 0x2014, 0x2018, 0x2019, 0x201A, 0x201C, 0x201D, 0x201E,
             0x2022, 0x2026, 0x2039, 0x203A, 0x20AC, 0x2122,
             0x2190, 0x2191, 0x2192, 0x2193, 0x25B2, 0x25B6, 0x25BC, 0x25C0]
    return sorted(set(cods))


def glifo(fuente, cod, tam):
    ch = chr(cod)
    lienzo = tam * 3
    base = tam * 2
    img = Image.new("1", (lienzo, lienzo), 0)
    d = ImageDraw.Draw(img)
    d.fontmode = "1"                       # sin suavizado: GRUB dibuja 1 bit/píxel
    x0 = tam // 2
    d.text((x0, base), ch, font=fuente, fill=1, anchor="ls")
    avance = int(round(fuente.getlength(ch)))
    caja = img.getbbox()
    if caja is None:                       # espacio u otro carácter vacío
        return 0, 0, 0, 0, avance, b""
    l, t, r, b = caja
    w, h = r - l, b - t
    xoff = l - x0
    yoff = base - b                        # borde inferior respecto a la línea base (positivo hacia arriba)
    bits = []
    px = img.load()
    for y in range(t, b):
        for x in range(l, r):
            bits.append(1 if px[x, y] else 0)
    datos = bytearray((len(bits) + 7) // 8)
    for i, v in enumerate(bits):
        if v:
            datos[i // 8] |= 0x80 >> (i % 8)
    return w, h, xoff, yoff, avance, bytes(datos)


def seccion(nombre, datos):
    return nombre + struct.pack(">I", len(datos)) + datos


def main():
    ttf, salida, familia, tam = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
    negrita = len(sys.argv) > 5 and sys.argv[5] == "bold"
    fuente = ImageFont.truetype(ttf, tam)
    asc, desc = fuente.getmetrics()

    glifos = []
    for cod in rangos():
        try:
            g = glifo(fuente, cod, tam)
        except Exception:
            continue
        if cod != 0x20 and g[0] == 0 and g[4] == 0:
            continue
        glifos.append((cod, g))

    peso = "Bold" if negrita else "Regular"
    nombre = f"{familia} {peso} {tam}"
    maxw = max(g[0] for _, g in glifos)
    maxh = max(g[1] for _, g in glifos)

    cab = seccion(b"FILE", b"PFF2")
    cab += seccion(b"NAME", nombre.encode() + b"\0")
    cab += seccion(b"FAMI", familia.encode() + b"\0")
    cab += seccion(b"WEIG", (b"bold" if negrita else b"normal") + b"\0")
    cab += seccion(b"SLAN", b"normal\0")
    cab += seccion(b"PTSZ", struct.pack(">H", tam))
    cab += seccion(b"MAXW", struct.pack(">H", maxw))
    cab += seccion(b"MAXH", struct.pack(">H", maxh))
    cab += seccion(b"ASCE", struct.pack(">H", asc))
    cab += seccion(b"DESC", struct.pack(">H", desc))

    tam_chix = 9 * len(glifos)
    inicio_datos = len(cab) + 8 + tam_chix + 8   # CHIX (cabecera + datos) + cabecera DATA
    chix = b""
    datos = b""
    for cod, (w, h, xo, yo, av, bm) in glifos:
        off = inicio_datos + len(datos)
        chix += struct.pack(">IBI", cod, 0, off)
        datos += struct.pack(">HHhhh", w, h, xo, yo, av) + bm

    out = cab + seccion(b"CHIX", chix) + b"DATA" + struct.pack(">I", 0xFFFFFFFF) + datos
    with open(salida, "wb") as f:
        f.write(out)
    print(f"{salida}: '{nombre}', {len(glifos)} glifos, {len(out)} bytes, "
          f"asc {asc} desc {desc} maxw {maxw} maxh {maxh}")


if __name__ == "__main__":
    main()
