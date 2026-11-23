#!/usr/bin/env python3
"""Generate the "moderno" GRUB theme for Ventoy (all artwork is procedural/original).

Usage: python3 make_theme.py <output_dir>
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1]
ICONS = os.path.join(OUT, "icons")
os.makedirs(ICONS, exist_ok=True)

ACCENT = (20, 184, 166)        # teal
ACCENT2 = (56, 189, 248)       # sky
BG_TOP = (11, 17, 32)
BG_BOT = (20, 30, 52)
PANEL = (255, 255, 255, 14)
BORDER = (255, 255, 255, 34)


def save(img, name, folder=OUT):
    # GRUB's PNG loader wants plain 8-bit RGB/RGBA, non-interlaced
    if img.mode not in ("RGB", "RGBA"):
        img = img.convert("RGBA")
    img.save(os.path.join(folder, name), optimize=True)


# ---------------------------------------------------------------- background
def background(w=1920, h=1080):
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(0, w):
            t = (y / h) * 0.75 + (x / w) * 0.25
            px[x, y] = tuple(int(BG_TOP[i] + (BG_BOT[i] - BG_TOP[i]) * t) for i in range(3))

    glow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    g = ImageDraw.Draw(glow)
    g.ellipse((w * 0.62, -h * 0.35, w * 1.25, h * 0.55), fill=ACCENT + (60,))
    g.ellipse((-w * 0.25, h * 0.65, w * 0.30, h * 1.35), fill=ACCENT2 + (38,))
    glow = glow.filter(ImageFilter.GaussianBlur(160))
    img = Image.alpha_composite(img.convert("RGBA"), glow)

    # thin accent line and subtle dotted grid in the corners
    d = ImageDraw.Draw(img)
    d.line((w * 0.15, h * 0.175, w * 0.85, h * 0.175), fill=ACCENT + (120,), width=2)
    for gx in range(40, 400, 28):
        for gy in range(40, 220, 28):
            d.ellipse((w - gx - 2, h - gy - 2, w - gx + 1, h - gy + 1), fill=(255, 255, 255, 18))
    return img.convert("RGB")


# ------------------------------------------------------------- 9-slice frames
def nine_slice(prefix, size, radius, fill, border=None, border_w=1):
    """Draw a rounded rect of size x size and cut it into GRUB 9-slice pieces."""
    s = size
    big = Image.new("RGBA", (s * 4, s * 4), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.rounded_rectangle((0, 0, s * 4 - 1, s * 4 - 1), radius=radius * 4, fill=fill,
                        outline=border, width=border_w * 4 if border else 0)
    img = big.resize((s, s), Image.LANCZOS)
    c = radius  # corner size
    m = s - 2 * c
    boxes = {
        "nw": (0, 0, c, c), "n": (c, 0, c + m, c), "ne": (c + m, 0, s, c),
        "w": (0, c, c, c + m), "c": (c, c, c + m, c + m), "e": (c + m, c, s, c + m),
        "sw": (0, c + m, c, s), "s": (c, c + m, c + m, s), "se": (c + m, c + m, s, s),
    }
    for k, b in boxes.items():
        save(img.crop(b), f"{prefix}_{k}.png")


# --------------------------------------------------------------------- icons
def icon_canvas():
    return Image.new("RGBA", (128, 128), (0, 0, 0, 0))


def finish(img, name):
    save(img.resize((32, 32), Image.LANCZOS), name + ".png", ICONS)


def ic_disc(color=ACCENT):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    d.ellipse((8, 8, 120, 120), fill=color + (255,))
    d.ellipse((24, 24, 104, 104), outline=(255, 255, 255, 70), width=4)
    d.ellipse((48, 48, 80, 80), fill=(15, 23, 42, 255))
    d.ellipse((58, 58, 70, 70), fill=color + (255,))
    d.arc((16, 16, 112, 112), 200, 250, fill=(255, 255, 255, 170), width=6)
    return im


def ic_layers(color=ACCENT2):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    for i, a in enumerate((110, 180, 255)):
        y = 78 - i * 24
        d.polygon([(64, y - 26), (118, y), (64, y + 26), (10, y)], fill=color + (a,))
    return im


def ic_chip(color=ACCENT):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    d.rounded_rectangle((30, 30, 98, 98), radius=10, fill=color + (255,))
    d.rounded_rectangle((46, 46, 82, 82), radius=4, fill=(15, 23, 42, 255))
    for p in range(40, 92, 16):
        d.rectangle((p, 12, p + 6, 28), fill=color + (255,))
        d.rectangle((p, 100, p + 6, 116), fill=color + (255,))
        d.rectangle((12, p, 28, p + 6), fill=color + (255,))
        d.rectangle((100, p, 116, p + 6), fill=color + (255,))
    return im


def ic_drive(color=(148, 163, 184), led=ACCENT):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    d.rounded_rectangle((10, 36, 118, 96), radius=12, fill=color + (255,))
    d.line((22, 74, 80, 74), fill=(15, 23, 42, 160), width=5)
    d.ellipse((92, 62, 106, 76), fill=led + (255,))
    return im


def ic_cube(color=ACCENT):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    top = [(64, 12), (112, 38), (64, 64), (16, 38)]
    left = [(16, 38), (64, 64), (64, 118), (16, 92)]
    right = [(112, 38), (64, 64), (64, 118), (112, 92)]
    d.polygon(top, fill=tuple(min(255, c + 60) for c in color) + (255,))
    d.polygon(left, fill=color + (255,))
    d.polygon(right, fill=tuple(int(c * 0.65) for c in color) + (255,))
    return im


def ic_folder(color=(251, 191, 36)):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    d.rounded_rectangle((10, 26, 60, 50), radius=8, fill=tuple(int(c * 0.8) for c in color) + (255,))
    d.rounded_rectangle((10, 38, 118, 108), radius=10, fill=color + (255,))
    return im


def ic_back(color=(148, 163, 184)):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    d.ellipse((8, 8, 120, 120), outline=color + (255,), width=8)
    d.polygon([(36, 64), (66, 36), (66, 52), (94, 52), (94, 76), (66, 76), (66, 92)], fill=color + (255,))
    return im


def ic_gear(color=ACCENT2):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    cx = cy = 64
    pts = []
    for i in range(16):
        a = i * math.pi / 8
        r = 56 if i % 2 == 0 else 42
        for da in (-0.17, 0.17):
            pts.append((cx + r * math.cos(a + da), cy + r * math.sin(a + da)))
    d.polygon(pts, fill=color + (255,))
    d.ellipse((40, 40, 88, 88), fill=color + (255,))
    d.ellipse((50, 50, 78, 78), fill=(15, 23, 42, 255))
    return im


def ic_power(color=(248, 113, 113)):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    d.arc((14, 18, 114, 118), 300, 240, fill=color + (255,), width=12)
    d.rounded_rectangle((58, 6, 70, 64), radius=6, fill=color + (255,))
    return im


def ic_shield(color=(74, 222, 128)):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    d.polygon([(64, 8), (112, 26), (106, 80), (64, 120), (22, 80), (16, 26)], fill=color + (255,))
    d.line([(40, 64), (58, 82), (90, 46)], fill=(15, 23, 42, 255), width=12, joint="curve")
    return im


def ic_wrench(color=(148, 163, 184)):
    im = icon_canvas(); d = ImageDraw.Draw(im)
    d.line((30, 98, 84, 44), fill=color + (255,), width=18)
    d.ellipse((70, 14, 118, 62), fill=color + (255,))
    d.ellipse((88, 20, 112, 44), fill=(0, 0, 0, 0))
    im2 = Image.new("RGBA", im.size, (0, 0, 0, 0))
    im2.paste(im, (0, 0))
    ImageDraw.Draw(im2).ellipse((90, 18, 114, 42), fill=(0, 0, 0, 0))
    return im2


# --------------------------------------------------------------------- build
save(background(), "background.png")
nine_slice("menu", 48, 16, PANEL, BORDER)
nine_slice("select", 24, 8, ACCENT + (215,))
nine_slice("terminal_box", 48, 12, (11, 17, 32, 245), (51, 65, 85, 255))

# scrollbar thumb (n / c / s)
thumb = Image.new("RGBA", (8 * 4, 24 * 4), (0, 0, 0, 0))
ImageDraw.Draw(thumb).rounded_rectangle((0, 0, 31, 95), radius=16, fill=ACCENT + (200,))
thumb = thumb.resize((8, 24), Image.LANCZOS)
save(thumb.crop((0, 0, 8, 4)), "slider_n.png")
save(thumb.crop((0, 4, 8, 20)), "slider_c.png")
save(thumb.crop((0, 20, 8, 24)), "slider_s.png")

icons = {
    "vtoyiso": ic_disc(), "vtoywim": ic_layers(), "vtoyefi": ic_chip(),
    "vtoyimg": ic_drive(), "vtoyvhd": ic_drive(led=ACCENT2), "vtoyvtoy": ic_cube(),
    "vtoydir": ic_folder(), "vtoyret": ic_back(), "F5tool": ic_gear(), "F4boot": ic_power(),
    "boot_disk": ic_drive(), "boot_uefi": ic_chip(ACCENT2), "boot_windows": ic_drive(led=ACCENT2),
    "custom": ic_wrench(), "customsub": ic_folder(ACCENT2),
}
for k in ("checksum_md5", "checksum_sha1", "checksum_sha256", "checksum_sha512"):
    icons[k] = ic_shield()
for name, im in icons.items():
    finish(im, name)

print("theme written to", OUT)
