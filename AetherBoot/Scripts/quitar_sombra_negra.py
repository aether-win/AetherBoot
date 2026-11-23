#!/usr/bin/env python3
"""Quita una sombra negra semitransparente (color a alfa desde negro).
Uso: python3 quitar_sombra_negra.py entrada.png salida.png"""
import sys
import numpy as np
from PIL import Image
im = Image.open(sys.argv[1]).convert("RGBA")
a = np.array(im).astype(np.float32)
rgb, al = a[..., :3], a[..., 3]
mx = rgb.max(axis=2)
f = np.clip(mx / 255.0, 0, 1)
# por debajo de ~12% de brillo es sombra pura
f = np.clip((f - 0.06) / 0.94, 0, 1)
# el cuerpo del logo (casi opaco) no se toca: solo la sombra
cuerpo = (al >= 240) & (mx >= 90)
f = np.where(cuerpo, 1.0, f)
nal = al * f
safe = np.maximum(mx, 1)[..., None]
un = np.clip(rgb * 255.0 / safe * np.minimum(1, mx[..., None] / 255.0 / np.maximum(f[..., None], 1e-3)), 0, 255)
nrgb = np.where(cuerpo[..., None], rgb, np.where(f[..., None] > 0, un, 0))
nal[nal < 4] = 0
out = np.dstack([nrgb, nal]).astype(np.uint8)
out[out[..., 3] == 0, :3] = 0
res = Image.fromarray(out, "RGBA")
b = res.getbbox(); m = 16
res = res.crop((max(0, b[0]-m), max(0, b[1]-m), min(res.width, b[2]+m), min(res.height, b[3]+m)))
res.save(sys.argv[2], optimize=True); print(sys.argv[2], res.size)
