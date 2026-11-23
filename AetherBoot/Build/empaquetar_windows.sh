#!/bin/bash
# ============================================================================
#  empaquetar_windows.sh — arma el paquete de Windows de AetherBoot.
#
#  Parte del paquete oficial de Ventoy (misma versión) y sustituye:
#    * ventoy/ventoy.disk.img.xz  -> partición de arranque AetherBoot
#    * ventoy/plugson.tar.xz      -> interfaz web de AetherPlugson
#    * ventoy/languages.json      -> textos de Aether2Disk (español por defecto)
#    * plugin/                    -> temas y ejemplos de AetherBoot
#    * ejecutables                -> Aether2Disk / AetherPlugson / AetherVInk compilados
#  y añade documentación, licencias y VerificarISOs.ps1.
#
#  Uso:
#    empaquetar_windows.sh <ventoy-X-windows.zip> <ventoy.disk.img.xz AetherBoot> <salida.zip> [carpeta_exe]
#
#  carpeta_exe (opcional) debe contener los .exe compilados desde este código:
#    Ventoy2Disk.exe Ventoy2Disk_X64.exe Ventoy2Disk_ARM64.exe [Ventoy2Disk_ARM.exe]
#    VentoyPlugson.exe VentoyPlugson_X64.exe VentoyVlnk.exe
#  Si no se indica, se conservan los ejecutables originales (paquete de prueba)
#  y se añade un aviso.
# ============================================================================
set -euo pipefail

ZIP_ORIG="$(realpath "$1")"; IMG="$(realpath "$2")"; SALIDA="$(realpath -m "$3")"; EXE="${4:+$(realpath "$4")}"
RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

echo "==> Descomprimiendo paquete original"
unzip -q "$ZIP_ORIG" -d "$TMP/orig"
SRC="$(find "$TMP/orig" -mindepth 1 -maxdepth 1 -type d | head -n1)"
VER="$(cat "$SRC/ventoy/version" | tr -d '\r\n ')"
DST="$TMP/aetherboot-$VER"
cp -a "$SRC" "$DST"

echo "==> Partición de arranque AetherBoot"
xz -t "$IMG"
cp "$IMG" "$DST/ventoy/ventoy.disk.img.xz"

echo "==> Interfaz web de AetherPlugson (plugson.tar.xz)"
mkdir -p "$TMP/plug"
cp -a "$RAIZ/Plugson/www" "$TMP/plug/www"
(
    cd "$TMP/plug"
    fecha="$(date '+%Y%m%d %H:%M:%S')"
    sed "s#.*plugson_build_date.*#                <b id=\"plugson_build_date\">AetherBoot $VER &middot; $fecha</b>#" -i ./www/index.html
    rm -f www/helplist www/menulist
    ls -1 "$RAIZ/INSTALL/grub/help/" | while read -r l; do printf '%s' "${l:0:5}" >> www/helplist; done
    ls -1 "$RAIZ/INSTALL/grub/menu/" | while read -r l; do printf '%s' "${l:0:5}" >> www/menulist; done
    printf '%s' "$fecha" > www/buildtime
    tar cf www.tar www
    xz --check=crc32 www.tar
)
cp "$TMP/plug/www.tar.xz" "$DST/ventoy/plugson.tar.xz"

echo "==> Textos e idiomas"
cp "$RAIZ/LANGUAGES/languages.json" "$DST/ventoy/languages.json"

echo "==> Plugins, temas y ejemplos"
rm -rf "$DST/plugin"
cp -a "$RAIZ/INSTALL/plugin" "$DST/plugin"
cp "$RAIZ/INSTALL/FOR_X64_ARM.txt" "$DST/FOR_X64_ARM.txt"
cp "$RAIZ/INSTALL/VerificarISOs.ps1" "$DST/VerificarISOs.ps1"

