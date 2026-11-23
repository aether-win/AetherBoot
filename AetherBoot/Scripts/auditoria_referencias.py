#!/usr/bin/env python3
"""Auditoría de referencias a Ventoy en el proyecto AetherBoot.

Clasifica cada aparición en:
  A  Visual/comercial visible para el usuario  -> debe sustituirse
  B  Documental o enlace innecesario           -> debe eliminarse o reemplazarse
  C  Técnica imprescindible o legal            -> se conserva (con motivo)

Uso:
  python3 auditoria_referencias.py <carpeta_proyecto> <salida.csv> [<salida.md>]
"""
import csv
import os
import re
import sys
from collections import Counter, defaultdict

RAIZ, CSV_OUT = sys.argv[1], sys.argv[2]
MD_OUT = sys.argv[3] if len(sys.argv) > 3 else None

PATRON = re.compile(r"(ventoy\.net|Ventoy2Disk|VentoyPlugson|VentoyVlnk|VentoyVInk|Ventoy[A-Za-z0-9_]*|ventoy[A-Za-z0-9_]*|VENTOY[A-Z0-9_]*)")
IGNORAR_DIRS = {".git", "node_modules"}
# código de terceros incluido tal cual (GRUB, EDK2, iPXE, BusyBox, etc.):
# su contenido no es interfaz de AetherBoot
TERCEROS = ("GRUB2/MOD_SRC/grub-2.04/grub-core/fs", "EDK2/edk2_mod/edk2-edk2-stable201911/MdePkg",
            "IPXE/", "SQUASHFS/", "BUSYBOX/", "DMSETUP/", "ZSTD/", "LZIP/", "ExFAT/", "cryptsetup/",
            "VBLADE/", "wimboot/", "/Lib/libhttp", "/Lib/exfat", "/Lib/fat_io_lib", "xz-embedded", "ff14/")
EXT_TEXTO = {".c", ".h", ".cfg", ".sh", ".txt", ".md", ".json", ".html", ".htm", ".js", ".css", ".rc",
             ".xml", ".py", ".ini", ".yml", ".yaml", ".glade", ".manifest", ".vcxproj", ".filters", ".sln", ""}
EXT_IMAGEN = {".png", ".ico", ".jpg", ".bmp", ".svg"}

UI_RUTAS = ("Plugson/www/", "LinuxGUI/WebUI/", "INSTALL/grub/", "LANGUAGES/", "INSTALL/plugin/")
UI_RC = (".rc",)


def leer(p):
    b = open(p, "rb").read()
    if b[:2] in (b"\xff\xfe", b"\xfe\xff"):
        return b.decode("utf-16", "replace")
    if b"\x00" in b[:2000]:
        return None
    return b.decode("utf-8", "replace")


COMPILADOS = ("GRUB2/MOD_SRC/", "EDK2/", "IMG/cpio", "vtoyjump/", "VtoyTool/", "Unix/", "INSTALL/EFI")
DOC_TECNICA = ("DOC/", "BLOB_List.md", "SecureBoot.md", "Unix/", "LiveCD/README", ".github/ISSUE_TEMPLATE", ".github/FUNDING")


