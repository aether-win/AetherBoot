#!/usr/bin/env python3
"""Extrae los textos visibles de la rama inglesa ('en') de Plugson."""
import re, glob, json, sys
CJK = re.compile(r'[　-鿿＀-￯]')
LIT = re.compile(r"'((?:[^'\\\n]|\\.)*)'|\"((?:[^\"\\\n]|\\.)*)\"")

def bloque(s, i):
    """Devuelve el bloque {...} o [...] que empieza en s[i]."""
    a = s[i]; c = '}' if a == '{' else ']'
    d = 0
    for j in range(i, len(s)):
        if s[j] == a: d += 1
        elif s[j] == c:
            d -= 1
            if d == 0: return s[i:j + 1]
    return s[i:]

def visible(t):
    if not t or CJK.search(t) or not re.search(r'[A-Za-z]{2,}', t): return False
    if re.match(r'^\s*[#.\[]', t) or re.search(r'\[id=|\[name=|^(span|input|div|tr|td|th|li|a|b|label)\b', t): return False
    if re.fullmatch(r'[a-z0-9_\-]+', t) or re.fullmatch(r'[A-Z0-9_]+', t): return False
    if t.startswith(('http', 'fa ', 'fa-', 'btn', 'id_', 'STR_', 'vtoy_', 'static/')): return False
    return True

def textos_en_codigo(code, dest, f):
    for m in LIT.finditer(code):
        t = m.group(1) if m.group(1) is not None else m.group(2)
        if visible(t): dest.setdefault(t, set()).add(f)

out = {}
for f in sorted(glob.glob('*.html')) + ['static/js/vtoy.js']:
    s = open(f, encoding='utf-8').read()
    for m in re.finditer(r"(?:newlang|g_current_language|lang)\s*===?\s*['\"]en['\"]\s*\)\s*\{", s):
        textos_en_codigo(bloque(s, m.end() - 1), out, f)
    for m in re.finditer(r"===?\s*['\"]en['\"]\s*\?\s*((?:'[^']*'|\"[^\"]*\"))", s):
        textos_en_codigo(m.group(1), out, f)
    # diccionarios ingleses
    for m in re.finditer(r'(?:g_vtoy_cur_language_en|device_language_en)\s*=\s*[\{\[]', s):
        textos_en_codigo(bloque(s, m.end() - 1), out, f)
    # HTML estático de los elementos con id="..._en"
    for m in re.finditer(r'<(\w+)[^>]*\bid="[^"]*_en"[^>]*>', s):
        tag = m.group(1); ini = m.end(); prof = 1; j = ini
        pat = re.compile(r'<(/?)' + tag + r'\b[^>]*>')
        for t in pat.finditer(s, ini):
            prof += -1 if t.group(1) else 1
            if prof == 0: j = t.start(); break
        trozo = s[ini:j]
        for tx in re.split(r'<[^>]+>', trozo):
            tx = ' '.join(tx.split())
            if visible(tx): out.setdefault(tx, set()).add(f + ' (html)')
json.dump({k: sorted(v) for k, v in out.items()}, open(sys.argv[1], 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print(len(out), 'textos únicos;', sum(len(k) for k in out), 'caracteres')
