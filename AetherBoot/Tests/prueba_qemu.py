#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
prueba_qemu.py — arranca un disco con AetherBoot en QEMU y comprueba que:

  1. aparece el menú de AetherBoot (captura de pantalla con el tema cargado);
  2. al elegir la ISO de prueba, esta arranca y escribe una marca por el puerto
     serie (AETHERBOOT_PRUEBA_OK).

Uso:
  prueba_qemu.py --modo bios|uefi --disco disk.img --salida carpeta
                 [--ovmf-code OVMF_CODE.fd --ovmf-vars OVMF_VARS.fd]

Devuelve 0 si todo salió bien, 1 si falló alguna comprobación.
Solo usa la biblioteca estándar de Python.
"""

import argparse
import os
import shutil
import socket
import struct
import subprocess
import sys
import time
import zlib

MARCA = b"AETHERBOOT_PRUEBA_OK"


def leer_ppm(ruta):
    with open(ruta, "rb") as f:
        datos = f.read()
    # Cabecera P6: "P6\n<ancho> <alto>\n<max>\n"
    partes = []
    i = 0
    while len(partes) < 4:
        while datos[i:i + 1].isspace():
            i += 1
        if datos[i:i + 1] == b"#":
            while datos[i:i + 1] not in (b"\n", b""):
                i += 1
            continue
        j = i
        while not datos[j:j + 1].isspace():
            j += 1
        partes.append(datos[i:j])
        i = j
    i += 1
    if partes[0] != b"P6":
        raise ValueError("no es PPM P6")
    ancho, alto = int(partes[1]), int(partes[2])
    return ancho, alto, datos[i:i + ancho * alto * 3]


def guardar_png(ruta, ancho, alto, rgb):
    def bloque(tipo, contenido):
        c = struct.pack(">I", len(contenido)) + tipo + contenido
        return c + struct.pack(">I", zlib.crc32(tipo + contenido) & 0xFFFFFFFF)

    filas = b"".join(b"\x00" + rgb[y * ancho * 3:(y + 1) * ancho * 3] for y in range(alto))
    png = b"\x89PNG\r\n\x1a\n"
    png += bloque(b"IHDR", struct.pack(">IIBBBBB", ancho, alto, 8, 2, 0, 0, 0))
    png += bloque(b"IDAT", zlib.compress(filas, 9))
    png += bloque(b"IEND", b"")
    with open(ruta, "wb") as f:
        f.write(png)


def contar_colores(rgb):
    colores = set()
    for k in range(0, len(rgb), 3 * 7):  # muestreo
        colores.add(rgb[k:k + 3])
    return len(colores)


class Monitor:
    def __init__(self, ruta):
        self.ruta = ruta
        self.sock = None

    def conectar(self, espera=30):
        fin = time.time() + espera
        while time.time() < fin:
            try:
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                s.connect(self.ruta)
                s.settimeout(2)
                self.sock = s
                self._leer()
                return
            except OSError:
                time.sleep(0.5)
        raise RuntimeError("no se pudo conectar al monitor de QEMU")

    def _leer(self):
        try:
            return self.sock.recv(65536)
        except socket.timeout:
            return b""

    def cmd(self, texto):
        self.sock.sendall(texto.encode() + b"\n")
        time.sleep(0.5)
        return self._leer()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--modo", choices=["bios", "uefi"], required=True)
    ap.add_argument("--disco", required=True)
    ap.add_argument("--salida", required=True)
    ap.add_argument("--ovmf-code")
    ap.add_argument("--ovmf-vars")
    ap.add_argument("--espera-menu", type=int, default=0,
                    help="segundos hasta la captura del menú (0 = automático)")
    ap.add_argument("--espera-iso", type=int, default=240)
    a = ap.parse_args()

    os.makedirs(a.salida, exist_ok=True)
    kvm = os.access("/dev/kvm", os.R_OK | os.W_OK)
    espera_menu = a.espera_menu or (20 if kvm else 60)

    mon = os.path.join(a.salida, "monitor-%s.sock" % a.modo)
    serie = os.path.join(a.salida, "serie-%s.log" % a.modo)
    for f in (mon, serie):
        if os.path.exists(f):
            os.remove(f)

    cmd = ["qemu-system-x86_64", "-m", "1536", "-smp", "2",
           "-drive", "file=%s,format=raw,if=ide" % a.disco,
           "-display", "none", "-vga", "std",
           "-serial", "file:%s" % serie,
           "-monitor", "unix:%s,server,nowait" % mon,
           "-no-reboot"]
    cmd += ["-accel", "kvm", "-cpu", "host"] if kvm else ["-accel", "tcg"]
    if a.modo == "uefi":
        vars_copia = os.path.join(a.salida, "OVMF_VARS-copia.fd")
        shutil.copy(a.ovmf_vars, vars_copia)
        cmd += ["-drive", "if=pflash,format=raw,readonly=on,file=%s" % a.ovmf_code,
                "-drive", "if=pflash,format=raw,file=%s" % vars_copia]

    print("[%s] KVM: %s" % (a.modo, "sí" if kvm else "no (más lento)"))
    print("[%s] %s" % (a.modo, " ".join(cmd)))
    qemu = subprocess.Popen(cmd)
    ok_menu = False
    ok_iso = False
    try:
        m = Monitor(mon)
        m.conectar()

        # 1) Menú de AetherBoot
        time.sleep(espera_menu)
        ppm = os.path.join(a.salida, "menu-%s.ppm" % a.modo)
        m.cmd("screendump %s" % ppm)
        time.sleep(2)
        if os.path.exists(ppm):
            ancho, alto, rgb = leer_ppm(ppm)
            guardar_png(os.path.join(a.salida, "menu-%s.png" % a.modo), ancho, alto, rgb)
            colores = contar_colores(rgb)
            print("[%s] Captura del menú: %dx%d, %d colores distintos" % (a.modo, ancho, alto, colores))
            # El tema de AetherBoot tiene degradados: muchos colores. Una pantalla
            # negra o un error de texto tiene muy pocos.
            ok_menu = colores >= 48
            os.remove(ppm)
        else:
            print("[%s] No se pudo obtener la captura del menú" % a.modo)

        # 2) Elegir la ISO (primera entrada) y esperar la marca por el puerto serie
        fin = time.time() + a.espera_iso
        siguiente_tecla = 0
        n = 0
        while time.time() < fin:
            if qemu.poll() is not None:
                break
            if os.path.exists(serie):
                with open(serie, "rb") as f:
                    if MARCA in f.read():
                        ok_iso = True
                        break
            if time.time() >= siguiente_tecla and n < 6:
                m.cmd("sendkey ret")
                n += 1
                siguiente_tecla = time.time() + 8
                if n == 2:
                    ppm2 = os.path.join(a.salida, "tras-elegir-%s.ppm" % a.modo)
                    m.cmd("screendump %s" % ppm2)
                    time.sleep(1)
                    if os.path.exists(ppm2):
                        w2, h2, rgb2 = leer_ppm(ppm2)
                        guardar_png(os.path.join(a.salida, "tras-elegir-%s.png" % a.modo), w2, h2, rgb2)
                        os.remove(ppm2)
            time.sleep(1)

        if not ok_iso:
            ppm3 = os.path.join(a.salida, "final-%s.ppm" % a.modo)
            try:
                m.cmd("screendump %s" % ppm3)
                time.sleep(1)
                if os.path.exists(ppm3):
                    w3, h3, rgb3 = leer_ppm(ppm3)
                    guardar_png(os.path.join(a.salida, "final-%s.png" % a.modo), w3, h3, rgb3)
                    os.remove(ppm3)
            except OSError:
                pass
    finally:
        if qemu.poll() is None:
            qemu.kill()
        qemu.wait()

    print("[%s] Menú de AetherBoot: %s" % (a.modo, "CORRECTO" if ok_menu else "FALLO"))
    print("[%s] Arranque de la ISO de prueba: %s" % (a.modo, "CORRECTO" if ok_iso else "FALLO"))
    if os.path.exists(serie):
        with open(serie, "rb") as f:
            cola = f.read()[-3000:]
        print("[%s] Últimas líneas del puerto serie:\n%s" % (a.modo, cola.decode("utf-8", "replace")))
    return 0 if (ok_menu and ok_iso) else 1


if __name__ == "__main__":
    sys.exit(main())