def clasificar(rel, linea, ref):
    l = linea.strip()
    ext = os.path.splitext(rel)[1].lower()
    lower = ref.lower()

    # --- C: licencias, documentación técnica y documentación propia de AetherBoot
    if rel.startswith("License/") or rel in ("COPYING",):
        return "C", "Licencia de un componente (información legal obligatoria)"
    if rel.startswith("AetherBoot/"):
        return "C", "Documentación o scripts de AetherBoot: explican la procedencia y los nombres técnicos conservados"
    if rel.startswith(DOC_TECNICA):
        return "C", "Documentación técnica para desarrolladores (procedencia del código y de los binarios)"
    # --- C: identificadores (MAYÚSCULAS, con guion bajo o pegados a otra palabra)
    if ref != "ventoy.net" and (re.fullmatch(r"[A-Z0-9_]+", ref) or "_" in ref or re.fullmatch(r"(?i)ventoy[a-z0-9]+", ref) and ref not in ("Ventoy2Disk", "VentoyPlugson", "VentoyVlnk", "VentoyVInk")):
        return "C", "Identificador, nombre de archivo o clave interna"
    # --- C: nombres de archivo de scripts/programas conservados
    if re.search(re.escape(ref) + r"\.(sh|exe|log|ini)\b", l):
        return "C", "Nombre de archivo técnico conservado por compatibilidad (los lanzadores Aether* lo llaman)"
    # --- C: nombres de funciones/variables que contienen la palabra
    if re.search(r"\w_" + re.escape(ref) + r"\b|\b" + re.escape(ref) + r"_\w|\b" + re.escape(ref) + r"\s*\(", l):
        return "C", "Identificador, nombre de archivo o clave interna"
    if rel in ("INSTALL/ventoy_pack.sh", "INSTALL/all_in_one.sh", "INSTALL/docker_ci_build.sh") or rel.startswith(".github/workflows/ci"):
        return "C", "Script de compilación oficial (mensajes para desarrolladores)"
    if re.search(r"etiqueta.*Ventoy|Label.*Ventoy|LABEL=Ventoy", l):
        return "C", "Etiqueta de partición que usan el motor y los scripts para localizar el USB"
    # --- C: textos dentro de binarios de arranque compilados/firmados
    if rel.startswith(COMPILADOS):
        return "C", "Texto dentro de un binario de arranque compilado (GRUB/shim/cpio): cambiarlo exige recompilar y volver a firmar, lo que rompería Secure Boot"
    # --- C: textos de las ramas de idioma inactivas (chino)
    if re.search(r"[\u4e00-\u9fff]", l):
        return "C", "Texto de la rama en chino, inactiva (las interfaces se fijan en español)"
    # --- C: tablas de mensajes en inglés que ya no se usan
    if rel.endswith(("Plugson/src/main_windows.c", "Vlnk/src/main_windows.c")) and re.match(r'L"', l):
        return "C", "Tabla de mensajes en inglés inactiva (se usa la tabla en español)"
    # --- C: sección/archivo INI y nombres de proyecto/ejecutable internos
    if re.search(r'TEXT\("Ventoy"\)|\[Ventoy\]', l):
        return "C", "Sección del archivo de configuración INI (compatibilidad con instalaciones previas)"
    if re.search(r"/V:Ventoy|0:Ventoy|f_setlabel", l):
        return "C", "Etiqueta de partición que usan el motor y los scripts para localizar el USB"
    if rel.startswith(("LinuxGUI/", "Plugson/src", "Ventoy2Disk/", "Vlnk/src", "vtoycli", "vtoygpt", "vtoyfat")) and re.search(r"printf|vlog|Log\(|LogA|LogW|debug", l) and "Aether" not in l:
        return "C", "Mensaje de registro interno (log), no visible en la interfaz"

    # --- C: legales / atribución
    if re.search(r"copyright|\(c\)|longpanda|gpl|licen|basad[oa] en la tecnolog|based on", l, re.I):
        return "C", "Aviso de copyright, licencia o atribución obligatoria"
    # --- C: nombres de archivo/carpeta y formatos de configuración del motor
    if re.search(r"ventoy\.json|ventoy_grub\.cfg|/ventoy/|\\\\ventoy\\\\|ventoy\\\\|\bventoy\b(?= (directory|dir|目录|folder))|carpeta ventoy|ventoy_backup|ventoy\.disk|ventoy_wimboot|ventoy_vhdboot|ventoy\.cpio|ventoy_openwrt|\.vtoy\b", l):
        return "C", "Nombre de archivo, carpeta o formato que el motor busca (compatibilidad)"
    if re.search(r"VTOYEFI|LABEL=Ventoy|L\"Ventoy\"|Label = L\"Ventoy\"|\"Ventoy\", VDS|FormatEx2", l):
        return "C", "Etiqueta de partición que usan el motor y los scripts para localizar el USB"
    if "VENTOY_GUID" in l or "ventoy_guid" in l:
        return "C", "Identificador binario (GUID) compartido entre GRUB, vtoyjump y los instaladores"
    if lower.startswith("ventoy.net") or "www.ventoy.net" in l or "ventoy.net/" in l:
        if ext in (".c", ".h") and "admin@ventoy.net" in l:
            return "C", "Aviso de copyright del autor original"
        if any(rel.startswith(u) for u in UI_RUTAS) or ext in UI_RC:
            return "B", "Enlace a la web de Ventoy en una interfaz"
        if ext in (".md", ".txt") or rel.startswith("DOC/"):
            return "B", "Enlace a la web de Ventoy en documentación"
        return "C", "URL en comentario o código interno, no visible"
    # --- código: identificadores, rutas internas, nombres de ejecutables internos
    if ext in (".c", ".h", ".py", ".sh", ".vcxproj", ".filters", ".sln", ".manifest", ".yml", ".yaml"):
        if ext in (".c", ".h") and re.search(r'(L?"[^"]*\b(Ventoy)\b[^"]*")', l) and re.search(r"(MessageBox|printf|echo|SetWindowText|L\"|TEXT\()", l):
            if re.search(r"Log\(|vlog|debug|vdebug|grub_printf\(\"\\n\\n=====|LogA|LogW|fprintf\(stderr", l):
                return "C", "Mensaje de registro interno (log), no visible en la interfaz"
            return "A", "Texto visible en un programa compilado"
        if ext == ".sh" and re.search(r"\becho\b|vterr|vtwarn|vtinfo", l) and re.search(r"\bVentoy\b", l) and ">>" not in l:
            return "A", "Mensaje visible en un script"
        return "C", "Identificador, variable, función, ruta o nombre interno del motor"
    if ext == ".cfg":
        if re.search(r"(echo|menuentry|submenu)\b", l) and re.search(r"\bVentoy\b", l):
            return "A", "Texto visible en el menú de arranque"
        return "C", "Variable, comando o ruta de GRUB usada por el motor"
    if ext == ".rc":
        if re.search(r"CAPTION|PUSHBUTTON|LTEXT|GROUPBOX|CONTROL|VALUE \"(ProductName|FileDescription|Comments)\"", l):
            return "A", "Texto o metadato visible del ejecutable"
        return "C", "Definición de recurso interna"
    if rel.endswith("theme.txt"):
        return "C", "Ruta de configuración del tema (carpeta /ventoy/)"
    if ext in (".html", ".htm", ".js", ".css", ".json", ".glade"):
        if re.search(r"function |var |\.js|\.css|id=|class=|callVtoy|ventoy_[a-z_]+\(|vtoy_|#|\$\(", l) and not re.search(r">[^<]*\bVentoy\b[^<]*<", l):
            return "C", "Identificador o ruta de la interfaz web"
        if re.search(r"[一-鿿]", l):
            return "C", "Texto de la rama en chino, inactiva (la interfaz se fija en español)"
        return "A", "Texto visible en la interfaz"
    if ext in (".md", ".txt") or os.path.basename(rel) in ("README", "README.txt"):
        if re.search(r"ventoy\.net", l):
            return "B", "Enlace a la web de Ventoy en documentación"
        return "B", "Mención en documentación"
    if os.path.basename(rel) in ("Makefile", "CMakeLists.txt") or ext in (".pro", ".mk"):
        return "C", "Archivo de compilación (nombres de ejecutables y rutas internas)"
    if rel == ".gitignore":
        return "C", "Configuración del repositorio"
    return "C", "Otro uso interno"


