#!/usr/bin/env python3
"""Aplica las traducciones al español a la interfaz web de Plugson (AetherPlugson).

Solo reemplaza dentro de los contextos de idioma inglés:
  * bloques  if (... === 'en') { ... }  y ternarios  === 'en' ? '...'
  * diccionarios/arreglos g_vtoy_cur_language_en y device_language_en
  * texto estático de los elementos HTML con id="..._en"
Así no se tocan identificadores ni valores que use la lógica.

Uso: python3 aplicar.py <carpeta www>
"""
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from traducciones import ES  # noqa: E402

WWW = sys.argv[1]
LIT = re.compile(r"'((?:[^'\\\n]|\\.)*)'|\"((?:[^\"\\\n]|\\.)*)\"")
cuenta = {"js": 0, "html": 0}


def fin_bloque(s, i):
    a = s[i]
    c = '}' if a == '{' else ']'
    d = 0
    for j in range(i, len(s)):
        if s[j] == a:
            d += 1
        elif s[j] == c:
            d -= 1
            if d == 0:
                return j + 1
    return len(s)


def traducir_codigo(code):
    def r(m):
        if m.group(1) is not None:
            t = m.group(1)
            if t in ES:
                cuenta["js"] += 1
                return "'" + ES[t].replace("\\", "\\\\").replace("'", "\\'") + "'"
        else:
            t = m.group(2)
            if t in ES:
                cuenta["js"] += 1
                return '"' + ES[t].replace("\\", "\\\\").replace('"', '\\"') + '"'
        return m.group(0)
    return LIT.sub(r, code)


def traducir_html(trozo):
    def r(m):
        texto = m.group(0)
        norm = " ".join(texto.split())
        if norm in ES:
            cuenta["html"] += 1
            ini = texto[: len(texto) - len(texto.lstrip())]
            fin = texto[len(texto.rstrip()):]
            return ini + ES[norm] + fin
        return texto
    return re.sub(r"[^<>]+", r, trozo)


def procesar(s):
    regiones = []
    for m in re.finditer(r"(?:newlang|g_current_language|lang)\s*===?\s*['\"]en['\"]\s*\)\s*\{", s):
        regiones.append((m.end() - 1, fin_bloque(s, m.end() - 1), "js"))
    for m in re.finditer(r"===?\s*['\"]en['\"]\s*\?\s*(?:'[^']*'|\"[^\"]*\")", s):
        regiones.append((m.start(), m.end(), "js"))
    for m in re.finditer(r"(?:g_vtoy_cur_language_en|device_language_en)\s*=\s*[\{\[]", s):
        regiones.append((m.end() - 1, fin_bloque(s, m.end() - 1), "js"))
    for m in re.finditer(r'<(\w+)[^>]*\bid="[^"]*_en"[^>]*>', s):
        tag = m.group(1)
        prof, j = 1, len(s)
        for t in re.finditer(r"<(/?)" + tag + r"\b[^>]*>", s[m.end():]):
            prof += -1 if t.group(1) else 1
            if prof == 0:
                j = m.end() + t.start()
                break
        regiones.append((m.end(), j, "html"))

    # aplicar de atrás hacia delante sin solapamientos
    regiones.sort(key=lambda x: x[0])
    limpias = []
    for r in regiones:
        if limpias and r[0] < limpias[-1][1]:
            continue
        limpias.append(r)
    for a, b, tipo in reversed(limpias):
        trozo = s[a:b]
        s = s[:a] + (traducir_codigo(trozo) if tipo == "js" else traducir_html(trozo)) + s[b:]
    return s


for f in sorted(glob.glob(os.path.join(WWW, "*.html"))) + [os.path.join(WWW, "static/js/vtoy.js")]:
    s = open(f, encoding="utf-8", newline="").read()
    n = procesar(s)
    if n != s:
        open(f, "w", encoding="utf-8", newline="").write(n)

print("traducciones aplicadas: código", cuenta["js"], "| html", cuenta["html"])
