#!/usr/bin/env python3
"""Simulación aproximada del menú AetherBoot (mismas piezas y posiciones que
el theme.txt). La fuente real de GRUB (unicode.pf2, mapa de bits) se ve distinta."""
import sys
from PIL import Image, ImageDraw, ImageFont
T, OUT, W, H = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
pct = lambda p, tot: p * tot / 100
bg = Image.open(f"{T}/background.png").convert("RGBA")
# crop: escala para cubrir y recorta al centro
s = max(W / bg.width, H / bg.height)
bg = bg.resize((round(bg.width * s), round(bg.height * s)), Image.LANCZOS)
x0 = (bg.width - W) // 2; y0 = (bg.height - H) // 2
can = bg.crop((x0, y0, x0 + W, y0 + H))
ld = lambda n: Image.open(f"{T}/{n}.png").convert("RGBA")
def nueve(pref, box):
    a, b, c, d = map(int, box); p = {k: ld(f"{pref}_{k}") for k in "nw n ne w c e sw s se".split()}
    cw = p["nw"].width; ch = p["nw"].height
    can.alpha_composite(p["nw"], (a, b)); can.alpha_composite(p["ne"], (c - cw, b))
    can.alpha_composite(p["sw"], (a, d - ch)); can.alpha_composite(p["se"], (c - cw, d - ch))
    can.alpha_composite(p["n"].resize((c - a - 2 * cw, ch)), (a + cw, b))
    can.alpha_composite(p["s"].resize((c - a - 2 * cw, ch)), (a + cw, d - ch))
    can.alpha_composite(p["w"].resize((cw, d - b - 2 * ch)), (a, b + ch))
    can.alpha_composite(p["e"].resize((cw, d - b - 2 * ch)), (c - cw, b + ch))
    can.alpha_composite(p["c"].resize((c - a - 2 * cw, d - b - 2 * ch)), (a + cw, b + ch))
f = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 15)
fs = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13)
dr = ImageDraw.Draw(can)
can.alpha_composite(ld("logo"), (int(W / 2 - 230), int(pct(3, H))))
txt = "AetherBoot 1.1.18 UEFI"; tw = dr.textlength(txt, font=fs)
dr.text(((W - tw) / 2, pct(3, H) + 104), txt, font=fs, fill="#22d3ee")
mx0, mx1 = int(pct(12, W)), int(pct(88, W)); my0 = int(pct(3, H) + 134); my1 = int(my0 + pct(79, H) - 134)
nueve("menu", (mx0, my0, mx1, my1))
items = [("vtoydir", "ISO"), ("vtoyiso", "ubuntu-24.04.3-desktop-amd64.iso"), ("vtoyiso", "Win11_25H2_Spanish_x64.iso"),
         ("vtoyiso", "linuxmint-22.2-cinnamon-64bit.iso"), ("vtoywim", "WinPE_rescate.wim"), ("vtoyimg", "memtest86-usb.img"),
         ("vtoyvhd", "Windows11_portable.vhdx"), ("vtoyefi", "shellx64.efi"), ("vtoyvtoy", "Kali_persistente.vtoy")]
y = my0 + 10
for i, (cls, t) in enumerate(items):
    if y + 36 > my1 - 10: break
    if i == 1: nueve("select", (mx0 + 10, y, mx1 - 26, y + 36))
    can.alpha_composite(Image.open(f"{T}/icons/{cls}.png").convert("RGBA").resize((28, 28)), (mx0 + 22, y + 4))
    dr.text((mx0 + 22 + 28 + 14, y + 9), t, font=f, fill="#ffffff" if i == 1 else "#cbd5e1")
    y += 40
can.alpha_composite(ld("slider_n"), (mx1 - 18, my0 + 12)); can.alpha_composite(ld("slider_c").resize((8, 90)), (mx1 - 18, my0 + 16)); can.alpha_composite(ld("slider_s"), (mx1 - 18, my0 + 106))
py = int(pct(88, H)); dr.rectangle((mx0, py, mx1, py + 20), fill="#0a1a3f", outline="#22d3ee")
dr.rectangle((mx0 + 1, py + 1, mx0 + int((mx1 - mx0) * 0.55), py + 19), fill="#1e6bff")
t = "Arranque automático en 6 s"; dr.text(((W - dr.textlength(t, font=fs)) / 2, py + 3), t, font=fs, fill="#e2e8f0")
can.alpha_composite(ld("barra_inferior").resize((W, 40)), (0, H - 40))
t = "L:Idioma  F1:Ayuda  F2:Navegar  F3:Vista de lista  F4:Arranque local  F5:Herramientas  F6:Menú ext."
dr.text(((W - dr.textlength(t, font=fs)) / 2, H - 29), t, font=fs, fill="#cbd5e1")
can.convert("RGB").save(OUT)