filas = []
imagenes = []
for d, ds, fs in os.walk(RAIZ):
    ds[:] = [x for x in ds if x not in IGNORAR_DIRS]
    for f in fs:
        p = os.path.join(d, f)
        rel = os.path.relpath(p, RAIZ).replace(os.sep, "/")
        ext = os.path.splitext(f)[1].lower()
        if ext in EXT_IMAGEN and re.search(r"ventoy", f, re.I):
            imagenes.append(rel)
        if any(t in rel for t in TERCEROS):
            continue
        if ext not in EXT_TEXTO or os.path.getsize(p) > 3_000_000:
            continue
        if rel.startswith("AetherBoot/Scripts/auditoria") or rel.startswith("AetherBoot/Informes/"):
            continue
        txt = leer(p)
        if txt is None:
            continue
        for n, linea in enumerate(txt.splitlines(), 1):
            for m in PATRON.finditer(linea):
                cat, motivo = clasificar(rel, linea, m.group(0))
                filas.append((rel, n, m.group(0), cat, motivo, linea.strip()[:180]))

for rel in imagenes:
    en_ui = any(rel.startswith(u) for u in UI_RUTAS) or "/Res/" in rel
    filas.append((rel, 0, os.path.basename(rel), "A" if en_ui else "C",
                  "Recurso gráfico original en una interfaz" if en_ui else "Recurso gráfico original fuera de la interfaz (no se distribuye en AetherBoot)", ""))

with open(CSV_OUT, "w", newline="", encoding="utf-8") as fh:
    w = csv.writer(fh)
    w.writerow(["archivo", "linea", "referencia", "categoria", "motivo", "contexto"])
    w.writerows(filas)

c = Counter(r[3] for r in filas)
print(f"Total: {len(filas)}  A={c['A']}  B={c['B']}  C={c['C']}")

if MD_OUT:
    por_motivo = defaultdict(Counter)
    for r in filas:
        por_motivo[r[3]][r[4]] += 1
    with open(MD_OUT, "w", encoding="utf-8") as fh:
        fh.write(f"Total de referencias: {len(filas)} (A={c['A']}, B={c['B']}, C={c['C']})\n\n")
        for cat in "ABC":
            fh.write(f"### Categoría {cat}\n\n| Motivo | Apariciones |\n|---|---|\n")
            for mot, k in por_motivo[cat].most_common():
                fh.write(f"| {mot} | {k} |\n")
            fh.write("\n")
        pend = [r for r in filas if r[3] in "AB"]
        fh.write(f"### Referencias A/B restantes ({len(pend)})\n\n")
        if pend:
            fh.write("| Archivo | Línea | Referencia | Motivo | Contexto |\n|---|---|---|---|---|\n")
            for r in pend[:400]:
                ctx = r[5].replace("|", "\\|")
                fh.write(f"| {r[0]} | {r[1]} | {r[2]} | {r[4]} | `{ctx[:120]}` |\n")
