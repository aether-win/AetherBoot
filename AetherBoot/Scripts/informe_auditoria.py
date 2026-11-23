#!/usr/bin/env python3
"""Genera el informe de auditoría comparando el original con AetherBoot.
Uso: informe_auditoria.py <original.csv> <aetherboot.csv> <salida.md>"""
import csv, sys
from collections import Counter, defaultdict
orig = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
new = list(csv.DictReader(open(sys.argv[2], encoding='utf-8')))
co, cn = Counter(r['categoria'] for r in orig), Counter(r['categoria'] for r in new)
o_ab = defaultdict(list)
for r in orig:
    if r['categoria'] in 'AB': o_ab[r['archivo']].append(r)
n_files = {r['archivo'] for r in new}
out = open(sys.argv[3], 'w', encoding='utf-8')
w = out.write
w("# Informe de auditoría de referencias a Ventoy\n\n")
w("Generado por `AetherBoot/Scripts/auditoria_referencias.py`, que recorre todos los archivos de texto del proyecto "
  "(incluidos los UTF-16 de Windows) y los nombres de las imágenes, buscando `Ventoy`, `Ventoy2Disk`, `VentoyPlugson`, "
  "`VentoyVlnk`, `ventoy.net` y variantes. Se excluye el código de terceros incluido tal cual (GRUB base, EDK2, iPXE, "
  "BusyBox, etc.), cuyo contenido no forma parte de ninguna interfaz.\n\n")
w("## Resumen\n\n| Categoría | Significado | Original | AetherBoot |\n|---|---|---|---|\n")
w(f"| A | Visual o comercial visible para el usuario: sustituir | {co['A']} | **{cn['A']}** |\n")
w(f"| B | Documental o enlace innecesario: eliminar o reemplazar | {co['B']} | **{cn['B']}** |\n")
w(f"| C | Técnica o legal imprescindible: conservar | {co['C']} | {cn['C']} |\n\n")
w(f"**No se eliminaron todas las referencias.** Quedan {cn['C']} de categoría C, que son necesarias para el "
  "funcionamiento, la compatibilidad o el cumplimiento de la licencia. Abajo se explica cada motivo. "
  "La lista completa (archivo, línea, referencia, categoría, motivo y contexto) está en `auditoria_referencias.csv`.\n\n")
w("## Referencias A y B tratadas, por archivo\n\n| Archivo | A+B en el original | Acción |\n|---|---|---|\n")
for f, rs in sorted(o_ab.items(), key=lambda t: -len(t[1])):
    if f == 'README.md':
        acc = 'Sustituido por un README de AetherBoot; el original se conserva en `AetherBoot/Documentation/original/`'
    elif f not in n_files and not any(r['archivo'] == f for r in new):
        acc = 'Eliminado o sustituido (ya no contiene referencias A/B)'
    else:
        acc = 'Textos visibles sustituidos por AetherBoot / español; enlaces eliminados'
    if all(r['categoria'] == 'B' for r in rs) and f.startswith(('DOC/', 'BLOB', 'SecureBoot', 'Unix/', 'License/', 'LiveCD/README')):
        acc = 'Reclasificado como C: documentación técnica o licencia que debe conservarse'
    w(f"| `{f}` | {len(rs)} | {acc} |\n")
w("\n## Referencias conservadas (C), por motivo\n\n| Motivo | Apariciones | Ejemplo |\n|---|---|---|\n")
por = defaultdict(list)
for r in new: por[r['motivo']].append(r)
for mot, rs in sorted(por.items(), key=lambda t: -len(t[1])):
    ej = rs[0]; ctx = ej['contexto'][:70].replace('|', '\\|')
    w(f"| {mot} | {len(rs)} | `{ej['archivo']}`: `{ctx}` |\n")
w("\n## Casos técnicos destacados\n\n"
  "- **Etiqueta de partición `Ventoy` y partición `VTOYEFI`:** el GRUB firmado comprueba la etiqueta `VTOYEFI` "
  "(si no coincide muestra \"[Unofficial …]\"), y los sistemas en vivo buscan la partición de datos por la etiqueta `Ventoy`.\n"
  "- **Carpeta `ventoy/` y `ventoy.json`:** el motor busca exactamente esas rutas; cambiarlas desactivaría todos los plugins.\n"
  "- **GUID `VENTOY_GUID`:** estructura binaria compartida entre GRUB, vtoyjump y los instaladores para pasar parámetros al sistema arrancado.\n"
  "- **Textos dentro de GRUB, shim y `ventoy.cpio`:** son binarios firmados para Secure Boot; recompilarlos sin la clave original rompería el arranque seguro.\n"
  "- **Etiqueta de versión inyectada por GRUB** (contiene \"www.ventoy.net\" dentro del GUID): no se puede quitar sin recompilar; "
  "se coloca fuera de la pantalla con las variables `VTLE_LFT/TOP/CLR` y el tema muestra la versión de AetherBoot.\n"
  "- **Avisos de copyright y licencias:** obligatorios por la GPLv3.\n")
out.close()
print('ok')