echo "==> Documentación y licencias"
mkdir -p "$DST/Documentacion" "$DST/Licencias"
cp "$RAIZ"/AetherBoot/Documentation/[0-9]*.md "$DST/Documentacion/"
cp "$RAIZ/COPYING" "$DST/Licencias/COPYING-GPLv3.txt"
cp "$RAIZ"/License/*.txt "$DST/Licencias/"
cat > "$DST/LEEME.txt" <<EOF
AetherBoot $VER - Gestor de arranque multisistema
==================================================

Aether2Disk.exe ......... instala o actualiza AetherBoot en una memoria USB
AetherPlugson.exe ....... configura los plugins y opciones avanzadas
AetherVInk.exe .......... crea enlaces .vlnk a imágenes de otros discos
VerificarISOs.ps1 ....... comprueba las sumas de verificación de las imágenes del USB
Documentacion\\ ......... manuales en español
Licencias\\ ............. GNU GPL v3 y licencias de los componentes de terceros

AetherBoot es una distribución personalizada basada en la tecnología Ventoy
(c) longpanda, bajo licencia GNU GPL v3 o posterior. El código fuente
completo, con las modificaciones de AetherBoot, se distribuye junto a este paquete.
EOF

echo "==> Ejecutables"
renombrar() {   # $1 origen (nombre Ventoy)  $2 destino (nombre Aether)
    if [ -f "$DST/$1" ]; then mv "$DST/$1" "$DST/$2"; fi
}
if [ -n "$EXE" ]; then
    for f in Ventoy2Disk.exe VentoyPlugson.exe VentoyVlnk.exe; do
        [ -f "$EXE/$f" ] || { echo "Falta $EXE/$f"; exit 1; }
    done
    cp "$EXE/Ventoy2Disk.exe"   "$DST/Ventoy2Disk.exe"
    cp "$EXE/VentoyPlugson.exe" "$DST/VentoyPlugson.exe"
    cp "$EXE/VentoyVlnk.exe"    "$DST/VentoyVlnk.exe"
    rm -f "$DST"/altexe/*.exe
    for f in Ventoy2Disk_X64.exe Ventoy2Disk_ARM64.exe Ventoy2Disk_ARM.exe VentoyPlugson_X64.exe; do
        [ -f "$EXE/$f" ] && cp "$EXE/$f" "$DST/altexe/$f"
    done
    renombrar Ventoy2Disk.exe Aether2Disk.exe
    renombrar VentoyPlugson.exe AetherPlugson.exe
    renombrar VentoyVlnk.exe AetherVInk.exe
    for f in "$DST"/altexe/*.exe; do
        [ -f "$f" ] || continue
        n="$(basename "$f")"; n="${n/Ventoy2Disk/Aether2Disk}"; n="${n/VentoyPlugson/AetherPlugson}"
        mv "$f" "$DST/altexe/$n"
    done
    TIPO="final"
else
    cat > "$DST/AVISO_EJECUTABLES.txt" <<'EOF'
PAQUETE DE PRUEBA
=================
Este paquete contiene el menú de arranque AetherBoot, la interfaz de AetherPlugson
y los textos en español, pero los ejecutables de Windows son todavía los originales
(Ventoy2Disk.exe, VentoyPlugson.exe, VentoyVlnk.exe), porque no se han compilado
desde el código de AetherBoot. Por eso su título e icono siguen siendo los antiguos.

Al instalar o actualizar un USB con este paquete, el USB arranca con el menú AetherBoot.

Para el paquete final (Aether2Disk.exe, AetherPlugson.exe y AetherVInk.exe con su
nueva identidad), compile el código con el flujo de GitHub Actions
.github/workflows/aetherboot.yml o con Visual Studio (ver Documentacion/11_Compilacion.md).
EOF
    TIPO="prueba"
fi

echo "==> Comprimiendo ($TIPO)"
rm -f "$SALIDA"
( cd "$TMP" && zip -qr -9 "$SALIDA" "aetherboot-$VER" )
echo "Listo: $SALIDA"
