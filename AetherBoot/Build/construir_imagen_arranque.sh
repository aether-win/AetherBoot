#!/bin/bash
# ============================================================================
#  construir_imagen_arranque.sh
#
#  Genera la partición de arranque de AetherBoot (ventoy.disk.img.xz) a partir
#  de la imagen original publicada, sustituyendo SOLO archivos de configuración
#  y apariencia de GRUB (grub/*.cfg, temas, idiomas, ayuda).
#
#  NO toca: binarios EFI firmados, GRUB compilado, shim, módulos, MOKManager,
#  ventoy.cpio, la etiqueta VTOYEFI ni la estructura de particiones.
#  Por eso Secure Boot sigue funcionando igual que en el original.
#
#  Uso:
#    ./construir_imagen_arranque.sh <ventoy.disk.img.xz original> <salida.xz>
#
#  Requisitos: bash, gcc, xz, tar (todo estándar en Linux).
# ============================================================================
set -euo pipefail

ORIG="$1"
SALIDA="$2"
RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"      # raíz del código fuente
GRUB="$RAIZ/INSTALL/grub"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "==> Compilando aetherfat"
FATLIB="$RAIZ/LinuxGUI/Ventoy2Disk/Lib/fat_io_lib"
gcc -O2 -I"$FATLIB" "$RAIZ/AetherBoot/Scripts/aetherfat/aetherfat.c" "$FATLIB"/fat_*.c -o "$TMP/aetherfat"
FAT="$TMP/aetherfat"

echo "==> Descomprimiendo imagen original"
xz -dc "$ORIG" > "$TMP/disk.img"
IMG="$TMP/disk.img"
"$FAT" "$IMG" get /grub/grub.cfg "$TMP/grub.cfg.orig" \
    || { echo "La imagen no contiene /grub/grub.cfg: no es una partición VTOYEFI válida"; exit 1; }
echo "    libre antes: $(( $("$FAT" "$IMG" free) / 1024 )) KiB"

# --- archivos de configuración (grub.cfg primero, como hace el empaquetador)
echo "==> Copiando configuración de GRUB"
for f in grub.cfg checksum.cfg debug.cfg extmenu.cfg hwinfo.cfg keyboard.cfg localboot.cfg power.cfg; do
    "$FAT" "$IMG" put "$GRUB/$f" "/grub/$f"
done

# --- menulang.cfg + menu.tar.gz + help.tar.gz (mismo proceso que ventoy_pack.sh)
echo "==> Generando idiomas y ayuda"
mkdir -p "$TMP/g"
cp -r "$GRUB/menu" "$GRUB/help" "$TMP/g/"
cp "$GRUB/menulang.cfg" "$TMP/g/menulang.cfg"
(
    cd "$TMP/g"
    t=$(grep VTLANG_LANGUAGE_NAME menu/zh_CN.json | awk -F\" '{print $4}')
    echo "menuentry \"zh_CN  -  $t\" --class=menu_lang_item --class=debug_menu_lang --class=F5tool {" >> menulang.cfg
    echo "    vt_load_menu_lang zh_CN" >> menulang.cfg
    echo "}" >> menulang.cfg
    ls -1 menu/ | grep -v 'zh_CN' | sort | while read -r l; do
        n=${l%.*}
        t=$(grep VTLANG_LANGUAGE_NAME "menu/$l" | awk -F\" '{print $4}')
        echo "menuentry \"$n  -  $t\" --class=menu_lang_item --class=debug_menu_lang --class=F5tool {" >> menulang.cfg
        echo "    vt_load_menu_lang $n" >> menulang.cfg
        echo "}" >> menulang.cfg
    done
    echo "menuentry \"\$VTLANG_RETURN_PREVIOUS\" --class=vtoyret VTOY_RET {" >> menulang.cfg
    echo "        echo \"Volviendo ...\"" >> menulang.cfg
    echo "}" >> menulang.cfg
    tar czf menu.tar.gz ./menu/
    tar czf help.tar.gz ./help/
)
for f in menulang.cfg menu.tar.gz help.tar.gz; do
    "$FAT" "$IMG" put "$TMP/g/$f" "/grub/$f"
done

# --- tema: se retira el original y se instala el de AetherBoot
echo "==> Instalando tema AetherBoot"
{ "$FAT" "$IMG" ls /grub/themes/ventoy 2>/dev/null || true; } | awk '$1=="-"{print $3}' > "$TMP/viejo"
while read -r f; do
    "$FAT" "$IMG" rm "$f"
done < "$TMP/viejo"
"$FAT" "$IMG" mkdir /grub/themes/aetherboot
"$FAT" "$IMG" mkdir /grub/themes/aetherboot/icons
( cd "$GRUB/themes/aetherboot" && find . -type f | sed 's|^\./||' ) | while read -r f; do
    "$FAT" "$IMG" put "$GRUB/themes/aetherboot/$f" "/grub/themes/aetherboot/$f"
done

# --- fuentes del tema (AetherBoot Sans, derivada de DejaVu Sans)
echo "==> Instalando fuentes AetherBoot Sans"
for f in "$GRUB"/fonts/aetherboot_*.pf2; do
    "$FAT" "$IMG" put "$f" "/grub/fonts/$(basename "$f")"
done

echo "    libre después: $(( $("$FAT" "$IMG" free) / 1024 )) KiB"

# --- verificación: lo escrito debe leerse idéntico
echo "==> Verificando"
errores=0
verificar() {
    "$FAT" "$IMG" get "$2" "$TMP/chk" && cmp -s "$1" "$TMP/chk" || { echo "    ERROR: $2"; errores=$((errores+1)); }
}
for f in grub.cfg checksum.cfg debug.cfg extmenu.cfg hwinfo.cfg keyboard.cfg localboot.cfg power.cfg; do
    verificar "$GRUB/$f" "/grub/$f"
done
for f in menulang.cfg menu.tar.gz help.tar.gz; do verificar "$TMP/g/$f" "/grub/$f"; done
( cd "$GRUB/themes/aetherboot" && find . -type f | sed 's|^\./||' ) > "$TMP/lista"
while read -r f; do verificar "$GRUB/themes/aetherboot/$f" "/grub/themes/aetherboot/$f"; done < "$TMP/lista"
for f in "$GRUB"/fonts/aetherboot_*.pf2; do verificar "$f" "/grub/fonts/$(basename "$f")"; done
[ "$errores" -eq 0 ] || { echo "Hubo $errores errores de verificación"; exit 1; }
echo "    $(( $(wc -l < "$TMP/lista") + 10 )) archivos verificados byte a byte"

echo "==> Comprimiendo"
xz --check=crc32 -c "$IMG" > "$SALIDA"   # mismo formato que ventoy_pack.sh
echo "Listo: $SALIDA ($(du -h "$SALIDA" | cut -f1))"
