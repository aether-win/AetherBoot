#!/bin/sh
#
# VerificarISOs.sh - comprueba la integridad de las imágenes copiadas a una USB Ventoy.
#
# Para cada imagen (.iso .wim .img .vhd .vhdx .efi .dat .vtoy) busca una suma de
# verificación de referencia y la compara con la del archivo:
#   * un archivo hermano:  imagen.iso.sha256 / .sha256sum / .sha512 / .sha1 / .md5
#   * o una lista en la misma carpeta: SHA256SUMS, sha256sum.txt, CHECKSUM, *.sha256 ...
#     (formatos GNU "hash  archivo", "hash *archivo" y BSD "SHA256 (archivo) = hash")
#
# Uso:
#   sh VerificarISOs.sh [-g] [carpeta]
#
#   carpeta   raíz de la partición Ventoy (por defecto: se busca la partición
#             montada con etiqueta "Aether" o "Ventoy", o la carpeta actual)
#   -g        genera "imagen.sha256" para las imágenes que no tienen referencia,
#             para poder comprobarlas más adelante (p. ej. tras copiar a otra USB)
#
# Código de salida: 0 si todo coincide, 1 si alguna imagen NO coincide.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later version.

GENERATE=0
ROOT=""

for arg in "$@"; do
    case "$arg" in
        -g|--generar) GENERATE=1 ;;
        -h|--help|--ayuda)
            sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) ROOT="$arg" ;;
    esac
done

if [ -z "$ROOT" ]; then
    if command -v findmnt >/dev/null 2>&1; then
        ROOT=$(findmnt -rn -o TARGET -S LABEL=Aether 2>/dev/null | head -n1)
        [ -z "$ROOT" ] && ROOT=$(findmnt -rn -o TARGET -S LABEL=Ventoy 2>/dev/null | head -n1)
    fi
    [ -z "$ROOT" ] && ROOT="."
fi

if [ ! -d "$ROOT" ]; then
    echo "No existe la carpeta: $ROOT" >&2
    exit 2
fi

if [ -t 1 ]; then
    C_OK=$(printf '\033[32m'); C_BAD=$(printf '\033[31m'); C_WARN=$(printf '\033[33m'); C_END=$(printf '\033[0m')
else
    C_OK=""; C_BAD=""; C_WARN=""; C_END=""
fi

hash_tool() {
    case "$1" in
        32)  echo md5sum ;;
        40)  echo sha1sum ;;
        64)  echo sha256sum ;;
        128) echo sha512sum ;;
        *)   echo "" ;;
    esac
}

# print the first hex digest (32/40/64/128 chars) found in stdin
first_digest() {
    tr -s ' \t*()=' '\n\n\n\n\n\n' | tr -d '\r' | grep -E '^([0-9a-fA-F]{32}|[0-9a-fA-F]{40}|[0-9a-fA-F]{64}|[0-9a-fA-F]{128})$' | head -n1
}

# look for a reference digest for image $1; prints it or nothing
find_reference() {
    img="$1"
    dir=$(dirname "$img")
    base=$(basename "$img")

    for ext in sha256 sha256sum sha512 sha512sum sha1 sha1sum md5 md5sum; do
        for cand in "$img.$ext" "$img.$(echo $ext | tr 'a-z' 'A-Z')"; do
            if [ -f "$cand" ]; then
                # a sidecar may list just the hash, or "hash  name"
                line=$(grep -F -- "$base" "$cand" 2>/dev/null | head -n1)
                [ -z "$line" ] && line=$(head -n1 "$cand")
                d=$(printf '%s\n' "$line" | first_digest)
                if [ -n "$d" ]; then echo "$d"; return 0; fi
            fi
        done
    done

    for list in "$dir"/SHA256SUMS* "$dir"/SHA512SUMS* "$dir"/SHA1SUMS* "$dir"/MD5SUMS* \
                "$dir"/sha256sum* "$dir"/sha512sum* "$dir"/md5sum* "$dir"/CHECKSUM* \
                "$dir"/*.sha256 "$dir"/*.sha512 "$dir"/*CHECKSUM*; do
        [ -f "$list" ] || continue
        line=$(grep -F -- "$base" "$list" 2>/dev/null | grep -vF -- "$base." | head -n1)
        [ -z "$line" ] && continue
        d=$(printf '%s\n' "$line" | first_digest)
        if [ -n "$d" ]; then echo "$d"; return 0; fi
    done
    return 1
}

total=0; ok=0; bad=0; noref=0; gen=0
tmplist=$(mktemp)
find "$ROOT" -type f \( -iname '*.iso' -o -iname '*.wim' -o -iname '*.img' -o -iname '*.vhd' \
     -o -iname '*.vhdx' -o -iname '*.efi' -o -iname '*.dat' -o -iname '*.vtoy' \) \
     -not -path '*/ventoy/*' -not -path '*/.Trash*' -not -path '*/$RECYCLE.BIN/*' 2>/dev/null | sort > "$tmplist"

echo "Verificando imágenes en: $ROOT"
echo ""

while IFS= read -r img; do
    total=$((total + 1))
    rel=${img#"$ROOT"/}
    ref=$(find_reference "$img")

    if [ -z "$ref" ]; then
        if [ "$GENERATE" = "1" ]; then
            printf '  %s ... ' "$rel"
            d=$(sha256sum "$img" | awk '{print $1}')
            printf '%s  %s\n' "$d" "$(basename "$img")" > "$img.sha256"
            echo "${C_WARN}GENERADA${C_END} ($img.sha256)"
            gen=$((gen + 1))
        else
            echo "  ${C_WARN}SIN REFERENCIA${C_END}  $rel"
            noref=$((noref + 1))
        fi
        continue
    fi

    tool=$(hash_tool ${#ref})
    printf '  %s [%s] ... ' "$rel" "${tool%sum}"
    calc=$($tool "$img" | awk '{print $1}')
    if [ "$(echo "$calc" | tr 'A-F' 'a-f')" = "$(echo "$ref" | tr 'A-F' 'a-f')" ]; then
        echo "${C_OK}OK${C_END}"
        ok=$((ok + 1))
    else
        echo "${C_BAD}NO COINCIDE${C_END}"
        echo "      esperado: $ref"
        echo "      obtenido: $calc"
        bad=$((bad + 1))
    fi
done < "$tmplist"
rm -f "$tmplist"

echo ""
echo "Total: $total   Correctas: $ok   Dañadas: $bad   Sin referencia: $noref   Generadas: $gen"
if [ "$total" -eq 0 ]; then
    echo "No se encontraron imágenes."
fi
if [ "$bad" -gt 0 ]; then
    echo "${C_BAD}Hay imágenes dañadas: vuelva a descargarlas o copiarlas.${C_END}"
    exit 1
fi
exit 0
