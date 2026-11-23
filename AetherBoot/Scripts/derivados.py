#!/usr/bin/env python3
"""Genera los recursos derivados de la identidad AetherBoot."""
import os
from PIL import Image

B = "/home/claude/aether/Branding"
L = f"{B}/Logo"
os.makedirs(f"{B}/Icons/ico", exist_ok=True)
os.makedirs(f"{B}/Icons/png", exist_ok=True)
os.makedirs(f"{B}/Logo/web", exist_ok=True)
os.makedirs(f"{B}/Grub", exist_ok=True)

def cuadrado(im):
    lado = max(im.size)
    c = Image.new("RGBA", (lado, lado), (0, 0, 0, 0))
    c.alpha_composite(im, ((lado - im.width) // 2, (lado - im.height) // 2))
    return c

def red(im, w=None, h=None):
    im = im.copy()
    if w and not h: h = round(im.height * w / im.width)
    if h and not w: w = round(im.width * h / im.height)
    return im.resize((w, h), Image.LANCZOS)

def png(im, ruta):
    im.save(ruta, optimize=True)

apps = {"Aether2Disk": "Aether2Disk_icono.png",
        "AetherPlugson": "AetherPlugson_icono.png",
        "AetherVInk": "AetherVInk_icono.png"}

for app, f in apps.items():
    sq = cuadrado(Image.open(f"{L}/{f}"))
    tams = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
    sq.save(f"{B}/Icons/ico/{app}.ico", sizes=[(t, t) for t in tams])
    for t in [16, 24, 32, 48, 64, 128, 256, 512, 1024]:
        png(red(sq, t, t), f"{B}/Icons/png/{app}_{t}.png")
    png(red(sq, 48, 48), f"{B}/Grub/{app}_48.png")

boot = Image.open(f"{L}/AetherBoot_logo.png")
for w in [320, 480, 640, 960, 1280, 1920]:
    png(red(boot, w), f"{L}/web/AetherBoot_logo_{w}.png")
hz = Image.open(f"{L}/Aether2Disk_logo_horizontal.png")
for w in [320, 480, 640, 960]:
    png(red(hz, w), f"{L}/web/Aether2Disk_logo_horizontal_{w}.png")

# GRUB2: cabecera del menú para 1920x1080 y 1024x768 (el lienzo se escala)
for w in [640, 420]:
    g = red(boot, w)
    g.save(f"{B}/Grub/AetherBoot_cabecera_{w}.png", optimize=True)

# verificación de formato para GRUB
for n in os.listdir(f"{B}/Grub"):
    im = Image.open(f"{B}/Grub/{n}")
    assert im.mode == "RGBA" and not im.info.get("interlace"), n
print("listo")
