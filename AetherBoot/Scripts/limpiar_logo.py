#!/usr/bin/env python3
"""Elimina el fondo/neblina de un logotipo y conserva el brillo cercano.

Uso: python3 limpiar_logo.py entrada.png salida.png
"""
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

src, dst = sys.argv[1], sys.argv[2]
im = Image.open(src).convert("RGBA")
a = np.array(im).astype(np.float32)
rgb, al = a[..., :3], a[..., 3]

escala = max(im.size) / 1254.0
RADIO_BRILLO = 28 * escala      # distancia máxima de halo que se conserva (px)
PISO = 14.0                     # alfa por debajo de esto es neblina

# 1) Cuerpo del logo: píxeles casi opacos; se descartan motas sueltas
cuerpo = al >= 160
etiquetas, n = ndimage.label(cuerpo)
if n:
    tam = ndimage.sum(cuerpo, etiquetas, range(1, n + 1))
    minimo = 400 * escala * escala
    cuerpo = np.isin(etiquetas, np.nonzero(tam >= minimo)[0] + 1)
cuerpo = ndimage.binary_fill_holes(cuerpo) & (al > 0) | cuerpo

# 2) Atenuación del halo según la distancia al cuerpo
dist = ndimage.distance_transform_edt(~cuerpo)
atenuacion = np.clip(1.0 - dist / RADIO_BRILLO, 0.0, 1.0) ** 1.5

# 3) Quitar el piso de neblina y aplicar la atenuación fuera del cuerpo
alfa = np.clip((al - PISO) * 255.0 / (255.0 - PISO), 0, 255)
alfa = np.where(cuerpo, np.maximum(alfa, al), alfa * atenuacion)

# 4) Halo blanquecino: si es casi blanco y semitransparente, sobre fondo oscuro
#    se ve como niebla; se le reduce el alfa
blancura = rgb.min(axis=2) / 255.0
semi = (~cuerpo) & (alfa < 200)
alfa = np.where(semi, alfa * (1.0 - 0.85 * np.clip((blancura - 0.75) / 0.25, 0, 1)), alfa)

alfa[alfa < 3] = 0
salida = np.dstack([rgb, alfa]).astype(np.uint8)
salida[salida[..., 3] == 0, :3] = 0

res = Image.fromarray(salida, "RGBA")
caja = res.getbbox()
if caja:
    m = int(12 * escala)
    caja = (max(0, caja[0] - m), max(0, caja[1] - m),
            min(res.width, caja[2] + m), min(res.height, caja[3] + m))
    res = res.crop(caja)
res.save(dst, optimize=True)
print(f"{dst}: {res.size}")
