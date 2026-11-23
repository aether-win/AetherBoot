#!/usr/bin/env python3
"""Sustituye el lema en inglés del logo AetherBoot por uno en español."""
import sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter
src, dst, texto, fuente = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
im = Image.open(src).convert("RGBA")
X0, X1, Y0, Y1 = 790, 1812, 298, 352          # caja del lema original
CAP = 32                                        # altura de mayúsculas original
COLOR = (214, 222, 238)

d = ImageDraw.Draw(im)
d.rectangle((X0, Y0, X1, Y1), fill=(0, 0, 0, 0))

# tamaño de letra para que la altura de mayúscula sea CAP
size = 40
while True:
    f = ImageFont.truetype(fuente, size)
    try:
        f.set_variation_by_axes([14, 500])   # opsz, peso medio
    except Exception:
        pass
    bb = f.getbbox("H")
    if bb[3] - bb[1] >= CAP: break
    size += 1

def ancho(esp):
    return sum(f.getlength(c) for c in texto) + esp * (len(texto) - 1)

objetivo = 1008
esp = (objetivo - sum(f.getlength(c) for c in texto)) / (len(texto) - 1)
esp = max(4, esp)
total = ancho(esp)
x = (797 + 1805) / 2 - total / 2
top = 308 - (f.getbbox("H")[1])

capa = Image.new("RGBA", im.size, (0, 0, 0, 0))
dc = ImageDraw.Draw(capa)
for c in texto:
    dc.text((x, top), c, font=f, fill=COLOR + (255,))
    x += f.getlength(c) + esp
# brillo suave como el original
glow = capa.filter(ImageFilter.GaussianBlur(3))
r, g, b, a = glow.split()
glow = Image.merge("RGBA", (r, g, b, a.point(lambda v: v * 0.35)))
im.alpha_composite(glow); im.alpha_composite(capa)
im.save(dst, optimize=True)
print(dst, "tamaño", size, "espaciado", round(esp, 1))
